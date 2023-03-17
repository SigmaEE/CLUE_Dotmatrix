using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;
using CommunityToolkit.Mvvm.ComponentModel;

namespace DotMatrixAnimationDesigner
{
    internal class DotMatrixContainer : ObservableObject
    {
        #region Public properties

        #region Backing fields
        private int _totalNumberOfFrames = 1;
        private int _selectedFrame;
        private int _gridWidth;
        private int _gridHeight;
        #endregion

        public int TotalNumberOfFrames
        {
            get => _totalNumberOfFrames;
            set => SetProperty(ref _totalNumberOfFrames, value);
        }

        public int SelectedFrame
        {
            get => _selectedFrame;
            set => SetProperty(ref _selectedFrame, value);
        }

        public int GridWidth
        {
            get => _gridWidth;
            set => SetProperty(ref _gridWidth, value);
        }

        public int GridHeight
        {
            get => _gridHeight;
            set => SetProperty(ref _gridHeight, value);
        }

        public ObservableCollection<DotModel> Dots { get; } = new();
        #endregion

        #region Private fields
        private readonly List<List<bool>> _frames = new();
        #endregion

        #region Public methods
        public DotMatrixContainer(int width, int height)
        {
            SetDimensions(width, height);
        }

        public void SetDimensions(int width, int height)
        {
            Dots.Clear();
            for (var i = 0; i < width * height; i++)
                Dots.Add(new());

            _frames.Clear();
            for (var i = 0; i < TotalNumberOfFrames; i++)
            {
                var frame = new List<bool>();
                for (var j = 0; j < width * height; j++)
                    frame.Add(false);
                _frames.Add(frame);
            }

            TotalNumberOfFrames = _frames.Count;
            SelectedFrame = TotalNumberOfFrames;
            GridWidth = width;
            GridHeight = height;
            SetSelectedFrameToActive();
        }

        public void MoveImage(ChangeDirection direction)
        {
            if (direction == ChangeDirection.Left)
            {
                for (var i = 1; i < GridWidth; i++)
                {
                    for (var j = 0; j < GridHeight; j++)
                    {
                        Dots[ToGridIndex(i - 1, j)].IsChecked = Dots[ToGridIndex(i, j)].IsChecked;
                        if (i == GridWidth - 1)
                            Dots[ToGridIndex(i, j)].IsChecked = false;
                    }
                }
            }
            else if (direction == ChangeDirection.Right)
            {
                for (var i = GridWidth - 2; i >= 0; i--)
                {
                    for (var j = 0; j < GridHeight; j++)
                    {
                        Dots[ToGridIndex(i + 1, j)].IsChecked = Dots[ToGridIndex(i, j)].IsChecked;
                        if (i == 0)
                            Dots[ToGridIndex(i, j)].IsChecked = false;
                    }
                }
            }
            else if (direction == ChangeDirection.Up)
            {
                for (var j = 1; j < GridHeight; j++)
                {
                    for (var i = 0; i < GridWidth; i++)
                    {
                        Dots[ToGridIndex(i, j - 1)].IsChecked = Dots[ToGridIndex(i, j)].IsChecked;
                        if (j == GridHeight - 1)
                            Dots[ToGridIndex(i, j)].IsChecked = false;
                    }
                }
            }
            else if (direction == ChangeDirection.Down)
            {
                for (var j = GridHeight - 2; j >= 0; j--)
                {
                    for (var i = 0; i < GridWidth; i++)
                    {
                        Dots[ToGridIndex(i, j + 1)].IsChecked = Dots[ToGridIndex(i, j)].IsChecked;
                        if (j == 0)
                            Dots[ToGridIndex(i, j)].IsChecked = false;
                    }
                }
            }
            CopyActiveToBackingBuffer();
        }

        public void AddNewFrame(bool duplicateCurrent)
        {
            CopyActiveToBackingBuffer();

            var newFrame = new List<bool>();

            TotalNumberOfFrames++;
            for (var i = 0; i < GridWidth * GridHeight; i++)
                newFrame.Add(duplicateCurrent && Dots[i].IsChecked);

            _frames.Add(newFrame);
            SelectedFrame = TotalNumberOfFrames;
            SetSelectedFrameToActive();
        }

        public void ChangeFrame(ChangeDirection direction)
        {
            if (direction == ChangeDirection.Down || direction == ChangeDirection.Up)
                return;

            if (TotalNumberOfFrames == 1)
                return;

            CopyActiveToBackingBuffer();

            if (direction == ChangeDirection.Left)
                SelectedFrame = SelectedFrame == 1 ? TotalNumberOfFrames : SelectedFrame - 1;
            else if (direction == ChangeDirection.Right)
                SelectedFrame = SelectedFrame == TotalNumberOfFrames ? 1 : SelectedFrame + 1;
            SetSelectedFrameToActive();
        }

        public void DeleteFrame()
        {
            if (TotalNumberOfFrames == 1)
                return;

            _frames.RemoveAt(SelectedFrame - 1);
            TotalNumberOfFrames = _frames.Count;
            SelectedFrame = SelectedFrame == 1 ? 1 : SelectedFrame - 1;
            SetSelectedFrameToActive();
        }

        public void ClearCurrentFrame()
        {
            foreach (var d in Dots)
                d.IsChecked = false;
            CopyActiveToBackingBuffer();
        }

        public void ImportFrames(int numberOfFrames, ReadOnlySpan<byte> frameData)
        {
            var numberOfValuesPerRow = (int)Math.Ceiling(GridWidth / 8.0);
            var valuesPerFrame = numberOfValuesPerRow * GridHeight;
            var remainingValuesOnRow = GridWidth;

            for (var i = 0; i < numberOfFrames; i++)
            {
                var newFrame = new List<bool>();
                var frameStart = i * valuesPerFrame;
                foreach (var value in frameData[frameStart..(frameStart + valuesPerFrame)])
                {
                    for (var bitShift = 7; bitShift >= 0; bitShift--)
                    {
                        newFrame.Add(((value >> bitShift) & 0x01) == 1);
                        remainingValuesOnRow--;
                        if (remainingValuesOnRow == 0)
                        {
                            remainingValuesOnRow = GridWidth;
                            break;
                        }
                    }
                }

                if (i > 0)
                    _frames.Add(newFrame);
                else
                    _frames[SelectedFrame - 1] = newFrame;
            }

            TotalNumberOfFrames = _frames.Count;
            SelectedFrame = TotalNumberOfFrames;
            SetSelectedFrameToActive();
        }

        public void WriteCurrentContentToFile(string filePath, bool includeAllFrames, ExportOption exportOption)
        {
            CopyActiveToBackingBuffer();
            var f = File.Create(filePath);

            if (exportOption == ExportOption.CppFunctionCalls || exportOption == ExportOption.Coordinates)
            {
                WriteFrameAsAsRawFunctionCallsOrCoordinates(f, exportOption == ExportOption.CppFunctionCalls);
                f.Close();
                return;
            }

            if (exportOption == ExportOption.RawCpp)
                f.Write(Encoding.ASCII.GetBytes("uint8_t data[] = { "));

            var numberOfFramesInFile = includeAllFrames ? (byte)TotalNumberOfFrames : (byte)1;
            f.Write(exportOption == ExportOption.RawCpp ? Encoding.ASCII.GetBytes($"{GetByteAsText(numberOfFramesInFile)}, ") : new byte[] { numberOfFramesInFile });
            f.Write(exportOption == ExportOption.RawCpp ? Encoding.ASCII.GetBytes($"{GetByteAsText((byte)GridWidth)}, ") : new byte[] { (byte)GridWidth });
            f.Write(exportOption == ExportOption.RawCpp ? Encoding.ASCII.GetBytes($"{GetByteAsText((byte)GridHeight)},\n") : new byte[] { (byte)GridHeight });

            if (includeAllFrames)
            {
                for (var i = 0; i < TotalNumberOfFrames; i++)
                {
                    var frameBytes = GetBytesForFrame(i + 1);
                    if (exportOption == ExportOption.RawCpp)
                    {
                        f.Write(Encoding.ASCII.GetBytes($"// Frame {i}\n"));
                        WriteFrameAsText(f, frameBytes);
                        if (i < TotalNumberOfFrames - 1)
                            f.Write(Encoding.ASCII.GetBytes(",\n"));
                    }
                    else
                    {
                        f.Write(frameBytes);
                    }
                }
            }
            else
            {
                var frameBytes = GetBytesForFrameRowWise(_frames[SelectedFrame - 1]);
                if (exportOption == ExportOption.RawCpp)
                    WriteFrameAsText(f, frameBytes);
                else
                    f.Write(frameBytes);
            }

            if (exportOption == ExportOption.RawCpp)
                f.Write(Encoding.ASCII.GetBytes(" };"));

            f.Close();
        }

        public ReadOnlySpan<byte> GetBytesForFrameAsCoordinates(int frameNumber)
        {
            CopyActiveToBackingBuffer();
            List<(int x, int y)> setPixels = new();
            for (var i = 0; i < GridHeight; i++)
            {
                for (var j = 0; j < GridWidth; j++)
                {
                    if (_frames[frameNumber - 1][ToGridIndex(j, i)])
                        setPixels.Add((i, j));
                }
            }

            var buffer = new byte[setPixels.Count * 2];
            for (var i = 0; i < setPixels.Count; i++)
            {
                buffer[i * 2] = (byte)setPixels[i].x;
                buffer[(i * 2) + 1] = (byte)setPixels[i].y;
            }
            return buffer;

        }

        public ReadOnlySpan<byte> GetBytesForFrame(int frameNumber)
        {
            CopyActiveToBackingBuffer();
            return GetBytesForFrameRowWise(_frames[frameNumber - 1]);
        }
        #endregion

        #region Private methods
        private void WriteFrameAsAsRawFunctionCallsOrCoordinates(FileStream f, bool asFunctionCalls)
        {
            var currentFrame = _frames[SelectedFrame - 1];
            var hasRowAbove = false;
            for (var i = 0; i < GridHeight; i++)
            {
                for (var j = 0; j < GridWidth; j++)
                {
                    if (currentFrame[ToGridIndex(j, i)])
                    {
                        if (hasRowAbove)
                            f.Write(Encoding.ASCII.GetBytes("\n"));
                        f.Write(Encoding.ASCII.GetBytes(
                            asFunctionCalls ? GetPixelAsFunctionCall(i, j) : GetPixelAsCoordinate(i, j)));
                        hasRowAbove = true;
                    }
                }
            }
        }

        private ReadOnlySpan<byte> GetBytesForFrameRowWise(List<bool> dots)
        {
            var numberOfBytesPerRow = (int)Math.Ceiling(GridWidth / 8.0);
            var result = new byte[numberOfBytesPerRow * GridHeight];
            var byteCounter = 0;

            for (var i = 0; i < GridHeight; i++)
            {
                var rowStart = i * GridWidth;
                var rowEnd = rowStart + GridWidth;

                var bitShift = 7;
                byte currentByte = 0;

                for (var j = rowStart; j < rowEnd; j++)
                {
                    currentByte += (byte)((dots[j] ? 0x01 : 0x00) << bitShift);
                    bitShift--;
                    if (bitShift == -1)
                    {
                        result[byteCounter++] = currentByte;
                        currentByte = 0;
                        bitShift = 7;
                    }
                }

                if (bitShift != 7)
                    result[byteCounter++] = currentByte;
            }
            return result;
        }

        private void WriteFrameAsText(FileStream fs, ReadOnlySpan<byte> frameBytes, bool lineBreakEveryRow = true)
        {
            StringBuilder sb = new();
            var numberOfBytesPerRow = lineBreakEveryRow ? (int)Math.Ceiling(GridWidth / 8.0) : 0;
            for (var j = 0; j < frameBytes.Length; j++)
            {
                sb.Append(GetByteAsText(frameBytes[j]));
                if (j < frameBytes.Length - 1)
                {
                    if (lineBreakEveryRow && ((j + 1) % numberOfBytesPerRow == 0))
                        sb.Append(",\n");
                    else
                        sb.Append(", ");
                }
            }

            fs.Write(Encoding.ASCII.GetBytes(sb.ToString()));
        }

        private void SetSelectedFrameToActive()
        {
            for (var i = 0; i < Dots.Count; i++)
                Dots[i].IsChecked = _frames[SelectedFrame - 1][i];
        }

        private void CopyActiveToBackingBuffer()
        {
            for (var i = 0; i < Dots.Count; i++)
                _frames[SelectedFrame - 1][i] = Dots[i].IsChecked;
        }

        private int ToGridIndex(int x, int y)
            => x + (y * GridWidth);

        private static string GetByteAsText(byte b)
            => $"0x{b:x2}";
        private static string GetPixelAsFunctionCall(int x, int y)
            => $"m_screen->set_value_for_pixel({x}, {y}, Screen::PixelValue::CURRENT);";
        private static string GetPixelAsCoordinate(int x, int y)
            => $"({x}, {y})";
        #endregion
    }
}
