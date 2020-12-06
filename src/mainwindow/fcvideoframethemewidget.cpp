#include "fcvideoframethemewidget.h"
extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

FCVideoFrameThemeWidget::FCVideoFrameThemeWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

FCVideoFrameThemeWidget::~FCVideoFrameThemeWidget()
{
}

void FCVideoFrameThemeWidget::setFrame(AVFrame* frame)
{
	_frame = frame;
	ui.ptsLabel->setText(QString::number(_frame->pts));
	
	int destW = frame->width / 4;
	int destH = frame->height / 4;
	SwsContext* swsCtx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, destW, destH, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
	uint8_t* destData[4]{ nullptr };
	int destLineSize[4]{ 0 };
	av_image_alloc(destData, destLineSize, destW, destH, AV_PIX_FMT_RGB24, 1);
	sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, destData, destLineSize);

	QImage image(frame->width / 4, frame->height / 4, QImage::Format_RGB888);
	for (int i = 0; i < frame->height / 4; ++i)
	{
		memcpy(image.scanLine(i), destData[0] + i * destLineSize[0], destW * 3);
	}
	av_freep(&destData[0]);
	sws_freeContext(swsCtx);
	ui.themeLabel->setPixmap(QPixmap::fromImage(image));
}