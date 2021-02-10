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
			var gifEncoder = scope GifEncoder("test.gif", w, h, palette, 2, 0);
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
			return true;
		}

		static int Main()
		{
			if (!GifEncodingTest())
			{
				return 1;
			}
			return 0;
		}
	}
}
