using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Security;
using System.Text;
using System.Threading;
using CommunityToolkit.Mvvm.ComponentModel;

namespace DotMatrixAnimationDesigner.Communication
{
    public enum ConnectionStatus
    {
        ConnectionParametersUnknown,
        NotConnected,
        Connecting,
        Connected,
        Sending,
        ConnectionFailed
    }

    public enum ResponseCode : byte
    {
        Ok = 0x01,
        TargetInternalError = 0x02,
        ChecksumMismatch = 0x03,
        TargetTimeout = 0x04,
        TargetUnexpectedResponse = 0x05,
        TargetUnexpectedPacket = 0x06,
        TimeoutAccessPoint = 0xff
    }

    internal class DotMatrixConnection : ObservableObject, IDisposable
    {
        #region Public properties

        #region Backing fields
        private ConnectionStatus _status = ConnectionStatus.ConnectionParametersUnknown;
        private string _connectionString = string.Empty;
        private string _lastConnectionErrorMessage = string.Empty;
        private string _transmissionProgressMessage = string.Empty;
        #endregion

        public ConnectionStatus Status
        {
            get => _status;
            set => SetProperty(ref _status, value);
        }

        public string ConnectionString
        {
            get => _connectionString;
            private set => SetProperty(ref _connectionString, value);
        }

        public string LastConnectionErrorMessage
        {
            get => _lastConnectionErrorMessage;
            private set => SetProperty(ref _lastConnectionErrorMessage, value);
        }

        public string TransmissionProgressMessage
        {
            get => _transmissionProgressMessage;
            private set => SetProperty(ref _transmissionProgressMessage, value);
        }
        #endregion

        #region Private fields
        private IPAddress? _ipAddress;
        private TcpClient? _client;
        private int _port;
        private bool _disposedValue;

        private readonly Timer _checkConnectionAliveTimer;
        private readonly Timer _clearTransmissionMessageProgressTimer;
        private const int CheckConnectionAliveIntervalMs = 20000;
        private const ushort Crc16Polynomial = 0x1021;
        private const int ResponseTimeoutMs = 5000;
        private const byte AnimationFrameMessageIdentifier = 0x10;
        private const byte GameOfLifeConfigIdentifier = 0x11;

        private static readonly Dictionary<byte, ResponseCode> s_responseCodeMap = new();
        #endregion

        public DotMatrixConnection()
        {
            _checkConnectionAliveTimer = new((s) => IsConnectionStillAlive(), null, Timeout.Infinite, Timeout.Infinite);
            _clearTransmissionMessageProgressTimer = new((s) => TransmissionProgressMessage = string.Empty, null, Timeout.Infinite, Timeout.Infinite);

            var crc = ComputeCrc16Checksum(Encoding.ASCII.GetBytes("123456789"));
        }

        #region Public methods
        public void SetConnectionInformation(IPAddress ipAddress, int port)
        {
            _ipAddress = ipAddress;
            _port = port;
            ConnectionString = $"{ipAddress.MapToIPv4()}:{port}";
        }

        public void TryConnect()
        {
            if (_ipAddress == null)
            {
                LastConnectionErrorMessage = "Connection parameters not set";
                Status = ConnectionStatus.ConnectionParametersUnknown;
                return;
            }

            Status = ConnectionStatus.Connecting;
            _client = new(AddressFamily.InterNetwork);
            LastConnectionErrorMessage = string.Empty;

            try
            {
                var didReturnNormally = _client.ConnectAsync(_ipAddress, _port).Wait(3000);
                if (_client.Connected)
                    _client.Client.LingerState = new LingerOption(false, 0);
                else
                    LastConnectionErrorMessage = $"Connection to {ConnectionString} {(didReturnNormally ? "failed" : "timed-out")}";
            }
            catch (Exception e) when (e is ArgumentNullException ||
                                        e is SocketException ||
                                        e is ObjectDisposedException ||
                                        e is SecurityException ||
                                        e is InvalidOperationException)
            {
                LastConnectionErrorMessage = $"{e.GetType()}: {e.Message}";
            }

            Status = string.IsNullOrEmpty(LastConnectionErrorMessage) ? ConnectionStatus.Connected : ConnectionStatus.ConnectionFailed;
            if (Status == ConnectionStatus.Connected)
            {
                _checkConnectionAliveTimer.Change(CheckConnectionAliveIntervalMs, Timeout.Infinite);
            }
            else
            {
                _client.Dispose();
                _client = null;
            }
        }

        public void Disconnect()
        {
            if (_client != null)
            {
                if (_client.Client != null && _client.Client.Connected)
                {
                    try
                    {
                        _client.Client.Disconnect(reuseSocket: false);
                        _client.Client.Close();
                    }
                    catch (SocketException) { }
                    _client.Dispose();
                }

                _checkConnectionAliveTimer.Change(Timeout.Infinite, Timeout.Infinite);
                Status = ConnectionStatus.NotConnected;
            }
        }

        public void TransmitRawFrames(DotMatrixContainer container, bool transmitOnlyCurrent)
        {
            if (_client == default || !_client.Connected)
                return;

            Status = ConnectionStatus.Sending;
            TransmissionProgressMessage = "Transmitting AnimationFrames header...";
            var startFrameNumber = transmitOnlyCurrent ? container.SelectedFrame : 1;
            var endFrameNumber = transmitOnlyCurrent ? container.SelectedFrame : container.TotalNumberOfFrames;
            var numberOfFramesToTransmit = endFrameNumber - startFrameNumber + 1;
            var numberOfBytesPerFrame = container.GetBytesForFrame(startFrameNumber).Length;
            var transmissionHeader = GetHeaderForAnimationFramesTransmission(numberOfFramesToTransmit, numberOfBytesPerFrame, container.GridHeight, container.GridWidth);
            if (!SendHeaderAndAwaitResponse(_client, transmissionHeader))
                return;

            var frameCounter = 0;
            var checksumMismatch = false;
            for (var i = startFrameNumber; i <= endFrameNumber; i++)
            {
                TransmissionProgressMessage = $"[Frame {frameCounter + 1}/{numberOfFramesToTransmit}] {(checksumMismatch ? "Checksum mismatch, sending again" : "Sending")}...";
                var message = GetMessage(container.GetBytesForFrame(i));
                _client.Client.Send(message);
                TransmissionProgressMessage = $"[Frame {frameCounter + 1}/{numberOfFramesToTransmit}] Awaiting response...";

                var responseCode = GetResponseCodeFromClient(_client);
                if (responseCode == ResponseCode.ChecksumMismatch)
                {
                    checksumMismatch = true;
                    i--;
                    continue;
                }
                else if (responseCode != ResponseCode.Ok)
                {
                    TransmissionProgressMessage = $"Transmission failed, got '{responseCode}' response";
                    ResetConnectionStatusAndSetClearMessageTimer();
                    return;
                }

                checksumMismatch = false;
            }

            TransmissionProgressMessage = "Successfully transmitted all data";
            ResetConnectionStatusAndSetClearMessageTimer();
        }

        public void TransmitGameOfLifeConfig(DotMatrixContainer container)
        {
            if (_client == default || !_client.Connected)
                return;

            Status = ConnectionStatus.Sending;
            TransmissionProgressMessage = "Transmitting GameOfLifeConfig header...";
            if (!SendHeaderAndAwaitResponse(_client, GetHeaderForGameOfLifeConfigTransmission()))
                return;

            var checksumMismatch = false;
            while (true)
            {
                TransmissionProgressMessage = checksumMismatch ? "Checksum mismatch, sending again..." : "Sending GameOfLife data to target...";
                _client.Client.Send(GetMessage(container.GetBytesForFrameAsCoordinates(container.SelectedFrame)));
                TransmissionProgressMessage = "Awaiting response...";

                var responseCode = GetResponseCodeFromClient(_client);
                if (responseCode == ResponseCode.ChecksumMismatch)
                {
                    checksumMismatch = true;
                    continue;
                }

                if (responseCode != ResponseCode.Ok)
                    TransmissionProgressMessage = $"Transmission failed, got '{responseCode}' response";
                else
                    TransmissionProgressMessage = "Successfully transmitted all data";
                ResetConnectionStatusAndSetClearMessageTimer();
                break;
            }
        }
        #endregion

        #region Private methods
        private void IsConnectionStillAlive()
        {
            if (Status == ConnectionStatus.Sending)
                return;

            var connectionDisconnect = false;
            var bytesAvailable = false;
            if (_client != null && _client.Client.Connected)
            {
                connectionDisconnect = _client.Client.Poll(1000, SelectMode.SelectRead);
                bytesAvailable = (_client.Client.Available == 0);
            }

            if ((connectionDisconnect && bytesAvailable) || (_client != null && !_client.Client.Connected))
                Disconnect();
            else
                _checkConnectionAliveTimer.Change(CheckConnectionAliveIntervalMs, Timeout.Infinite);
        }

        private bool SendHeaderAndAwaitResponse(TcpClient client, ReadOnlySpan<byte> header)
        {
            client.Client.Send(header);
            TransmissionProgressMessage = "Awaiting response...";
            var responseCode = GetResponseCodeFromClient(client);
            if (responseCode != ResponseCode.Ok)
            {
                TransmissionProgressMessage = $"Transmission failed, got '{responseCode}' response";
                ResetConnectionStatusAndSetClearMessageTimer();
            }
            return responseCode == ResponseCode.Ok;
        }

        private void ResetConnectionStatusAndSetClearMessageTimer()
        {
            Status = ConnectionStatus.Connected;
            _clearTransmissionMessageProgressTimer.Change(5000, Timeout.Infinite);
        }

        private static ReadOnlySpan<byte> GetHeaderForAnimationFramesTransmission(int numberOfFrames, int numberOfBytesPerFrame, int numberOfRows, int numberOfColumns)
        {
            var result = new byte[1 + 2 + 2 + 1 + 1];
            result[0] = AnimationFrameMessageIdentifier;
            var numberOfFramesBuffer = BitConverter.GetBytes((ushort)numberOfFrames);
            var numberOfBytesPerFrameBuffer = BitConverter.GetBytes((ushort)numberOfBytesPerFrame);

            Buffer.BlockCopy(numberOfFramesBuffer, 0, result, 1, numberOfFramesBuffer.Length);
            Buffer.BlockCopy(numberOfBytesPerFrameBuffer, 0, result, 3, numberOfBytesPerFrameBuffer.Length);
            result[5] = (byte)numberOfRows;
            result[6] = (byte)numberOfColumns;

            return result;
        }

        private static ReadOnlySpan<byte> GetHeaderForGameOfLifeConfigTransmission()
            => new(new byte[] { GameOfLifeConfigIdentifier });

        private static ReadOnlySpan<byte> GetMessage(ReadOnlySpan<byte> payloadBytes)
        {
            var payloadLength = (ushort)payloadBytes.Length;
            var crc = ComputeCrc16Checksum(payloadBytes);

            // Header is payloadLength + crc
            var message = new byte[2 + 2 + payloadLength];
            message[0] = (byte)(crc & 0xff);
            message[1] = (byte)((crc >> 8) & 0xff);
            message[2] = (byte)(payloadLength & 0xff);
            message[3] = (byte)((payloadLength >> 8) & 0xff);
            Buffer.BlockCopy(payloadBytes.ToArray(), 0, message, 4, payloadLength);

            return message;
        }

        private static ResponseCode GetResponseCodeFromClient(TcpClient client)
        {
            try
            {
                client.Client.ReceiveTimeout = ResponseTimeoutMs;
                var buffer = new byte[client.ReceiveBufferSize];
                var numberOfReceivedBytes = client.Client.Receive(buffer);
                if (numberOfReceivedBytes == 1 && TryGetAsResponseCode(buffer[0], out var responseCode))
                    return responseCode;
                else
                    return ResponseCode.TargetUnexpectedPacket;
            }
            catch (SocketException)
            {
                return ResponseCode.TimeoutAccessPoint;
            }


        }

        private static bool TryGetAsResponseCode(byte rawValue, out ResponseCode responseCode)
        {
            if (s_responseCodeMap.TryGetValue(rawValue, out responseCode))
                return true;

            if (Enum.IsDefined(typeof(ResponseCode), rawValue))
            {
                responseCode = (ResponseCode)rawValue;
                s_responseCodeMap.Add(rawValue, responseCode);
                return true;
            }
            else
            {
                responseCode = ResponseCode.TargetUnexpectedPacket;
                return false;
            }
        }

        private static ushort ComputeCrc16Checksum(ReadOnlySpan<byte> bytes)
        {
            // See https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
            ushort crc = 0xffff;
            var i = 0;
            while (i < bytes.Length)
            {
                crc ^= (ushort)(bytes[i] << 8);
                for (var j = 0; j < 8; j++)
                {
                    if ((crc & 0x8000) > 0)
                        crc = (ushort)((ushort)(crc << 1) ^ Crc16Polynomial);
                    else
                        crc = (ushort)(crc << 1);
                }
                i++;
            }
            return crc;
        }
        #endregion

        #region Disposal
        protected virtual void Dispose(bool disposing)
        {
            if (!_disposedValue)
            {
                if (disposing)
                {
                    Disconnect();
                    _checkConnectionAliveTimer.Dispose();
                    _clearTransmissionMessageProgressTimer.Change(Timeout.Infinite, Timeout.Infinite);
                    _clearTransmissionMessageProgressTimer.Dispose();
                }
                _disposedValue = true;
            }
        }

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
