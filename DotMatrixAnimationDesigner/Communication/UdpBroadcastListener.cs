using System;
using System.Net;
using System.Net.Sockets;
using CommunityToolkit.Mvvm.ComponentModel;

namespace DotMatrixAnimationDesigner.Communication
{
    internal class UdpBroadcastListener : ObservableObject
    {
        public event EventHandler<UdpListenResult>? UdpListenDone;

        #region Public properties

        #region Private fields
        private bool _lastScanSuccessful = true;
        private string _lastErrorMessage = string.Empty;
        #endregion

        public bool LastScanSuccessful
        {
            get => _lastScanSuccessful;
            private set => SetProperty(ref _lastScanSuccessful, value);
        }

        public string LastErrorMessage
        {
            get => _lastErrorMessage;
            private set => SetProperty(ref _lastErrorMessage, value);
        }
        #endregion

        #region Private fields
        private const int ExpectedNumberOfBytesInUdpPacket = 6;
        #endregion

        public void StartUdpBroadcastListen(IPAddress localAddress, int udpPort)
        {
            var endpoint = new IPEndPoint(localAddress, udpPort);
            using var client = new UdpClient(endpoint);
            client.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            client.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, 45000);

            var broadcastListenEndPoint = new IPEndPoint(IPAddress.Any, udpPort);
            UdpListenResult result;
            try
            {
                var receivedBytes = client.Receive(ref broadcastListenEndPoint);
                var sourceIpAddress = broadcastListenEndPoint.Address.MapToIPv4().ToString();

                if (receivedBytes.Length == ExpectedNumberOfBytesInUdpPacket)
                {
                    var targetPort = (ushort)((ushort)(receivedBytes[0] << 8) | receivedBytes[1]);
                    var targetIpAddress = new IPAddress(receivedBytes[2..]);
                    result = UdpListenResult.FromSuccess(targetIpAddress, targetPort);
                }
                else
                {
                    result = UdpListenResult.FromFailure($"Received message from {sourceIpAddress}, but the message had unexpected number of bytes ({receivedBytes.Length}, expected {ExpectedNumberOfBytesInUdpPacket})");
                }
            }
            catch (Exception e) when (e is SocketException or ObjectDisposedException)
            {
                var isTimeout = e is SocketException sE && sE.SocketErrorCode == SocketError.TimedOut;
                result = UdpListenResult.FromFailure(isTimeout ? "UDP listener timed-out" : $"UDP listener failed - {e.GetType()}: {e.Message}");

            }

            LastScanSuccessful = result.Success;
            LastErrorMessage = result.ErrorMessage;
            UdpListenDone?.Invoke(this, result);
        }
    }
}
