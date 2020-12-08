#pragma once

#include <utility>
#include <QPair>
extern "C"
{
#include <libswscale/swscale.h>
}

class FCScaler
{
public:
	FCScaler() = default;

	int create(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int destWidth, int destHeight, AVPixelFormat destFormat);

	QPair<const uint8_t *const *, const int*> scale(const uint8_t* const* srcSlice, const int* srcStride);

	bool equal(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int destWidth, int destHeight, AVPixelFormat destFormat);

private:
	SwsContext *_swsContext = nullptr;
	int _srcWidth = 0;
	int _srcHeight = 0;
	AVPixelFormat _srcFormat = AV_PIX_FMT_NONE;
	int _scaledWidth = 0;
	int _scaledHeight = 0;
	AVPixelFormat _scaledFormat = AV_PIX_FMT_NONE;
	uint8_t *_scaledImageData[4]{ 0 };
	int _scaledImageLineSizes[4]{ 0 };
};