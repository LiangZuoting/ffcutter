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
	ui.thumbnailLabel->setScaledContents(false);
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
	connect(_service.data(), SIGNAL(scaleFinished(AVFrame*, QPixmap)), this, SLOT(onScaleFinished(AVFrame *, QPixmap)));
}

void FCVideoFrameWidget::setStreamIndex(int streamIndex)
{
	_streamIndex = streamIndex;
}

void FCVideoFrameWidget::setFrame(AVFrame* frame)
{
	_frame = frame;
	ui.ptsLabel->setText(QString::number(_service->tsToSec(_streamIndex, frame->pts)));
	// 以 widget 的高为基准缩放
	QSize size = QSize(frame->width, frame->height).scaled(frame->width, ui.thumbnailLabel->height(), Qt::KeepAspectRatio);
	ui.thumbnailLabel->setFixedWidth(size.width());
	setFixedWidth(size.width());
	_service->scaleAsync(frame, size.width(), size.height());
}

void FCVideoFrameWidget::setSelection(bool select)
{
	setStyleSheet(select ? "color: green;" : "");
}

AVFrame* FCVideoFrameWidget::frame() const
{
	return _frame;
}

int64_t FCVideoFrameWidget::pts() const
{
	return _frame->pts;
}

double FCVideoFrameWidget::sec() const
{
	return _service->tsToSec(_streamIndex, _frame->pts);
}

void FCVideoFrameWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	setSelection(true);
	emit doubleClicked();
	QWidget::mouseDoubleClickEvent(event);
}

void FCVideoFrameWidget::onScaleFinished(AVFrame *src, QPixmap scaled)
{
	if (src == _frame)
	{
		ui.thumbnailLabel->setPixmap(scaled);
	}
}