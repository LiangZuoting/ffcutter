#include "fcvideoframewidget.h"
#include <QMouseEvent>
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
	connect(_service.data(), SIGNAL(scaleFinished(AVFrame*, QPixmap, void *)), this, SLOT(onScaleFinished(AVFrame *, QPixmap, void *)));
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
	_service->scaleAsync(frame, size.width(), size.height(), this);
}

void FCVideoFrameWidget::setStart(bool select)
{
	_isStart = select;
	if (_isStart && _isEnd)
	{
		setStyleSheet("color: yellow;");
	}
	else
	{
		setStyleSheet(_isStart ? "color: green;" : "");
	}
}

void FCVideoFrameWidget::setEnd(bool select)
{
	_isEnd = select;
	if (_isStart && _isEnd)
	{
		setStyleSheet("color: yellow;");
	}
	else
	{
		setStyleSheet(_isEnd ? "color: red;" : "");
	}
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

void FCVideoFrameWidget::beginSelect()
{
	_selecting = true;
}

void FCVideoFrameWidget::endSelect()
{
	_selecting = false;
}

void FCVideoFrameWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		setStart(true);
		emit leftDoubleClicked();
	}
	else if (event->button() == Qt::RightButton)
	{
		setEnd(true);
		emit rightDoubleClicked();
	}
	QWidget::mouseDoubleClickEvent(event);
}

void FCVideoFrameWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && _selecting)
	{
		auto pos = event->pos();
		auto w = width();
		auto h = height();
		auto imgW = ui.thumbnailLabel->pixmap().width();
		auto imgH = ui.thumbnailLabel->pixmap().height();
		auto xRatio = _frame->width * 1.0 / imgW;
		auto yRatio = _frame->height * 1.0 / imgH;
		QPoint imgPos = { int((pos.x() - (w - imgW)) * xRatio), int((pos.y() - (h - imgH)) * yRatio) };
		emit startSelect(imgPos);
	}
}

void FCVideoFrameWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && _selecting)
	{
		auto pos = event->pos();
		auto w = width();
		auto h = height();
		auto imgW = ui.thumbnailLabel->pixmap().width();
		auto imgH = ui.thumbnailLabel->pixmap().height();
		auto xRatio = _frame->width * 1.0 / imgW;
		auto yRatio = _frame->height * 1.0 / imgH;
		QPoint imgPos = { int((pos.x() - (w - imgW)) * xRatio), int((pos.y() - (h - imgH)) * yRatio) };
		emit stopSelect(imgPos);
	}
}

void FCVideoFrameWidget::onScaleFinished(AVFrame *src, QPixmap scaled, void *userData)
{
	if (src == _frame)
	{
		ui.thumbnailLabel->setPixmap(scaled);
	}
}