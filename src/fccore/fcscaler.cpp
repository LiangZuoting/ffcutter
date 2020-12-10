#include "fcscaler.h"
extern "C"
{
#include <libavutil/imgutils.h>
}

int FCScaler::create(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int destWidth, int destHeight, AVPixelFormat destFormat)
{
	_swsContext = sws_getContext(srcWidth, srcHeight, srcFormat, destWidth, destHeight, destFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
	//int ret = av_image_alloc(_scaledImageData, _scaledImageLineSizes, destWidth, destHeight, AV_PIX_FMT_PAL8, 1);
	int bytes = av_image_get_buffer_size(destFormat, destWidth, destHeight, 1);
	uint8_t *data = (uint8_t *)av_malloc(bytes);
	int ret = av_image_fill_arrays(_scaledImageData, _scaledImageLineSizes, data, destFormat, destWidth, destHeight, 1);
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

FCScaler::ScaleResult FCScaler::scale(const uint8_t *const *srcSlice, const int *srcStride, uint8_t *scaledData[4], int scaledLineSizes[4])
{
	if (!scaledData)
	{
		scaledData = _scaledImageData;
	}
	if (!scaledLineSizes)
	{
		scaledLineSizes = _scaledImageLineSizes;
	}
	int ret = sws_scale(_swsContext, srcSlice, srcStride, 0, _srcHeight, scaledData, scaledLineSizes);
	if (ret <= 0)
	{
		return { nullptr, nullptr };
	}
	return { scaledData, scaledLineSizes };
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