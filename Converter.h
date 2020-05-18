#pragma once

#include <vector>
#include <stdint.h>
#include "png.h"

enum EImageType {
	IMAGE_TYPE_UNKNOWN = 0,
	IMAGE_TYPE_PNG,
};

enum EImageFormat {
	IMAGE_FORMAT_R = 0,
	IMAGE_FORMAT_RGB,
	IMAGE_FORMAT_RGBA,
};

typedef std::vector<uint8_t> ImageLoader_LoadedImageBytes;

struct SByteLoader {
	size_t m_LoadOffset;
	ImageLoader_LoadedImageBytes* m_pLoadedImageBytes;
};

class CConverter {
	SByteLoader m_ByteLoader;
	png_bytepp m_pRowPointers;
	uint8_t* m_pImage;
	png_uint_32 m_Width;
	png_uint_32 m_Height;
	png_byte m_ColorType;
	size_t m_BytesPerPixel;
	
	size_t m_BytesInRow;

	uint8_t* m_pBodyBuffer;
	uint32_t m_BodyWidth;
	uint32_t m_BodyHeight;

	uint8_t* m_pMarkingBuffer;
	uint32_t m_MarkingWidth;
	uint32_t m_MarkingHeight;

	uint8_t* m_pEyesBuffer;
	uint32_t m_EyesWidth;
	uint32_t m_EyesHeight;

	uint8_t* m_pFeetBuffer;
	uint32_t m_FeetWidth;
	uint32_t m_FeetHeight;

	uint8_t* m_pHandsBuffer;
	uint32_t m_HandsWidth;
	uint32_t m_HandsHeight;

	void ConvertReal();

	bool Load();
	bool Save(EImageFormat ImageFormat, uint8_t* pRawBuffer, SByteLoader& WrittenBytes, uint32_t Width, uint32_t Height);
public:
	void Convert(const char* pSkin06, const char* pSkinName07);
};
