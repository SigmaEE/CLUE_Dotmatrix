using System.Net;

namespace DotMatrixAnimationDesigner.Communication
{
    internal readonly struct UdpListenResult
    {
        public bool Success { get; }
        
        public string ErrorMessage { get; }
        
        public IPAddress IPAddress { get; }
        
        public int Port { get; }

        public static UdpListenResult FromSuccess(IPAddress address, int port)
            => new(true, string.Empty, address, port);

        public static UdpListenResult FromFailure(string errorMessage)
            => new(false, errorMessage, IPAddress.None, -1);

        private UdpListenResult(bool success, string errorMessage, IPAddress ipAddress, int port)
        {
            Success = success;
            ErrorMessage = errorMessage;
            IPAddress = ipAddress;
            Port = port;
        }
    }
}
