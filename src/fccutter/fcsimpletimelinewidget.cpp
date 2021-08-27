#include "fcsimpletimelinewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QTimer>
#include <fcservice.h>
#include "fcvideoframewidget.h"

FCSimpleTimelineWidget::FCSimpleTimelineWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setMouseTracking(true);
}

FCSimpleTimelineWidget::~FCSimpleTimelineWidget()
{
}

void FCSimpleTimelineWidget::loadFile(const QString &filePath)
{
	_service.reset(new FCService());
	connect(_service.data(), &FCService::fileOpened, this, [=]() 
		{
			QTimer::singleShot(0, this, [=]() 
				{
					setCurrentStream(_streamIndex);
				});
		});
	connect(_service.data(), SIGNAL(seekFinished(int, QList<FCFrame>, void *)), this, SLOT(onSeekFinished(int, QList<FCFrame>, void *)));
	connect(_service.data(), SIGNAL(frameDeocded(QList<FCFrame>, void *)), this, SLOT(onFrameDecoded(QList<FCFrame>, void *)));
	
	_service->openFileAsync(filePath, this);
}

void FCSimpleTimelineWidget::setCurrentStream(int streamIndex)
{
	_streamIndex = streamIndex;
	if (_service)
	{
		_duration = _service->duration(streamIndex) / 1000;
	}
}

void FCSimpleTimelineWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	QPainterPath path;
	path.addRoundedRect(0, 0, width(), height(), 5, 5);
	painter.setPen(Qt::darkGray);
	painter.fillPath(path, Qt::gray);
}

void FCSimpleTimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (_service)
	{
		_frameWidget.reset();

		_x = event->x();
		_seekSeconds = _duration * _x / width();
		_service->fastSeekAsync(_streamIndex, _seekSeconds, this);
	}
}

void FCSimpleTimelineWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	auto pos = _duration * event->x() / width();
	emit seekRequest(pos);
}

void FCSimpleTimelineWidget::enterEvent(QEvent *event)
{
	_cursorIn = true;
}

void FCSimpleTimelineWidget::leaveEvent(QEvent *event)
{
	_cursorIn = false;
	_frameWidget.reset();
}

void FCSimpleTimelineWidget::onSeekFinished(int streamIndex, QList<FCFrame> frames, void *userData)
{
	if (userData == this && _cursorIn && _service)
	{
		_service->decodeOnePacketAsync(_streamIndex, this);
	}
}

void FCSimpleTimelineWidget::onFrameDecoded(QList<FCFrame> frames, void *userData)
{
	if (userData == this && _cursorIn && _service)
	{
		_frameWidget.reset(new FCVideoFrameWidget());
		_frameWidget->setWindowFlag(Qt::ToolTip);
		_frameWidget->setService(_service);
		_frameWidget->setStreamIndex(_streamIndex);
		_frameWidget->setFrame(frames.front().frame);
		auto pos = mapToGlobal({ _x, 0 });
		_frameWidget->move(pos.x() - _frameWidget->width() / 2, pos.y() - _frameWidget->height());
		_frameWidget->show();
	}
}