using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Security;
using System.Threading;
using System.Threading.Tasks;
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
        private const int CheckConnectionAliveIntervalMs = 10000;
        private const int ResponseTimeoutMs = 5000;

        private static readonly Dictionary<byte, ResponseCode> s_responseCodeMap = new();
        #endregion

        public DotMatrixConnection()
        {
            _checkConnectionAliveTimer = new((s) => IsConnectionStillAlive(), null, Timeout.Infinite, Timeout.Infinite);
            _clearTransmissionMessageProgressTimer = new((s) => TransmissionProgressMessage = string.Empty, null, Timeout.Infinite, Timeout.Infinite);
        }

        #region Public methods
        public void SetConnectionInformation(IPAddress ipAddress, int port)
        {
            _ipAddress = ipAddress;
            _port = port;
            ConnectionString = $"{ipAddress.MapToIPv4()}:{port}";
        }

        public async Task TryConnect(int timeoutMs)
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
            CancellationTokenSource cts = new();
            cts.CancelAfter(timeoutMs);

            try
            {
                await _client.ConnectAsync(_ipAddress, _port, cts.Token);
                if (_client.Connected)
                    _client.Client.LingerState = new LingerOption(false, 0);
                else
                    LastConnectionErrorMessage = $"Connection to {ConnectionString} failed";
            }
            catch (Exception e) when (e is ArgumentNullException ||
                                        e is SocketException ||
                                        e is ObjectDisposedException ||
                                        e is SecurityException ||
                                        e is InvalidOperationException ||
                                        e is OperationCanceledException)
            {
                LastConnectionErrorMessage = cts.IsCancellationRequested ? $"Connection to {ConnectionString} timed-out" : $"{e.GetType()}: {e.Message}";
            }

            cts.Dispose();
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
                        _client.Close();
                    }
                    catch (SocketException) { }
                    _client.Dispose();
                }

                _checkConnectionAliveTimer.Change(Timeout.Infinite, Timeout.Infinite);
                Status = ConnectionStatus.NotConnected;
            }
        }

        public void Transmit(DotMatrixContainer container, TransmitOption transmitOption, bool transmitOnlyCurrent)
        {
            if (_client == default || !_client.Connected)
                return;

            Status = ConnectionStatus.Sending;
            TransmissionProgressMessage = $"Transmitting {transmitOption} header...";

            DotMatrixMessage msg = transmitOption switch
            {
                TransmitOption.AnimationFrames => AnimationFramesMessage.Get(container, transmitOnlyCurrent),
                TransmitOption.GameOfLifeConfig => GameOfLifeConfigMesssage.Get(container),
                _ => throw new NotImplementedException(),
            };

            var responseCode = SendAndAwaitResponse(_client, msg.GetTransmissionHeader());
            if (responseCode != ResponseCode.Ok)
                return;

            for (var i = 0; i < msg.NumberOfPackets; i++)
            {
                responseCode = SendAndAwaitResponse(_client, msg.GetPacket(i), $"Frame {i + 1}/{msg.NumberOfPackets}");
                if (responseCode == ResponseCode.ChecksumMismatch)
                {
                    i--;
                    continue;
                }
                else if (responseCode != ResponseCode.Ok)
                {
                    return;
                }
            }

            TransmissionProgressMessage = "Successfully transmitted all data";
            ResetConnectionStatusAndSetClearMessageTimer();
        }
        #endregion

        #region Private methods
        private void IsConnectionStillAlive()
        {
            if (Status == ConnectionStatus.Sending)
                return;

            var shouldDisconnect = true;
            if (_client != null && _client.Connected)
                shouldDisconnect = SendAndAwaitResponse(_client, new ConnectionAlivePingMessage().GetTransmissionHeader(), suppressOutput: true) != ResponseCode.Ok;

            if (shouldDisconnect)
                Disconnect();
            else
                _checkConnectionAliveTimer.Change(CheckConnectionAliveIntervalMs, Timeout.Infinite);
        }

        private ResponseCode SendAndAwaitResponse(TcpClient client, ReadOnlySpan<byte> data, string statusPrefix = "", bool suppressOutput = false)
        {
            statusPrefix = string.IsNullOrEmpty(statusPrefix) ? "" : $"[{statusPrefix}] ";
            try
            {
                client.Client.Send(data);
            }
            catch (SocketException e)
            {
                TransmissionProgressMessage = $"{statusPrefix}Transmission failed, got SocketException {e.ErrorCode} ({e.Message})";
                ResetConnectionStatusAndSetClearMessageTimer();
                return ResponseCode.TimeoutAccessPoint;
            }

            if (!suppressOutput)
                TransmissionProgressMessage = $"{statusPrefix}Awaiting response...";

            var responseCode = GetResponseCodeFromClient(client);
            if (responseCode != ResponseCode.Ok)
            {
                if (!suppressOutput)
                    TransmissionProgressMessage = $"{statusPrefix}Transmission failed, got '{responseCode}' response";
                ResetConnectionStatusAndSetClearMessageTimer();
            }
            return responseCode;
        }

        private void ResetConnectionStatusAndSetClearMessageTimer()
        {
            Status = ConnectionStatus.Connected;
            _clearTransmissionMessageProgressTimer.Change(5000, Timeout.Infinite);
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
