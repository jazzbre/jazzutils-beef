using System;

namespace jazzutils
{
	public class ZipArchiveFileInfo
	{
		public String fileName = new String() ~ delete _;
		public uint64 compressedSize;
		public uint64 uncompressedSize;
	}

	class ZipArchive
	{
		public bool IsOpen { get; private set; }
		public int FileCount => (int)mz_zip_reader_get_num_files(&zipArchive);

		var zipArchive = mz_zip_archive();

		public mz_alloc_func allocFunction;
		public mz_free_func freeFunction;
		public mz_realloc_func reallocFunction;

		public mz_file_read_func readFunction;
		public mz_file_write_func writeFunction;
		public mz_file_needs_keepalive keepAliveFunction;

		/* Note: These enums can be reduced as needed to save memory or stack space - they are pretty conservative. */
		public const int MZ_ZIP_MAX_IO_BUF_SIZE = 64 * 1024;
		public const int MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE = 512;
		public const int MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE = 512;

		public typealias mz_uint16 = uint16;
		public typealias mz_uint = uint32;
		public typealias mz_uint32 = uint32;
		public typealias mz_uint64 = uint64;
		public typealias mz_bool = int32;

		public typealias mz_alloc_func = function void*(void* opaque, uint items, uint size);
		public typealias mz_free_func = function void(void* opaque, void* address);
		public typealias mz_realloc_func = function void*(void* opaque, void* address, uint items, uint size);

		public typealias mz_file_read_func = function uint(void* pOpaque, mz_uint64 file_ofs, void* pBuf, uint n);
		public typealias mz_file_write_func = function uint(void* pOpaque, mz_uint64 file_ofs, void* pBuf, uint n);
		public typealias mz_file_needs_keepalive = function mz_bool(void* pOpaque);

		public enum mz_zip_mode : int32
		{
			MZ_ZIP_MODE_INVALID = 0,
			MZ_ZIP_MODE_READING = 1,
			MZ_ZIP_MODE_WRITING = 2,
			MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3
		}

		public enum mz_zip_flags : int32
		{
			MZ_ZIP_FLAG_CASE_SENSITIVE = 0x0100,
			MZ_ZIP_FLAG_IGNORE_PATH = 0x0200,
			MZ_ZIP_FLAG_COMPRESSED_DATA = 0x0400,
			MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY = 0x0800,
			MZ_ZIP_FLAG_VALIDATE_LOCATE_FILE_FLAG = 0x1000,/* if enabled, mz_zip_reader_locate_file() will be called on
			each file as its validated to ensure the func finds the file in the central dir (intended for testing) */
			MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY = 0x2000,/* validate the local headers, but don't decompress the entire
			file and check the crc32 */
			MZ_ZIP_FLAG_WRITE_ZIP64 = 0x4000,/* always use the zip64 file format, instead of the original zip file
			format with automatic switch to zip64. Use as flags parameter with mz_zip_writer_init*_v2 */
			MZ_ZIP_FLAG_WRITE_ALLOW_READING = 0x8000,
			MZ_ZIP_FLAG_ASCII_FILENAME = 0x10000
		}

		public enum mz_zip_type : int32
		{
			MZ_ZIP_TYPE_INVALID = 0,
			MZ_ZIP_TYPE_USER,
			MZ_ZIP_TYPE_MEMORY,
			MZ_ZIP_TYPE_HEAP,
			MZ_ZIP_TYPE_FILE,
			MZ_ZIP_TYPE_CFILE,
			MZ_ZIP_TOTAL_TYPES
		}

		/* miniz error codes. Be sure to update mz_zip_get_error_string() if you add or modify this enum. */
		public enum mz_zip_error : int32
		{
			MZ_ZIP_NO_ERROR = 0,
			MZ_ZIP_UNDEFINED_ERROR,
			MZ_ZIP_TOO_MANY_FILES,
			MZ_ZIP_FILE_TOO_LARGE,
			MZ_ZIP_UNSUPPORTED_METHOD,
			MZ_ZIP_UNSUPPORTED_ENCRYPTION,
			MZ_ZIP_UNSUPPORTED_FEATURE,
			MZ_ZIP_FAILED_FINDING_CENTRAL_DIR,
			MZ_ZIP_NOT_AN_ARCHIVE,
			MZ_ZIP_INVALID_HEADER_OR_CORRUPTED,
			MZ_ZIP_UNSUPPORTED_MULTIDISK,
			MZ_ZIP_DECOMPRESSION_FAILED,
			MZ_ZIP_COMPRESSION_FAILED,
			MZ_ZIP_UNEXPECTED_DECOMPRESSED_SIZE,
			MZ_ZIP_CRC_CHECK_FAILED,
			MZ_ZIP_UNSUPPORTED_CDIR_SIZE,
			MZ_ZIP_ALLOC_FAILED,
			MZ_ZIP_FILE_OPEN_FAILED,
			MZ_ZIP_FILE_CREATE_FAILED,
			MZ_ZIP_FILE_WRITE_FAILED,
			MZ_ZIP_FILE_READ_FAILED,
			MZ_ZIP_FILE_CLOSE_FAILED,
			MZ_ZIP_FILE_SEEK_FAILED,
			MZ_ZIP_FILE_STAT_FAILED,
			MZ_ZIP_INVALID_PARAMETER,
			MZ_ZIP_INVALID_FILENAME,
			MZ_ZIP_BUF_TOO_SMALL,
			MZ_ZIP_INTERNAL_ERROR,
			MZ_ZIP_FILE_NOT_FOUND,
			MZ_ZIP_ARCHIVE_TOO_LARGE,
			MZ_ZIP_VALIDATION_FAILED,
			MZ_ZIP_WRITE_CALLBACK_FAILED,
			MZ_ZIP_TOTAL_ERRORS
		}

		public ~this()
		{
			Close();
		}

		public bool Open(uint size)
		{
			if (IsOpen)
			{
				return false;
			}
			var result = mz_zip_reader_init(&zipArchive, size, 0);
			if (result != 0)
			{
				IsOpen = true;
			}
			return IsOpen;
		}

		public bool Open(void* memory, uint size)
		{
			if (IsOpen)
			{
				return false;
			}
			var result = mz_zip_reader_init_mem(&zipArchive, memory, size, 0);
			if (result != 0)
			{
				IsOpen = true;
			}
			return IsOpen;
		}

		public bool Open(StringView fileName)
		{
			if (IsOpen)
			{
				return false;
			}
			var result = mz_zip_reader_init_file(&zipArchive, fileName.Ptr, 0);
			if (result != 0)
			{
				IsOpen = true;
			}
			return IsOpen;
		}

		public bool Open(StringView fileName, uint64 fileOffset, uint64 fileSize)
		{
			if (IsOpen)
			{
				return false;
			}
			var result = mz_zip_reader_init_file_v2(&zipArchive, fileName.Ptr, 0, fileOffset, fileSize);
			if (result != 0)
			{
				IsOpen = true;
			}
			return IsOpen;
		}

		public void Close()
		{
			if (!IsOpen)
			{
				return;
			}
			mz_zip_reader_end(&zipArchive);
			IsOpen = false;
		}

		public int FindFile(StringView fileName)
		{
			return mz_zip_reader_locate_file(&zipArchive, fileName.Ptr, null, 0);
		}

		public bool GetFileInfo(int index, ZipArchiveFileInfo fileInfo)
		{
			var fileStat = mz_zip_archive_file_stat();
			if (mz_zip_reader_file_stat(&zipArchive, (uint32)index, &fileStat) == 0)
			{
				return false;
			}
			fileInfo.fileName.Set(StringView(&fileStat.m_filename[0], Internal.CStrLen(&fileStat.m_filename[0])));
			fileInfo.compressedSize = fileStat.m_comp_size;
			fileInfo.uncompressedSize = fileStat.m_uncomp_size;
			return true;
		}

		public bool ReadFile(int index, void* data, uint dataSize, void* buffer, uint bufferSize)
		{
			return mz_zip_reader_extract_to_mem_no_alloc(&zipArchive, (uint32)index, data, dataSize, 0, buffer, bufferSize) != 0;
		}

		[CRepr]
		struct mz_zip_archive_file_stat
		{
			/* Central directory file index. */
			public mz_uint32 m_file_index;

			/* Byte offset of this entry in the archive's central directory. Note we currently only support up to
			UINT_MAX or less bytes in the central dir. */
			public mz_uint64 m_central_dir_ofs;

			/* These fields are copied directly from the zip's central dir. */
			public mz_uint16 m_version_made_by;
			public mz_uint16 m_version_needed;
			public mz_uint16 m_bit_flag;
			public mz_uint16 m_method;

			/* CRC-32 of uncompressed data. */
			public mz_uint32 m_crc32;

			/* File's compressed size. */
			public mz_uint64 m_comp_size;

			/* File's uncompressed size. Note, I've seen some old archives where directory entries had 512 bytes for
			their uncompressed sizes, but when you try to unpack them you actually get 0 bytes. */
			public mz_uint64 m_uncomp_size;

			/* Zip internal and external file attributes. */
			public mz_uint16 m_internal_attr;
			public mz_uint32 m_external_attr;

			/* Entry's local header file offset in bytes. */
			public mz_uint64 m_local_header_ofs;

			/* Size of comment in bytes. */
			public mz_uint32 m_comment_size;

			/* MZ_TRUE if the entry appears to be a directory. */
			public mz_bool m_is_directory;

			/* MZ_TRUE if the entry uses encryption/strong encryption (which miniz_zip doesn't support) */
			public mz_bool m_is_encrypted;

			/* MZ_TRUE if the file is not encrypted, a patch file, and if it uses a compression method we support. */
			public mz_bool m_is_supported;

			/* Filename. If string ends in '/' it's a subdirectory entry. */
			/* Guaranteed to be zero terminated, may be truncated to fit. */
			public char8[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE] m_filename;

			/* Comment field. */
			/* Guaranteed to be zero terminated, may be truncated to fit. */
			public char8[MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE] m_comment;
		}

		[CRepr]
		struct mz_zip_archive
		{
			public mz_uint64 m_archive_size;
			public mz_uint64 m_central_directory_file_ofs;

			/* We only support up to UINT32_MAX files in zip64 mode. */
			public mz_uint32 m_total_files;
			public mz_zip_mode m_zip_mode;
			public mz_zip_type m_zip_type;
			public mz_zip_error m_last_error;

			public mz_uint64 m_file_offset_alignment;

			public mz_alloc_func m_pAlloc;
			public mz_free_func m_pFree;
			public mz_realloc_func m_pRealloc;
			public void* m_pAlloc_opaque;

			public mz_file_read_func m_pRead;
			public mz_file_write_func m_pWrite;
			public mz_file_needs_keepalive m_pNeeds_keepalive;
			public void* m_pIO_opaque;

			public void* m_pState;
		}

		[CLink] private static extern mz_bool mz_zip_reader_init(mz_zip_archive* pZip, mz_uint64 size, mz_uint flags);
		[CLink] private static extern mz_bool mz_zip_reader_init_mem(mz_zip_archive* pZip, void* pMem, uint size, mz_uint flags);
		[CLink] private static extern mz_bool mz_zip_reader_init_file(mz_zip_archive* pZip, char8* pFilename, mz_uint32 flags);
		[CLink] private static extern mz_bool mz_zip_reader_init_file_v2(mz_zip_archive* pZip, char8* pFilename, mz_uint flags, mz_uint64 file_start_ofs, mz_uint64 archive_size);
		[CLink] private static extern mz_bool mz_zip_reader_end(mz_zip_archive* pZip);
		[CLink] private static extern mz_uint mz_zip_reader_get_num_files(mz_zip_archive* pZip);
		[CLink] private static extern int32 mz_zip_reader_locate_file(mz_zip_archive* pZip, char8* pName, char8* pComment, mz_uint flags);
		[CLink] private static extern mz_bool mz_zip_reader_file_stat(mz_zip_archive* pZip, mz_uint file_index, mz_zip_archive_file_stat* pStat);
		[CLink] private static extern mz_bool mz_zip_reader_extract_to_mem_no_alloc(mz_zip_archive* pZip, mz_uint file_index, void* pBuf, uint buf_size, mz_uint flags, void* pUser_read_buf, uint user_read_buf_size);
		[CLink] private static extern mz_bool mz_zip_reader_extract_file_to_mem_no_alloc(mz_zip_archive* pZip, char8* pFilename, void* pBuf, uint buf_size, mz_uint flags, void* pUser_read_buf, uint user_read_buf_size);
	}
}
