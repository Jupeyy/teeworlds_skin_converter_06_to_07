#pragma once

#if defined(_WIN64) || defined(_WIN32)
#define __WINDOWS__
#include "windows.h"
#else
#define __UNIX__
#include <sys/stat.h>
#include <dirent.h>
#endif

#include <string.h>
#include <vector>

static void mem_zero(void* pBuff, size_t BuffSize) {
	memset(pBuff, 0, BuffSize);
}

static const uint8_t s_Utf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static bool utf8_to_utf16(wchar_t *pDst, size_t *pDstSize, const char *pSrc, size_t SrcSize) {
	size_t DestPos = 0, SrcPos = 0;
	for (;;) {
		uint8_t c;
		size_t NumAdds = 0;

		if (SrcPos == SrcSize) {
			if (pDstSize)
				*pDstSize = DestPos;
			return true;
		}

		c = (uint8_t)pSrc[SrcPos++];

		if (c < 0x80) {
			if (pDst)
				pDst[DestPos] = (uint16_t)c;
			DestPos++;
			if (c == 0)
				break;
			else
				continue;
		}

		if (c < 0xC0)
			break;

		for (NumAdds = 1; NumAdds < 5; NumAdds++)
			if (c < s_Utf8Limits[NumAdds])
				break;

		uint32_t Value = (uint32_t)(c - s_Utf8Limits[NumAdds - 1]);

		do {
			uint8_t c2;
			if (SrcPos == SrcSize)
				break;
			c2 = (uint8_t)pSrc[SrcPos++];
			if (c2 < 0x80 || c2 >= 0xC0)
				break;
			Value <<= 6;
			Value |= (uint32_t)(c2 - 0x80);
		}
		while (--NumAdds != 0);

		if (Value < 0x10000) {
			if (pDst)
				pDst[DestPos] = (uint16_t)Value;
			DestPos++;
		}
		else {
			Value -= 0x10000;
			if (Value >= 0x100000)
				break;
			if (pDst) {
				pDst[DestPos + 0] = (uint16_t)(0xD800 + (Value >> 10));
				pDst[DestPos + 1] = (uint16_t)(0xDC00 + (Value & 0x3FF));
			}
			DestPos += 2;
		}
	}

	if (pDstSize)
		*pDstSize = DestPos;

	return false;
}

struct directory_item_info {
	directory_item_info() {}
	
	union dir_items {		
		struct item_info {
			char name[256];
		} item;

		struct item_file_info : public item_info {
			uint64_t size;
			time_t creation_time;
		} file;

		struct item_directory_info : public item_info {
		} dir;
	} item;

	bool is_dir;
};

static void directory_items(const char* pPath, std::vector<directory_item_info>& Files, const char* pFilter) {
	char Slash = '/';
	std::string Path = pPath;
	if (Path.empty())
		return;

	if (Path[Path.size() - 1] != Slash)
		Path.append(1, Slash);
#ifdef __WINDOWS__
	HANDLE hFile;
	WIN32_FIND_DATA FileData;

	if (pFilter)
		Path.append(pFilter);
	else
		Path.append(1, '*');

	hFile = FindFirstFileA(Path.c_str(), &FileData);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			//TODO apply filter
			directory_item_info dir_info;
			strncpy(dir_info.item.item.name, FileData.cFileName, sizeof(dir_info.item.item.name));
			if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				dir_info.is_dir = true;
			}
			else {
				SYSTEMTIME Systime;
				FileTimeToSystemTime(&FileData.ftCreationTime, &Systime);

				tm t;
				mem_zero(&t, sizeof(tm));
				t.tm_year = (int)Systime.wYear - 1900;
				t.tm_mon = (int)Systime.wMonth - 1;
				t.tm_wday = (int)Systime.wDayOfWeek;
				t.tm_mday = (int)Systime.wDay;
				t.tm_hour = (int)Systime.wHour;
				t.tm_min = (int)Systime.wMinute;
				t.tm_sec = (int)Systime.wSecond;

				dir_info.item.file.creation_time = mktime(&t);

				ULARGE_INTEGER ul;
				ul.HighPart = FileData.nFileSizeHigh;
				ul.LowPart = FileData.nFileSizeLow;

				dir_info.item.file.size = (uint64_t)ul.QuadPart;

				dir_info.is_dir = false;
			}
			Files.push_back(dir_info);
		}
		while (FindNextFileA(hFile, &FileData));
		FindClose(hFile);
	}
#elif defined(__UNIX__)
	DIR* pDir = opendir(pPath);
	if(pDir) {
		dirent* pDirItem = NULL;
		while((pDirItem = readdir(pDir)) != NULL) {
			//TODO apply filter

			directory_item_info dir_info;
			if(pDirItem->d_type == DT_DIR) {
				strncpy(dir_info.item.dir.name, pDirItem->d_name, sizeof(dir_info.item.dir.name) / sizeof(dir_info.item.dir.name[0]));
				dir_info.is_dir = true;
			}
			else if(pDirItem->d_type == DT_REG) {
				dir_info.is_dir = false;
				strncpy(dir_info.item.file.name, pDirItem->d_name, sizeof(dir_info.item.file.name) / sizeof(dir_info.item.file.name[0]));
				
				dir_info.item.file.creation_time = 0;
				dir_info.item.file.size = pDirItem->d_reclen;
			}
			Files.push_back(dir_info);
		}

		closedir(pDir);
	}
#else
#error not implemented
#endif
}

static void create_path(const char* pPath) {
#ifdef __WINDOWS__
	wchar_t WPathStr[4096];

	unsigned int i = 0;
	unsigned int CurPathOffset = 0;
	while (pPath[i] != '\0') {
		if (pPath[i] == '\\' || pPath[i] == '/') {
			utf8_to_utf16(WPathStr, NULL, pPath, i);
			WPathStr[i] = L'\0';
			CreateDirectoryW(WPathStr, NULL);

			CurPathOffset = i + 1;
		}
		++i;
	}

	if (i - CurPathOffset > 0) {
		utf8_to_utf16(WPathStr, NULL, pPath, i);
		WPathStr[i] = L'\0';
		CreateDirectoryW(WPathStr, NULL);
	}
#elif defined(__UNIX__)
	char PathStr[4096];

	unsigned int i = 0;
	unsigned int CurPathOffset = 0;
	while (pPath[i] != '\0') {
		if (pPath[i] == '/') {
			strncpy(PathStr, pPath, sizeof(PathStr) / sizeof(PathStr[0]));
			PathStr[i] = '\0';
			mkdir(PathStr, S_IRUSR | S_IWUSR | S_IXUSR);

			CurPathOffset = i + 1;
		}
		++i;
	}

	if (i - CurPathOffset > 0) {
		strncpy(PathStr, pPath, sizeof(PathStr) / sizeof(PathStr[0]));
		PathStr[i] = '\0';
		mkdir(PathStr, S_IRUSR | S_IWUSR | S_IXUSR);
	}
#else
#error not implemented
#endif
}

static void remove_directories_from_path(char* pNewPathBuff, size_t NewPathBuffCharCount, const char* pPath) {
	size_t i = 0;
	size_t CurStringOffset = 0;
	while (pPath[i] != '\0') {
		if(pPath[i] == '/' || pPath[i] == '\\') {
			CurStringOffset = i + 1;
		}
		++i;
	}

	const char* pNewPath = &pPath[CurStringOffset];
	size_t NewPathLen = strlen(pNewPath);

	if(NewPathLen > 0) {
		strncpy(pNewPathBuff, pNewPath, NewPathBuffCharCount);
	}
	else {
		pNewPathBuff[0] = 0;
	}
}
