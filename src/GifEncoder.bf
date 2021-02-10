using System;

namespace jazzutils
{
	[CRepr]
	struct GifRgb
	{
		public this(uint8 _r, uint8 _g, uint8 _b)
		{
			r = _r;
			g = _g;
			b = _b;
		}

		public uint8 r, g, b;
	}

	class GifEncoder
	{
		private ge_GIF* handle;

		public uint8* Frame => handle != null ? handle.frame : null;

		public this(StringView fileName, int width, int height, Span<GifRgb> palette, int depth, int loop)
		{
			handle = ge_new_gif(fileName.Ptr, (.)width, (.)height, &palette[0].r, depth, loop);
		}

		public ~this()
		{
			if (handle != null)
			{
				ge_close_gif(handle);
				handle = null;
			}
		}

		public void AddFrame(int delay)
		{
			if (handle == null)
			{
				return;
			}
			ge_add_frame(handle, (.)delay);
		}

		[CRepr]
		struct ge_GIF
		{
			public uint16 w, h;
			public int32 depth;
			public int32 fd;
			public int32 offset;
			public int32 nframes;
			public uint8* frame;
			public uint8* back;
			public uint32 partial;
			public uint8[0xff] buffer;
		}

		[CLink]
		private static extern ge_GIF* ge_new_gif(char8* fname, uint16 width, uint16 height, uint8* palette, int depth, int loop);
		[CLink]
		private static extern void ge_add_frame(ge_GIF* gif, uint16 delay);
		[CLink]
		private static extern void ge_close_gif(ge_GIF* gif);
	}
}
