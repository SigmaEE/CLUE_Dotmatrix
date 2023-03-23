using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using CommunityToolkit.Mvvm.ComponentModel;

namespace DotMatrixAnimationDesigner
{
    internal class DotMatrixContainer : ObservableObject
    {
        #region Public properties

        #region Backing fields
        private int _totalNumberOfFrames = 1;
        private int _numberOfAnimationRepeats = 0;
        private int _selectedFrame;
        private int _selectedFrameTime = 0;
        private int _gridWidth;
        private int _gridHeight;
        #endregion

        public int TotalNumberOfFrames
        {
            get => _totalNumberOfFrames;
            set => SetProperty(ref _totalNumberOfFrames, value);
        }

        public int NumberOfAnimationRepeats
        {
            get => _numberOfAnimationRepeats;
            set => SetProperty(ref _numberOfAnimationRepeats, value);
        }

        public int SelectedFrame
        {
            get => _selectedFrame;
            set => SetProperty(ref _selectedFrame, value);
        }

        public int SelectedFrameTime
        {
            get => _selectedFrameTime;
            set
            {
                _frames[SelectedFrame - 1] = (value, _frames[SelectedFrame - 1].pixels);
                SetProperty(ref _selectedFrameTime, value);
            }
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
        private readonly List<(int frameTime, List<bool> pixels)> _frames = new();
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
                _frames.Add((0, frame));
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

            _frames.Add((duplicateCurrent ? SelectedFrameTime : 0, newFrame));
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
            var valuesPerFrame = 2 + (numberOfValuesPerRow * GridHeight);
            var remainingValuesOnRow = GridWidth;

            for (var i = 0; i < numberOfFrames; i++)
            {
                var newFramePixels = new List<bool>();
                var frameStart = i * valuesPerFrame;
                var newFrameDuration = frameData[frameStart + 1] << 8 | frameData[frameStart];
                frameStart += 2;
                foreach (var value in frameData[frameStart..(frameStart + valuesPerFrame - 2)])
                {
                    for (var bitShift = 7; bitShift >= 0; bitShift--)
                    {
                        newFramePixels.Add(((value >> bitShift) & 0x01) == 1);
                        remainingValuesOnRow--;
                        if (remainingValuesOnRow == 0)
                        {
                            remainingValuesOnRow = GridWidth;
                            break;
                        }
                    }
                }

                if (i > 0)
                    _frames.Add((newFrameDuration, newFramePixels));
                else
                    _frames[SelectedFrame - 1] = (newFrameDuration, newFramePixels);
            }

            TotalNumberOfFrames = _frames.Count;
            SelectedFrame = TotalNumberOfFrames;
            SetSelectedFrameToActive();
        }

        public void WriteCurrentContentToFile(string filePath, bool includeAllFrames)
        {
            CopyActiveToBackingBuffer();
            var f = File.Create(filePath);

            var numberOfFramesInFile = includeAllFrames ? (byte)TotalNumberOfFrames : (byte)1;
            f.Write(new byte[] { numberOfFramesInFile });
            f.Write(new byte[] { (byte)GridWidth });
            f.Write(new byte[] { (byte)GridHeight });

            if (includeAllFrames)
            {
                for (var i = 0; i < TotalNumberOfFrames; i++)
                {
                    var frameTime = GetFrameLengthForFrame(i + 1);
                    f.Write(BitConverter.GetBytes(frameTime));
                    var frameBytes = GetBytesForFrame(i + 1);
                    f.Write(frameBytes);
                }
            }
            else
            {
                var frameTime = GetFrameLengthForFrame(SelectedFrame);
                f.Write(BitConverter.GetBytes(frameTime));
                var frameBytes = GetBytesForFrame(SelectedFrame);
                f.Write(frameBytes);
            }
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
                    if (_frames[frameNumber - 1].pixels[ToGridIndex(j, i)])
                        setPixels.Add((j, i));
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

        public ushort GetFrameLengthForFrame(int frameNumber)
        {
            CopyActiveToBackingBuffer();
            return (ushort)_frames[frameNumber - 1].frameTime;
        }

        public ReadOnlySpan<byte> GetBytesForFrame(int frameNumber)
        {
            CopyActiveToBackingBuffer();
            return GetBytesForFrameRowWise(_frames[frameNumber - 1].pixels);
        }
        #endregion

        #region Private methods
        private byte[] GetBytesForFrameRowWise(List<bool> dots)
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

        private void SetSelectedFrameToActive()
        {
            for (var i = 0; i < Dots.Count; i++)
                Dots[i].IsChecked = _frames[SelectedFrame - 1].pixels[i];
            SelectedFrameTime = _frames[SelectedFrame - 1].frameTime;
        }

        private void CopyActiveToBackingBuffer()
        {
            for (var i = 0; i < Dots.Count; i++)
                _frames[SelectedFrame - 1].pixels[i] = Dots[i].IsChecked;
        }

        private int ToGridIndex(int x, int y)
            => x + (y * GridWidth);
        #endregion
    }
}
