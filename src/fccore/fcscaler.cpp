#include "fcscaler.h"
extern "C"
{
#include <libavutil/imgutils.h>
}

int FCScaler::create(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int destWidth, int destHeight, AVPixelFormat destFormat)
{
	_swsContext = sws_getContext(srcWidth, srcHeight, srcFormat, destWidth, destHeight, destFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
	int ret = av_image_alloc(_scaledImageData, _scaledImageLineSizes, destWidth, destHeight, destFormat, 1);
	if (ret < 0)
	{
		sws_freeContext(_swsContext);
		_swsContext = nullptr;
		return ret;
	}
	_srcWidth = srcWidth;
	_srcHeight = srcHeight;
	_srcFormat = srcFormat;
	_scaledWidth = destWidth;
	_scaledHeight = destHeight;
	_scaledFormat = destFormat;
	return 0;
}

FCScaler::~FCScaler()
{
	destroy();
}

QPair<const uint8_t *const *, const int *> FCScaler::scale(const uint8_t *const *srcSlice, const int *srcStride)
{
	int ret = sws_scale(_swsContext, srcSlice, srcStride, 0, _srcHeight, _scaledImageData, _scaledImageLineSizes);
	if (ret <= 0)
	{
		return { nullptr, nullptr };
	}
	return { _scaledImageData, _scaledImageLineSizes };
}

bool FCScaler::equal(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int destWidth, int destHeight, AVPixelFormat destFormat)
{
	return _srcWidth == srcWidth &&
		_srcHeight == srcHeight && _srcFormat == srcFormat &&
		_scaledWidth == destWidth && _scaledHeight == destHeight &&
		_scaledFormat == destFormat;
}

void FCScaler::destroy()
{
	if (_swsContext)
	{
		sws_freeContext(_swsContext);
		_swsContext = nullptr;
	}
	if (_scaledImageData)
	{
		av_freep(&_scaledImageData[0]);
	}
}