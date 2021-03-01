using System;
using jazzutils;

namespace example
{
	class Program
	{
		static bool GifEncodingTest()
		{
			int w = 120, h = 90;
			var palette = GifRgb[4](
				.(0x00, 0x00, 0x00),/* 0 -> black */
				.(0xFF, 0x00, 0x00),/* 1 -> red */
				.(0x00, 0xFF, 0x00),/* 2 -> green */
				.(0x00, 0x00, 0xFF)/* 3 -> blue */
				);
			var gifEncoder = scope GifEncoder("test.gif", w, h, 2, 0);
			if (gifEncoder.Frame == null)
			{
				return false;
			}
			/* draw some frames */
			for (var i = 0; i < 4 * 6 / 3; i++)
			{
				for (var j = 0; j < w * h; j++)
				{
					gifEncoder.Frame[j] = (uint8)((i * 3 + j) / 6 % 4);
				}
				gifEncoder.AddFrame(10);
			}
			// Fill palette
			for (int i = 0; i < palette.Count; ++i)
			{
				gifEncoder.Palette[i] = palette[i];
			}
			return true;
		}

		static bool ZipArchiveTest()
		{
			var zipArchive = scope ZipArchive();
			if (!zipArchive.Open("example.zip"))
			{
				return false;
			}
			var count = zipArchive.FileCount;
			var fileInfo = scope ZipArchiveFileInfo();
			for (int i = 0; i < count; ++i)
			{
				if (!zipArchive.GetFileInfo(i, fileInfo))
				{
					return false;
				}
				Console.WriteLine(scope $"File {i}/{count} '{fileInfo.fileName}' Size:{fileInfo.uncompressedSize} Compressed Size:{fileInfo.compressedSize}");
			}
			var index = zipArchive.FindFile("example/src/Program.bf");
			if (!zipArchive.GetFileInfo(index, fileInfo))
			{
				return false;
			}
			var data = scope uint8[fileInfo.uncompressedSize];
			var buffer = scope uint8[0xffff];
			var result = zipArchive.ReadFile(index, data.CArray(), fileInfo.uncompressedSize, buffer.CArray(), (uint)buffer.Count);
			if (!result)
			{
				return false;
			}
			System.IO.File.WriteAll("test.out", data);
			return true;
		}

		static int Main()
		{
			if (!GifEncodingTest())
			{
				Console.WriteLine("GifEncodingTest failed!");
				return 1;
			}
			if (!ZipArchiveTest())
			{
				Console.WriteLine("ZipArchiveTest failed!");
				return 1;
			}
			return 0;
		}
	}
}
