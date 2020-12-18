#include "fcscaler.h"
#include "fcutil.h"
extern "C"
{
#include <libavutil/imgutils.h>
}

int FCScaler::create(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
	_swsContext = sws_getContext(srcWidth, srcHeight, srcFormat, dstWidth, dstHeight, dstFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
	int bytes = av_image_get_buffer_size(dstFormat, dstWidth, dstHeight, 1);
	uint8_t *data = (uint8_t *)av_malloc(bytes);
	_lastError = av_image_fill_arrays(_scaledImageData, _scaledImageLineSizes, data, dstFormat, dstWidth, dstHeight, 1);
	if (_lastError < 0)
	{
		FCUtil::printAVError(_lastError, "av_image_fill_arrays");
		sws_freeContext(_swsContext);
		_swsContext = nullptr;
		return _lastError;
	}
	_srcWidth = srcWidth;
	_srcHeight = srcHeight;
	_srcFormat = srcFormat;
	_scaledWidth = dstWidth;
	_scaledHeight = dstHeight;
	_scaledFormat = dstFormat;
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
	_lastError = sws_scale(_swsContext, srcSlice, srcStride, 0, _srcHeight, scaledData, scaledLineSizes);
	if (_lastError <= 0)
	{
		FCUtil::printAVError(_lastError, "sws_scale");
		return { nullptr, nullptr };
	}
	return { scaledData, scaledLineSizes };
}

bool FCScaler::equal(int srcWidth, int srcHeight, AVPixelFormat srcFormat, int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
	return _srcWidth == srcWidth &&
		_srcHeight == srcHeight && _srcFormat == srcFormat &&
		_scaledWidth == dstWidth && _scaledHeight == dstHeight &&
		_scaledFormat == dstFormat;
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