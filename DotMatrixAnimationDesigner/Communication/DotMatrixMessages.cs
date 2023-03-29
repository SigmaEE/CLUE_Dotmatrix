using System;
using System.Collections.Generic;

namespace DotMatrixAnimationDesigner.Communication
{
    public enum MessageType
    {
        ConnectionAlivePing,
        AnimationFrames,
        GameOfLifeConfig,
        CommandMessage
    }

    public enum DotMatrixCommand
    {
        SetClockMode = 0x01,
        SetSecondAnimationActive = 0x02,
        SetSecondAnimationInactive = 0x03,
        ScrollDate = 0x04
    }

    internal abstract class DotMatrixMessage
    {
        private const ushort Crc16Polynomial = 0x1021;

        public abstract byte Identifier { get; }

        public abstract MessageType Type { get; }

        public abstract ushort NumberOfPackets { get; }

        protected const int CommonHeaderLength = 4;

        public ReadOnlySpan<byte> GetTransmissionHeader()
            => new byte[]
            {
                Identifier,
                GetLsb(NumberOfPackets),
                GetMsb(NumberOfPackets),
            };

        public abstract ReadOnlySpan<byte> GetPacket(int index = 0);

        protected static byte[] GetCommonHeader(ReadOnlySpan<byte> payload, ushort packetLength)
        {
            var header = new byte[CommonHeaderLength];
            var crc = ComputeCrc16Checksum(payload);
            header[0] = GetLsb(crc);
            header[1] = GetMsb(crc);
            header[2] = GetLsb(packetLength);
            header[3] = GetMsb(packetLength);
            return header;
        }

        protected static byte GetMsb(ushort u)
            => (byte)((u & 0xff00) >> 8);

        protected static byte GetLsb(ushort u)
            => (byte)(u & 0x00ff);

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
    }

    internal class ConnectionAlivePingMessage : DotMatrixMessage
    {
        public override byte Identifier => 0xff;

        public override MessageType Type => MessageType.ConnectionAlivePing;

        public override ushort NumberOfPackets => 0;

        public override ReadOnlySpan<byte> GetPacket(int index = 0) => throw new NotImplementedException();
    }

    internal class AnimationFramesMessage : DotMatrixMessage
    {
        public override byte Identifier => 0x10;

        public override MessageType Type => MessageType.AnimationFrames;

        public override ushort NumberOfPackets { get; }

        private readonly List<(ushort frameTime, byte[] frameData)> _packetPayloads;
        private readonly ushort _numberOfBytesPerFrame;
        private readonly byte _numberOfAnimationRepeats;
        private readonly byte _numberOfRows;
        private readonly byte _numberOfColumns;
        private const int HeaderLength = 9;

        private AnimationFramesMessage(List<(ushort frameTime, byte[] frameData)> payloads, int numberOfAnimationRepeats, int numberOfFramesToTransmit, int numberOfBytesPerFrame, int numberOfRows, int numberOfColumns)
        {
            _packetPayloads = payloads;
            NumberOfPackets = (ushort)numberOfFramesToTransmit;
            _numberOfBytesPerFrame = (ushort)numberOfBytesPerFrame;
            _numberOfAnimationRepeats = (byte)numberOfAnimationRepeats;
            _numberOfRows = (byte)numberOfRows;
            _numberOfColumns = (byte)numberOfColumns;
        }

        public override ReadOnlySpan<byte> GetPacket(int index = 0)
        {
            var packet = new byte[CommonHeaderLength + HeaderLength + _numberOfBytesPerFrame];
            var idx = CommonHeaderLength;
            packet[idx++] = GetLsb((ushort)index);
            packet[idx++] = GetMsb((ushort)index);
            packet[idx++] = GetLsb(_numberOfBytesPerFrame);
            packet[idx++] = GetMsb(_numberOfBytesPerFrame);
            packet[idx++] = _numberOfRows;
            packet[idx++] = _numberOfColumns;
            packet[idx++] = GetLsb(_packetPayloads[index].frameTime);
            packet[idx++] = GetMsb(_packetPayloads[index].frameTime);
            packet[idx++] = _numberOfAnimationRepeats;

            Buffer.BlockCopy(_packetPayloads[index].frameData, 0, packet, idx, _numberOfBytesPerFrame);
            var commonHeader = GetCommonHeader(packet.AsSpan()[CommonHeaderLength..], (ushort)(_numberOfBytesPerFrame + HeaderLength));
            Buffer.BlockCopy(commonHeader, 0, packet, 0, CommonHeaderLength);
            return packet;
        }

        public static AnimationFramesMessage Get(DotMatrixContainer container, bool transmitOnlyCurrent)
        {
            var startFrameNumber = transmitOnlyCurrent ? container.SelectedFrame : 1;
            var endFrameNumber = transmitOnlyCurrent ? container.SelectedFrame : container.TotalNumberOfFrames;

            List<(ushort frameTime, byte[] frameData)> packetPayloads = new();
            for (var i = startFrameNumber; i <= endFrameNumber; i++)
                packetPayloads.Add((container.GetFrameLengthForFrame(i), container.GetBytesForFrame(i).ToArray()));

            return new(packetPayloads, container.NumberOfAnimationRepeats, packetPayloads.Count, packetPayloads[0].frameData.Length, container.GridHeight, container.GridWidth);
        }
    }

    internal class GameOfLifeConfigMesssage : DotMatrixMessage
    {
        public override byte Identifier => 0x11;

        public override MessageType Type => MessageType.GameOfLifeConfig;

        public override ushort NumberOfPackets => 1;

        private readonly byte[] _dataPacket;
        private readonly ushort _timeBetweenTicks;

        private GameOfLifeConfigMesssage(ReadOnlySpan<byte> dataPacket, ushort timeBetweenTicks)
        {
            _dataPacket = dataPacket.ToArray();
            _timeBetweenTicks = timeBetweenTicks;
        }

        public override ReadOnlySpan<byte> GetPacket(int index = 0)
        {
            var packet = new byte[CommonHeaderLength + 2 + _dataPacket.Length];
            packet[CommonHeaderLength] = GetLsb(_timeBetweenTicks);
            packet[CommonHeaderLength + 1] = GetMsb(_timeBetweenTicks);
            Buffer.BlockCopy(_dataPacket, 0, packet, CommonHeaderLength + 2, _dataPacket.Length);
            var commonHeader = GetCommonHeader(packet.AsSpan()[CommonHeaderLength..], (ushort)(2 + _dataPacket.Length));
            Buffer.BlockCopy(commonHeader, 0, packet, 0, CommonHeaderLength);
            return packet;
        }

        public static GameOfLifeConfigMesssage Get(DotMatrixContainer container)
            => new(container.GetBytesForFrameAsCoordinates(container.SelectedFrame), container.GetFrameLengthForFrame(container.SelectedFrame));
    }

    internal class CommandMessage : DotMatrixMessage
    {
        public override byte Identifier => 0x12;

        public override MessageType Type => MessageType.CommandMessage;

        public override ushort NumberOfPackets => 1;

        private readonly byte _commandValue;

        private CommandMessage(byte commandValue)
        {
            _commandValue = commandValue;
        }

        public static CommandMessage Get(DotMatrixCommand cmd)
            => new((byte)cmd);

        public override ReadOnlySpan<byte> GetPacket(int index = 0)
        {
            var packet = new byte[CommonHeaderLength + 1];
            packet[CommonHeaderLength] = _commandValue;
            var commonHeader = GetCommonHeader(new byte[] { _commandValue }, 1);
            Buffer.BlockCopy(commonHeader, 0, packet, 0, CommonHeaderLength);
            return packet;
        }
    }
}
