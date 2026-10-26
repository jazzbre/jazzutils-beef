using System;

namespace jazzutils
{
	static class Zstd
	{
		public const int32 DefaultLevel = 3;
		public const int32 MaximumLevel = 22;

		[CLink] private static extern uint ZSTD_compressBound(uint sourceSize);
		[CLink] private static extern uint ZSTD_compress(void* destination, uint destinationCapacity, void* source, uint sourceSize, int32 level);
		[CLink] private static extern uint ZSTD_decompress(void* destination, uint destinationCapacity, void* source, uint compressedSize);
		[CLink] private static extern uint32 ZSTD_isError(uint code);
		[CLink] private static extern uint64 ZSTD_getFrameContentSize(void* source, uint sourceSize);

		public static int GetCompressBound(int sourceSize)
		{
			return (int)ZSTD_compressBound((uint)sourceSize);
		}

		/// Returns the compressed size, or -1 when the destination is too small.
		public static int Compress(void* source, int sourceSize, void* destination, int destinationCapacity, int32 level = DefaultLevel)
		{
			let result = ZSTD_compress(destination, (uint)destinationCapacity, source, (uint)sourceSize, level);
			return ZSTD_isError(result) != 0 ? -1 : (int)result;
		}

		/// Returns the decompressed size, or -1 when the data is corrupt or the destination is too small.
		public static int Decompress(void* source, int compressedSize, void* destination, int destinationCapacity)
		{
			let result = ZSTD_decompress(destination, (uint)destinationCapacity, source, (uint)compressedSize);
			return ZSTD_isError(result) != 0 ? -1 : (int)result;
		}

		/// The size stored in the frame header, or -1 when it is unknown or the frame is invalid.
		public static int64 GetDecompressedSize(void* source, int compressedSize)
		{
			let size = ZSTD_getFrameContentSize(source, (uint)compressedSize);
			return size >= 0xFFFFFFFFFFFFFFFEUL ? -1 : (int64)size;
		}
	}
}
