#include "fcvideoframewidget.h"
extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

FCVideoFrameWidget::FCVideoFrameWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

FCVideoFrameWidget::~FCVideoFrameWidget()
{
	if (_frame)
	{
		av_frame_free(&_frame);
	}
}

void FCVideoFrameWidget::setService(const QSharedPointer<FCService>& service)
{
	_service = service;
	connect(_service.data(), SIGNAL(scaleFinished(QPixmap)), this, SLOT(onScaleFinished(QPixmap)));
}

void FCVideoFrameWidget::setStreamIndex(int streamIndex)
{
	_streamIndex = streamIndex;
}

void FCVideoFrameWidget::setFrame(AVFrame* frame)
{
	_frame = frame;
	ui.ptsLabel->setText(QString::number(_service->timestampToSecond(_streamIndex, frame->pts)));
	_service->scaleAsync(frame, frame->width / 4, frame->height / 4);
}

void FCVideoFrameWidget::setSelection(bool select)
{
	setStyleSheet(select ? "color: green;" : "");
}

AVFrame* FCVideoFrameWidget::frame() const
{
	return _frame;
}

void FCVideoFrameWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	setSelection(true);
	emit doubleClicked();
	QWidget::mouseDoubleClickEvent(event);
}

void FCVideoFrameWidget::onScaleFinished(QPixmap pixmap)
{
	ui.thumbnailLabel->setPixmap(pixmap);
}