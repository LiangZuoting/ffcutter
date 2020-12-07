#include "fcvideotimelinewidget.h"
#include "fcvideoframethemewidget.h"

FCVideoTimelineWidget::FCVideoTimelineWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

FCVideoTimelineWidget::~FCVideoTimelineWidget()
{
}

void FCVideoTimelineWidget::setStreamIndex(int streamIndex)
{
	_streamIndex = streamIndex;
}

void FCVideoTimelineWidget::setService(const QSharedPointer<FCService>& service)
{
	_service = service;
}

void FCVideoTimelineWidget::decodeOnce()
{
	clear();
	auto f = _service->decodeFrames(_streamIndex, MAX_LIST_SIZE);
	_vecFrame = f.result();
	if (_vecFrame.isEmpty())
	{
		auto [iErr, strErr] = _service->lastError();
		if (iErr)
		{
			qCritical(QString("decode error %1").arg(strErr).toStdString().data());
		}
	}
	for (auto frame : _vecFrame)
	{
		FCVideoFrameThemeWidget* widget = new FCVideoFrameThemeWidget(this);
		ui.timelineLayout->addWidget(widget);
		widget->setFrame(frame);
	}
}

void FCVideoTimelineWidget::clear()
{
	for (auto frame : _vecFrame)
	{
		av_frame_unref(frame);
	}
	_vecFrame.clear();
}