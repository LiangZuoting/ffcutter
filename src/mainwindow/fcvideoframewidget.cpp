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
	_service->scaleAsync(frame, AV_PIX_FMT_RGB24, frame->width / 4, frame->height / 4);
}

void FCVideoFrameWidget::setSelection(bool select)
{
	setStyleSheet(select ? "color: green;" : "");
}

AVFrame* FCVideoFrameWidget::frame() const
{
	return _frame;
}

void FCVideoFrameWidget::mousePressEvent(QMouseEvent* event)
{
	_pressed = true;
	QWidget::mousePressEvent(event);
}

void FCVideoFrameWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (_pressed)
	{
		_pressed = false;
		setSelection(true);
		emit clicked();
	}
	QWidget::mouseReleaseEvent(event);
}

void FCVideoFrameWidget::onScaleFinished(QPixmap pixmap)
{
	ui.thumbnailLabel->setPixmap(pixmap);
}