#include "Converter.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <string.h>
#include <string>
#include "png.h"
#include <math.h>
#include <algorithm>

#include "helper_functions.h"

template<typename T>
void clamp(T& v, T min, T max) {
	if(v < min)
		v = min;
	if(v > max)
		v = max;
}

void FileSave(std::string& Path, std::string& Name, SByteLoader& File) {
	create_path(Path.c_str());
	FILE* pFile = fopen((Path + Name).c_str(), "wb");
	if(pFile) {
		fwrite(&(*File.m_pLoadedImageBytes)[0], 1, File.m_pLoadedImageBytes->size(), pFile);
		fclose(pFile);
	}
}

double cubic_hermite(double A, double B, double C, double D, double t) {

	double a = -A / 2.0 + (3.0*B) / 2.0 - (3.0*C) / 2.0 + D / 2.0;
	double b = A - (5.0*B) / 2.0 + 2.0*C - D / 2.0;
	double c = -A / 2.0 + C / 2.0;
	double d = B;

	return a*t*t*t + b*t*t + c*t + d;
}

void get_pixel_clamped(uint8_t* source_image, uint32_t x, uint32_t y, uint32_t W, uint32_t H, size_t BBP, uint8_t temp[])  {
	clamp<uint32_t>(x, 0, W - 1);
	clamp<uint32_t>(y, 0, H - 1);

	temp[0] = source_image[x*BBP+(W*BBP*y) + 0];
	temp[1] = source_image[x*BBP+(W*BBP*y) + 1];
	temp[2] = source_image[x*BBP+(W*BBP*y) + 2];
	temp[3] = source_image[x*BBP+(W*BBP*y) + 3];
}

void sample_bicubic(uint8_t* source_image, double u, double v, uint32_t W, uint32_t H, size_t BBP, uint8_t sample[]) {
	double x = (u * W) - 0.5;
	int xint = (int)x;
	double xfract = x-floor(x);

	double y = (v * H) - 0.5;
	int yint = (int)y;
	double yfract = y - floor(y);

	int i;

	uint8_t p00[4];
	uint8_t p10[4];
	uint8_t p20[4];
	uint8_t p30[4];

	uint8_t p01[4];
	uint8_t p11[4];
	uint8_t p21[4];
	uint8_t p31[4];

	uint8_t p02[4];
	uint8_t p12[4];
	uint8_t p22[4];
	uint8_t p32[4];

	uint8_t p03[4];
	uint8_t p13[4];
	uint8_t p23[4];
	uint8_t p33[4];

	// 1st row
	get_pixel_clamped(source_image, xint - 1, yint - 1, W, H, BBP, p00);   
	get_pixel_clamped(source_image, xint + 0, yint - 1, W, H, BBP, p10);
	get_pixel_clamped(source_image, xint + 1, yint - 1, W, H, BBP, p20);
	get_pixel_clamped(source_image, xint + 2, yint - 1, W, H, BBP, p30);

	// 2nd row
	get_pixel_clamped(source_image, xint - 1, yint + 0, W, H, BBP, p01);
	get_pixel_clamped(source_image, xint + 0, yint + 0, W, H, BBP, p11);
	get_pixel_clamped(source_image, xint + 1, yint + 0, W, H, BBP, p21);
	get_pixel_clamped(source_image, xint + 2, yint + 0, W, H, BBP, p31);

	// 3rd row
	get_pixel_clamped(source_image, xint - 1, yint + 1, W, H, BBP, p02);
	get_pixel_clamped(source_image, xint + 0, yint + 1, W, H, BBP, p12);
	get_pixel_clamped(source_image, xint + 1, yint + 1, W, H, BBP, p22);
	get_pixel_clamped(source_image, xint + 2, yint + 1, W, H, BBP, p32);

	// 4th row
	get_pixel_clamped(source_image, xint - 1, yint + 2, W, H, BBP, p03);
	get_pixel_clamped(source_image, xint + 0, yint + 2, W, H, BBP, p13);
	get_pixel_clamped(source_image, xint + 1, yint + 2, W, H, BBP, p23);
	get_pixel_clamped(source_image, xint + 2, yint + 2, W, H, BBP, p33);

	// interpolate bi-cubically!
	for (i = 0; i < 4; i++) {

		double col0 = cubic_hermite(p00[i], p10[i], p20[i], p30[i], xfract);
		double col1 = cubic_hermite(p01[i], p11[i], p21[i], p31[i], xfract);
		double col2 = cubic_hermite(p02[i], p12[i], p22[i], p32[i], xfract);
		double col3 = cubic_hermite(p03[i], p13[i], p23[i], p33[i], xfract);

		double value = cubic_hermite(col0, col1, col2, col3, yfract);

		clamp<double>(value, 0.0, 255.0);

		sample[i] = (uint8_t)value;
	}
}

void resize_image(uint8_t* source_image, uint32_t SW, uint32_t SH, uint8_t* destination_image, uint32_t W, uint32_t H, size_t BBP) {
	uint8_t sample[4];
	int y, x;

	double Scale = W / (double)SW;

	for (y = 0; y < H; y++) {

		double v = (double)y / (double)(H - 1);

		for (x = 0; x < W; ++x) {

			double u = (double)x / (double)(W - 1);
			sample_bicubic(source_image, u, v, SW, SH, BBP, sample);

			destination_image[x * BBP+((W * BBP)*y) + 0] = sample[0];
			destination_image[x * BBP+((W * BBP)*y) + 1] = sample[1];
			destination_image[x * BBP+((W * BBP)*y) + 2] = sample[2];
			destination_image[x * BBP+((W * BBP)*y) + 3] = sample[3];
		}
	}
}

void Upscale(uint8_t* pSrc, uint32_t Width, uint32_t Height, uint8_t* pDest, uint32_t NewWidth, uint32_t NewHeight, size_t BytesPerPixel) {
	resize_image(pSrc, Width, Height, pDest, NewWidth, NewHeight, BytesPerPixel);
}

void CopyBuffer(uint8_t* pDest, uint32_t XOffset, uint32_t YOffset, uint32_t DestWidth, uint32_t DestHeight, uint8_t* pSrc, uint32_t XOffsetSrc, uint32_t YOffsetSrc, uint32_t SrcWidth, uint32_t SrcHeight, uint32_t CopyWidth, uint32_t CopyHeight, size_t BytesPerPixel, bool DoAlphaBlending = false) {
	for(uint32_t y = 0; y < CopyHeight; ++y) {
		for(uint32_t x = 0; x < CopyWidth; ++x) {
			uint8_t AlphaValue = *(pSrc + (((y + YOffsetSrc) * SrcWidth * BytesPerPixel) + (x + XOffsetSrc) * BytesPerPixel + 3));
			for(size_t bpp = 0; bpp < BytesPerPixel; ++bpp) {
				if(!DoAlphaBlending) {
					*(pDest + (((y + YOffset) * DestWidth * BytesPerPixel) + (x + XOffset) * BytesPerPixel + bpp)) = *(pSrc + (((y + YOffsetSrc) * SrcWidth * BytesPerPixel) + (x + XOffsetSrc) * BytesPerPixel + bpp));
				}
				else {
					uint8_t Value = *(pSrc + (((y + YOffsetSrc) * SrcWidth * BytesPerPixel) + (x + XOffsetSrc) * BytesPerPixel + bpp));
					uint8_t CurVal = *(pDest + (((y + YOffset) * DestWidth * BytesPerPixel) + (x + XOffset) * BytesPerPixel + bpp));
					*(pDest + (((y + YOffset) * DestWidth * BytesPerPixel) + (x + XOffset) * BytesPerPixel + bpp)) = (CurVal) * (1.0 - (AlphaValue / 255.0)) + (AlphaValue / 255.0) * Value;
					if(bpp == 3) {
						*(pDest + (((y + YOffset) * DestWidth * BytesPerPixel) + (x + XOffset) * BytesPerPixel + bpp)) = std::min<uint64_t>(CurVal + AlphaValue, 255);
					}
				}
			}
		}
	}
}

void MirrorCopy(uint8_t* pBuffer, uint32_t Width, uint32_t Height, size_t BytesPerPixel) {
	size_t FullWidth = Width * BytesPerPixel;
	size_t FullSize = Width * Height * BytesPerPixel;
	uint8_t* pHelperBuffer = new uint8_t[FullSize];
	//mirror x
	for(size_t HeightIndex = 0; HeightIndex < Height; ++HeightIndex) {
		for(size_t WidthIndex = 0; WidthIndex < Width; ++WidthIndex) {
			memcpy(pHelperBuffer + (ptrdiff_t)(HeightIndex * FullWidth) + (ptrdiff_t)(((Width - 1) - WidthIndex) * BytesPerPixel), pBuffer + (ptrdiff_t)(HeightIndex * FullWidth) + (ptrdiff_t)(WidthIndex * BytesPerPixel), sizeof(uint8_t) * BytesPerPixel);
		}
	}
	memcpy(pBuffer, pHelperBuffer, FullSize);
	delete[] pHelperBuffer;
}

void CConverter::ConvertReal() {
	m_BodyWidth = 256;
	m_BodyHeight = 256;
	m_pBodyBuffer = new uint8_t[m_BodyWidth * m_BodyHeight * m_BytesPerPixel];
	mem_zero(m_pBodyBuffer, m_BodyWidth * m_BodyHeight * m_BytesPerPixel);
	
	uint32_t SizeBody = 128 - 2;
	uint32_t BodyOffsetX = 1;
	uint32_t BodyOffsetY = 0;
	uint32_t BodyCutY = 2;
	uint8_t* pBodyBuffer = new uint8_t[SizeBody * SizeBody * m_BytesPerPixel];
	uint8_t* pOldBodyBuffer = new uint8_t[96 * 96 * m_BytesPerPixel];
	CopyBuffer(pOldBodyBuffer, 0, 0, 96, 96, m_pImage, 96, 0, m_Width, m_Height, 96, 96, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 96, 96, pBodyBuffer, SizeBody, SizeBody, m_BytesPerPixel);
	CopyBuffer(m_pBodyBuffer, 128 + BodyOffsetX, BodyOffsetY, m_BodyWidth, m_BodyHeight, pBodyBuffer, 0, BodyCutY, SizeBody, SizeBody, SizeBody, SizeBody - BodyCutY, m_BytesPerPixel);

	m_MarkingWidth = 128;
	m_MarkingHeight = 128;
	m_pMarkingBuffer = new uint8_t[m_MarkingWidth * m_MarkingHeight * m_BytesPerPixel];
	mem_zero(m_pMarkingBuffer, m_MarkingWidth * m_MarkingHeight * m_BytesPerPixel);
	
	CopyBuffer(pOldBodyBuffer, 0, 0, 96, 96, m_pImage, 0, 0, m_Width, m_Height, 96, 96, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 96, 96, pBodyBuffer, SizeBody, SizeBody, m_BytesPerPixel);
	CopyBuffer(m_pMarkingBuffer, 0 + BodyOffsetX, BodyOffsetY, m_MarkingWidth, m_MarkingHeight, pBodyBuffer, 0, BodyCutY, SizeBody, SizeBody, SizeBody, SizeBody - BodyCutY, m_BytesPerPixel);

	m_EyesWidth = 128;
	m_EyesHeight = 128;
	m_pEyesBuffer = new uint8_t[m_EyesWidth * m_EyesHeight * m_BytesPerPixel];
	mem_zero(m_pEyesBuffer, m_EyesWidth * m_EyesHeight * m_BytesPerPixel);

	uint32_t EyeSize = 42;
	pOldBodyBuffer = new uint8_t[32 * 32 * m_BytesPerPixel];
	pBodyBuffer = new uint8_t[EyeSize * EyeSize * m_BytesPerPixel];

	uint32_t EyeOffsetX = 5;
	uint32_t EyeOffsetX2 = 13;
	uint32_t EyeOffsetY = 5;

	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 32 * 2, 32 * 3, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	mem_zero(pBodyBuffer, EyeSize * EyeSize * m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, EyeOffsetX, 0, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel);
	MirrorCopy(pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, 32 - EyeOffsetX2, 0, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel, true);

	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 32 * 3, 32 * 3, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	mem_zero(pBodyBuffer, EyeSize * EyeSize * m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, EyeOffsetX + 64 * 1, 0, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel);
	MirrorCopy(pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, 32 - EyeOffsetX2 + 64 * 1, 0, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel, true);

	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 32 * 4, 32 * 3, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	mem_zero(pBodyBuffer, EyeSize * EyeSize * m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, EyeOffsetX + 64 * 0, 32 * 1, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel);
	MirrorCopy(pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, 32 - EyeOffsetX2 + 64 * 0, 32 * 1, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel, true);

	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 32 * 5, 32 * 3, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	mem_zero(pBodyBuffer, EyeSize * EyeSize * m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, EyeOffsetX + 64 * 1, 32 * 1, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel);
	MirrorCopy(pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, 32 - EyeOffsetX2 + 64 * 1, 32 * 1, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel, true);

	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 32 * 7, 32 * 3, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	mem_zero(pBodyBuffer, EyeSize * EyeSize * m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, EyeOffsetX + 64 * 0, 32 * 2, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel);
	MirrorCopy(pBodyBuffer, EyeSize, EyeSize, m_BytesPerPixel);
	CopyBuffer(m_pEyesBuffer, 32 - EyeOffsetX2 + 64 * 0, 32 * 2, m_EyesWidth, m_EyesHeight, pBodyBuffer, 0, EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, EyeSize, EyeSize - EyeOffsetY, m_BytesPerPixel, true);

	m_FeetWidth = 128;
	m_FeetHeight = 64;
	m_pFeetBuffer = new uint8_t[m_FeetWidth * m_FeetHeight * m_BytesPerPixel];
	mem_zero(m_pFeetBuffer, m_FeetWidth * m_FeetHeight * m_BytesPerPixel);

	pOldBodyBuffer = new uint8_t[32 * 32 * m_BytesPerPixel];
	pBodyBuffer = new uint8_t[64 * 64 * m_BytesPerPixel];
	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 192 + 16, 32, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, 64, 64, m_BytesPerPixel);
	CopyBuffer(m_pFeetBuffer, 0, 0, m_FeetWidth, m_FeetHeight, pBodyBuffer, 0, 0, 64, 64, 64, 64, m_BytesPerPixel);
	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 192 + 16, 64, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, 64, 64, m_BytesPerPixel);
	CopyBuffer(m_pFeetBuffer, 64, 0, m_FeetWidth, m_FeetHeight, pBodyBuffer, 0, 0, 64, 64, 64, 64, m_BytesPerPixel);

	m_HandsWidth = 128;
	m_HandsHeight = 64;
	m_pHandsBuffer = new uint8_t[m_HandsWidth * m_HandsHeight * m_BytesPerPixel];
	mem_zero(m_pHandsBuffer, m_HandsWidth * m_HandsHeight * m_BytesPerPixel);

	pOldBodyBuffer = new uint8_t[32 * 32 * m_BytesPerPixel];
	pBodyBuffer = new uint8_t[48 * 48 * m_BytesPerPixel];
	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 192, 0, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, 48, 48, m_BytesPerPixel);
	CopyBuffer(m_pHandsBuffer, 8, 8, m_HandsWidth, m_HandsHeight, pBodyBuffer, 0, 0, 48, 48, 48, 48, m_BytesPerPixel);
	CopyBuffer(pOldBodyBuffer, 0, 0, 32, 32, m_pImage, 192 + 32, 0, m_Width, m_Height, 32, 32, m_BytesPerPixel);
	Upscale(pOldBodyBuffer, 32, 32, pBodyBuffer, 48, 48, m_BytesPerPixel);
	CopyBuffer(m_pHandsBuffer, 64 + 8, 8, m_HandsWidth, m_HandsHeight, pBodyBuffer, 0, 0, 48, 48, 48, 48, m_BytesPerPixel);
}

void CreateJSONSub(const char* pSkinName07, const char* pBodyPart, FILE* pFile, bool AddAlpha = false, bool IsLast = false) {
	const char* pText = "\t\"";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = pBodyPart;
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\": {\n";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\t\t\"filename\": \"";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = pSkinName07;
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\",\n";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\t\t\"custom_colors\": \"false\",\n";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\t\t\"hue\": 255,\n";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\t\t\"sat\": 255,\n";
	fwrite(pText, 1, strlen(pText), pFile);
	pText = "\t\t\"lgt\": 255";
	fwrite(pText, 1, strlen(pText), pFile);
	if(AddAlpha) {
		pText = ",\n";
		fwrite(pText, 1, strlen(pText), pFile);
		pText = "\t\t\"alp\": 255\n";
		fwrite(pText, 1, strlen(pText), pFile);
	}
	else {
		pText = "\n";
		fwrite(pText, 1, strlen(pText), pFile);
	}
	pText = "\t}";
	fwrite(pText, 1, strlen(pText), pFile);
	if(IsLast) {
		pText = "\n";
		fwrite(pText, 1, strlen(pText), pFile);
	}
	else {
		pText = ",\n";
		fwrite(pText, 1, strlen(pText), pFile);
	}
}

void CreateJSON(const char* pSkinName07, std::string& Path) {
	create_path(Path.c_str());
	FILE* pFile = fopen(std::string(Path + pSkinName07 + ".json").c_str(), "wb");
	if(pFile) {
		const char* pText = "{\"skin\": {\n";
		fwrite(pText, 1, strlen(pText), pFile);
		CreateJSONSub(pSkinName07, "body", pFile);
		CreateJSONSub(pSkinName07, "marking", pFile, true);
		CreateJSONSub(pSkinName07, "hands", pFile);
		CreateJSONSub(pSkinName07, "feet", pFile);
		CreateJSONSub(pSkinName07, "eyes", pFile, false, true);
		pText = "}}\n";
		fwrite(pText, 1, strlen(pText), pFile);
		fclose(pFile);
	}
}

void CConverter::Convert(const char* pSkin06, const char* pSkinName07) {
	FILE* pFile = fopen(pSkin06, "rb");
	if(pFile) {
		fseek(pFile, 0, SEEK_END);
		size_t size = (size_t)ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
		uint8_t* pBuffer = new uint8_t[size];

		fread(pBuffer, 1, size, pFile);

		m_ByteLoader.m_pLoadedImageBytes = new std::vector<uint8_t>();
		m_ByteLoader.m_pLoadedImageBytes->resize(size);
		memcpy(&(*m_ByteLoader.m_pLoadedImageBytes)[0], pBuffer, size);

		delete[] pBuffer;

		fclose(pFile);

		Load();
		ConvertReal();

		SByteLoader WrittenBytes;
		WrittenBytes.m_pLoadedImageBytes = new std::vector<uint8_t>();

		Save(EImageFormat::IMAGE_FORMAT_RGBA, m_pBodyBuffer, WrittenBytes, m_BodyWidth, m_BodyHeight);
		std::string str = std::string("output/") + "body/";
		std::string strname = std::string(pSkinName07) + ".png";
		FileSave(str, strname, WrittenBytes);

		Save(EImageFormat::IMAGE_FORMAT_RGBA, m_pMarkingBuffer, WrittenBytes, m_MarkingWidth, m_MarkingHeight);
		str = std::string("output/") + "marking/";
		FileSave(str, strname, WrittenBytes);

		Save(EImageFormat::IMAGE_FORMAT_RGBA, m_pEyesBuffer, WrittenBytes, m_EyesWidth, m_EyesHeight);
		str = std::string("output/") + "eyes/";
		FileSave(str, strname, WrittenBytes);

		Save(EImageFormat::IMAGE_FORMAT_RGBA, m_pFeetBuffer, WrittenBytes, m_FeetWidth, m_FeetHeight);
		str = std::string("output/") + "feet/";
		FileSave(str, strname, WrittenBytes);

		Save(EImageFormat::IMAGE_FORMAT_RGBA, m_pHandsBuffer, WrittenBytes, m_HandsWidth, m_HandsHeight);
		str = std::string("output/") + "hands/";
		FileSave(str, strname, WrittenBytes);

		str = "output/";
		CreateJSON(pSkinName07, str);
	}
}

void ReadDataFromLoadedBytes(png_structp pPNGStruct, png_bytep pOutBytes, png_size_t ByteCountToRead) {
	png_voidp pIO_Ptr = png_get_io_ptr(pPNGStruct);

	SByteLoader* pByteLoader = (SByteLoader*)pIO_Ptr;

	memcpy(pOutBytes, &(*pByteLoader->m_pLoadedImageBytes)[pByteLoader->m_LoadOffset], (size_t)ByteCountToRead);

	pByteLoader->m_LoadOffset += (size_t)ByteCountToRead;
}

bool CConverter::Load() {
	png_structp	m_pPNGStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	png_infop m_pPNGInfo = png_create_info_struct(m_pPNGStruct);

	m_ByteLoader.m_LoadOffset = 8;

	png_set_read_fn(m_pPNGStruct, (png_bytep)&m_ByteLoader, ReadDataFromLoadedBytes);

	png_set_sig_bytes(m_pPNGStruct, 8);

	png_read_info(m_pPNGStruct, m_pPNGInfo);

	m_Width = png_get_image_width(m_pPNGStruct, m_pPNGInfo);
	m_Height = png_get_image_height(m_pPNGStruct, m_pPNGInfo);
	m_ColorType = png_get_color_type(m_pPNGStruct, m_pPNGInfo);
	png_byte BitDepth = png_get_bit_depth(m_pPNGStruct, m_pPNGInfo);

	if(BitDepth == 16)
		png_set_strip_16(m_pPNGStruct);
	
	if (m_ColorType == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(m_pPNGStruct);

	if(m_ColorType == PNG_COLOR_TYPE_GRAY && BitDepth < 8)
		png_set_expand_gray_1_2_4_to_8(m_pPNGStruct);

	if(png_get_valid(m_pPNGStruct, m_pPNGInfo, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(m_pPNGStruct);

	png_read_update_info(m_pPNGStruct, m_pPNGInfo);

	m_BytesInRow = png_get_rowbytes(m_pPNGStruct, m_pPNGInfo);

	m_pRowPointers = new png_bytep[m_Height];
	for(uint32_t y = 0; y < m_Height; ++y) {
		m_pRowPointers[y] = new png_byte[m_BytesInRow];
	}

	png_read_image(m_pPNGStruct, m_pRowPointers);

	m_BytesPerPixel = 4;

	m_pImage = new uint8_t[m_Height * m_Width * m_BytesPerPixel];
	for(uint32_t y = 0; y < m_Height; ++y) {
		for(uint32_t x = 0; x < m_Width; ++x) {
			for(size_t bpp = 0; bpp < m_BytesPerPixel; ++bpp) {
				*(m_pImage + (y * m_Width * m_BytesPerPixel) + x * m_BytesPerPixel + bpp) = *(m_pRowPointers[y] + x * m_BytesPerPixel + bpp);
			}
		}
	}


	return true;
}

void WriteDataFromLoadedBytes(png_structp pPNGStruct, png_bytep pOutBytes, png_size_t ByteCountToWrite) {
	if(ByteCountToWrite > 0) {
		png_voidp pIO_Ptr = png_get_io_ptr(pPNGStruct);

		SByteLoader* pByteLoader = (SByteLoader*)pIO_Ptr;

		size_t NewSize = pByteLoader->m_LoadOffset + (size_t)ByteCountToWrite;
		pByteLoader->m_pLoadedImageBytes->resize(NewSize);
		
		memcpy(&(*pByteLoader->m_pLoadedImageBytes)[pByteLoader->m_LoadOffset], pOutBytes, (size_t)ByteCountToWrite);
		pByteLoader->m_LoadOffset = NewSize;
	}
}

void FlushPNGWrite(png_structp png_ptr) {
}

size_t BytesPerPixel(EImageFormat ImageFormat) {
	if(ImageFormat == IMAGE_FORMAT_R)
		return 1;
	else if(ImageFormat == IMAGE_FORMAT_RGB)
		return 3;
	else if(ImageFormat == IMAGE_FORMAT_RGBA)
		return 4;
	return 0;
}

bool CConverter::Save(EImageFormat ImageFormat, uint8_t* pRawBuffer, SByteLoader& WrittenBytes, uint32_t Width, uint32_t Height) {
	png_structp pPNGStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	png_infop pPNGInfo = png_create_info_struct(pPNGStruct);

	WrittenBytes.m_LoadOffset = 0;
	WrittenBytes.m_pLoadedImageBytes->clear();

	png_set_write_fn(pPNGStruct, (png_bytep)&WrittenBytes, WriteDataFromLoadedBytes, FlushPNGWrite);

	int ColorType = PNG_COLOR_TYPE_RGB;
	size_t WriteBytesPerPixel = BytesPerPixel(ImageFormat);
	if(ImageFormat == IMAGE_FORMAT_R) {
		ColorType = PNG_COLOR_TYPE_GRAY;
	}
	else if(ImageFormat == IMAGE_FORMAT_RGBA) {
		ColorType = PNG_COLOR_TYPE_RGBA;
	}

	png_set_IHDR(pPNGStruct, pPNGInfo, Width, Height, 8, ColorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(pPNGStruct, pPNGInfo);

	png_bytepp pRowPointers = new png_bytep[Height];
	size_t WidthBytes = (size_t)Width * WriteBytesPerPixel;
	ptrdiff_t BufferOffset = 0;
	for(uint32_t y = 0; y < Height; ++y) {
		pRowPointers[y] = new png_byte[WidthBytes];
		memcpy(pRowPointers[y], pRawBuffer + BufferOffset, WidthBytes);
		BufferOffset += (ptrdiff_t)WidthBytes;
	}
	png_write_image(pPNGStruct, pRowPointers);

	png_write_end(pPNGStruct, pPNGInfo);

	for(uint32_t y = 0; y < Height; ++y) {
		delete[] (pRowPointers[y]);
	}
	delete[] (pRowPointers);

	png_destroy_info_struct(pPNGStruct, &pPNGInfo);
	png_destroy_write_struct(&pPNGStruct, NULL);

	return true;
}