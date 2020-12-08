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

void FCVideoFrameThemeWidget::setService(const QSharedPointer<FCService>& service)
{
	_service = service;
	connect(_service.data(), SIGNAL(scaleFinished(QPixmap)), this, SLOT(onScaleFinished(QPixmap)));
}

void FCVideoFrameThemeWidget::setFrame(AVFrame* frame)
{
	_frame = frame;
	ui.ptsLabel->setText(QString::number(_frame->pts));
	_service->scaleAsync(frame, frame->width / 4, frame->height / 4);
}

void FCVideoFrameThemeWidget::onScaleFinished(QPixmap pixmap)
{
	ui.themeLabel->setPixmap(pixmap);
}