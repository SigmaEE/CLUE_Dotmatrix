using System;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using DotMatrixAnimationDesigner.Communication;
using Ookii.Dialogs.Wpf;

namespace DotMatrixAnimationDesigner
{
    public enum ChangeDirection
    {
        Left,
        Right,
        Up,
        Down
    }

    public enum ExportOption
    {
        Raw,
        RawCpp,
        Coordinates,
        CppFunctionCalls
    }

    public enum TransmitOption
    {
        AnimationFrames,
        GameOfLifeConfig
    }

    internal class MainWindowViewModel : ObservableObject
    {
        #region Public properties

        #region Backing fields
        private int _width = 28;
        private int _height = 16;
        private int _udpListenPort = 5578;
        private bool _isScanningNetwork = false;
        private ExportOption _exportOption = ExportOption.Raw;
        private TransmitOption _transmitOption = TransmitOption.AnimationFrames;
        #endregion

        public int Width
        {
            get => _width;
            set => SetProperty(ref _width, value);
        }

        public int Height
        {
            get => _height;
            set => SetProperty(ref _height, value);
        }

        public int UdpListenPort
        {
            get => _udpListenPort;
            set => SetProperty(ref _udpListenPort, value);
        }

        public bool IsScanningNetwork
        {
            get => _isScanningNetwork;
            set => SetProperty(ref _isScanningNetwork, value);
        }

        public ExportOption ExportOption
        {
            get => _exportOption;
            set => SetProperty(ref _exportOption, value);
        }

        public TransmitOption TransmitOption
        {
            get => _transmitOption;
            set => SetProperty(ref _transmitOption, value);
        }


        public DotMatrixContainer Container { get; }

        public UdpBroadcastListener UdpListener { get; }

        public DotMatrixConnection Connection { get; }
        #endregion

        #region Commands
        private ICommand? _setDimensionsCommand;
        private ICommand? _clearCurrentFrameCommand;
        private ICommand? _exportFrameCommand;
        private ICommand? _exportFramesCommand;
        private ICommand? _importFrameOrFramesCommand;
        private ICommand? _moveImageCommand;
        private ICommand? _addNewFrameCommand;
        private ICommand? _changeFrameCommand;
        private ICommand? _deleteFrameCommand;
        private ICommand? _scanNetworkCommand;
        private ICommand? _connectOrDisconnectToPinMatrixCommand;
        private ICommand? _sendFrameToPinMatrixCommand;
        private ICommand? _sendFramesToPinMatrixCommand;

        public ICommand SetDimensionsCommand
        {
            get
            {
                _setDimensionsCommand ??= new RelayCommand(() => Container.SetDimensions(Width, Height));
                return _setDimensionsCommand;
            }
        }

        public ICommand ClearCurrentFrameCommand
        {
            get
            {
                _clearCurrentFrameCommand ??= new RelayCommand(Container.ClearCurrentFrame);
                return _clearCurrentFrameCommand;
            }
        }

        public ICommand ExportFrameCommand
        {
            get
            {
                _exportFrameCommand ??= new RelayCommand(() => ExportFrames(true));
                return _exportFrameCommand;
            }
        }

        public ICommand ExportFramesCommand
        {
            get
            {
                _exportFramesCommand ??= new RelayCommand(() => ExportFrames(false));
                return _exportFramesCommand;
            }
        }

        public ICommand ImportFrameOrFramesCommand
        {
            get
            {
                _importFrameOrFramesCommand ??= new RelayCommand(ImportFrameOrFrames);
                return _importFrameOrFramesCommand;
            }
        }

        public ICommand MoveImageCommand
        {
            get
            {
                _moveImageCommand ??= new RelayCommand<ChangeDirection>(Container.MoveImage);
                return _moveImageCommand;
            }
        }

        public ICommand AddNewFrameCommand
        {
            get
            {
                _addNewFrameCommand ??= new RelayCommand<bool>(Container.AddNewFrame);
                return _addNewFrameCommand;
            }
        }

        public ICommand ChangeFrameCommand
        {
            get
            {
                _changeFrameCommand ??= new RelayCommand<ChangeDirection>(Container.ChangeFrame);
                return _changeFrameCommand;
            }
        }

        public ICommand DeleteFrameCommand
        {
            get
            {
                _deleteFrameCommand ??= new RelayCommand(Container.DeleteFrame);
                return _deleteFrameCommand;
            }
        }

        public ICommand ScanNetworkCommand
        {
            get
            {
                _scanNetworkCommand ??= new RelayCommand(ScanNetwork);
                return _scanNetworkCommand;
            }
        }

        public ICommand ConnectOrDisconnectToPinMatrixCommand
        {
            get
            {
                _connectOrDisconnectToPinMatrixCommand ??= new RelayCommand(ConnectOrDisconnect);
                return _connectOrDisconnectToPinMatrixCommand;
            }
        }

        public ICommand SendFrameToPinMatrixCommand
        {
            get
            {
                _sendFrameToPinMatrixCommand ??= new RelayCommand(() => TransmitFrames(true));
                return _sendFrameToPinMatrixCommand;
            }
        }

        public ICommand SendFramesToPinMatrixCommand
        {
            get
            {
                _sendFramesToPinMatrixCommand ??= new RelayCommand(() => TransmitFrames(false));
                return _sendFramesToPinMatrixCommand;
            }
        }
        #endregion

        private readonly VistaSaveFileDialog _exportDialog = new()
        {
            AddExtension = true,
        };

        private readonly VistaOpenFileDialog _frameImportPathDialog = new()
        {
            Title = "Import frame data",
            Filter = "Binary file (*.dat)|*.dat",
            Multiselect = false
        };

        public MainWindowViewModel()
        {
            Container = new(Width, Height);
            Connection = new();
            UdpListener = new();

            UdpListener.UdpListenDone += UdpListenDone;
        }

        private void UdpListenDone(object? sender, UdpListenResult e)
        {
            IsScanningNetwork = false;
            if (e.Success)
            {
                Connection.SetConnectionInformation(e.IPAddress, e.Port);
                Connection.Status = ConnectionStatus.NotConnected;
            }
        }

        #region Private methods
        private void ImportFrameOrFrames()
        {
            if (_frameImportPathDialog.ShowDialog() != true)
                return;

            var frameData = File.ReadAllBytes(_frameImportPathDialog.FileName);
            var numberOfFrames = frameData[0];
            var fileWidth = frameData[1];
            var fileHeigth = frameData[2];

            if (fileWidth != Width || fileHeigth != Height)
            {
                var result = MessageBox.Show("Mismatched dimensions", "The dimensions in the file does not match current settings.\nProceeding will clear all current frames.", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                if (result == MessageBoxResult.Cancel)
                    return;

                Width = fileWidth;
                Height = fileHeigth;
                Container.SetDimensions(Width, Height);
            }

            Container.ImportFrames(numberOfFrames, frameData[3..].AsSpan());
        }

        private void ExportFrames(bool onlyExportCurrent)
        {
            var formatSupportsMultipleFrames = ExportOption == ExportOption.Raw || ExportOption == ExportOption.RawCpp;
            var isCppFormat = ExportOption == ExportOption.RawCpp || ExportOption == ExportOption.CppFunctionCalls;

            _exportDialog.Title = onlyExportCurrent ? "Export current frame" :
                (formatSupportsMultipleFrames ? "Export all frames" : "Export current frame");

            _exportDialog.Filter = ExportOption == ExportOption.Raw ? "Binary file (*.dat)|*.dat" :
               (isCppFormat ? "C++ Source file (*.cpp)|*.cpp" : "Text file (*.txt|*.txt)");

            _exportDialog.DefaultExt = ExportOption == ExportOption.Raw ? "dat" :
                (isCppFormat ? "cpp" : "txt");

            if (_exportDialog.ShowDialog() != true)
                return;

            Container.WriteCurrentContentToFile(_exportDialog.FileName, !onlyExportCurrent, ExportOption);
        }

        private void ScanNetwork()
        {
            IsScanningNetwork = true;
            Task.Run(() => UdpListener.StartUdpBroadcastListen(UdpListenPort));
        }

        private void ConnectOrDisconnect()
        {
            Task.Run(() =>
            {
                if (Connection.Status != ConnectionStatus.Connected)
                    Connection.TryConnect();
                else
                    Connection.Disconnect();
            });
        }

        private void TransmitFrames(bool onlyIncludeCurrent)
        {
            if (Connection.Status != ConnectionStatus.Connected)
                return;
        }
        #endregion
    }
}
