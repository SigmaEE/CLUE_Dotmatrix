using System;
using System.Net;
using System.Net.Sockets;
using System.Security;
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
        ConnectionFailed
    }

    internal class DotMatrixConnection : ObservableObject, IDisposable
    {
        #region Public properties

        #region Backing fields
        private ConnectionStatus _status = ConnectionStatus.ConnectionParametersUnknown;
        private string _connectionString = string.Empty;
        private string _lastConnectionErrorMessage = string.Empty;
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
        #endregion

        private IPAddress? _ipAddress;
        private TcpClient? _client;
        private int _port;
        private bool _disposedValue;
        private readonly Timer _checkConnectionAliveTimer;
        private const int CheckConnectionAliveIntervalMs = 20000;

        public DotMatrixConnection()
        {
            _checkConnectionAliveTimer = new((s) => IsConnectionStillAlive(), null, Timeout.Infinite, Timeout.Infinite);
        }

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
                var didReturnNormally =  _client.ConnectAsync(_ipAddress, _port).Wait(3000);
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
                _checkConnectionAliveTimer.Dispose();

                Status = ConnectionStatus.NotConnected;
            }
        }
        
        #region Private fields
        private void IsConnectionStillAlive()
        {
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
        #endregion

        #region Disposal
        protected virtual void Dispose(bool disposing)
        {
            if (!_disposedValue)
            {
                if (disposing)
                    Disconnect();
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
