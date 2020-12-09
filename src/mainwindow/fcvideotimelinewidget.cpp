#include "fcvideotimelinewidget.h"
#include "fcvideoframewidget.h"
#include <QDebug>

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
	connect(_service.data(), SIGNAL(frameDeocded(QList<AVFrame*>)), this, SLOT(onFrameDecoded(QList<AVFrame *>)));
	connect(_service.data(), SIGNAL(decodeFinished()), this, SLOT(onDecodeFinished()));
}

void FCVideoTimelineWidget::decodeOnce()
{
	clear();
	_service->decodePacketsAsync(_streamIndex, MAX_LIST_SIZE);
}

void FCVideoTimelineWidget::onFrameDecoded(QList<AVFrame*> frames)
{
	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " decode frame error " << des;
	}
	else
	{
		for (auto frame : frames)
		{
			FCVideoFrameWidget* widget = new FCVideoFrameWidget(this);
			ui.timelineLayout->addWidget(widget);
			widget->setService(_service);
			widget->setStreamIndex(_streamIndex);
			widget->setFrame(frame);
		}
	}
}

void FCVideoTimelineWidget::onDecodeFinished()
{

}

void FCVideoTimelineWidget::clear()
{
	if (auto children = findChildren<FCVideoFrameWidget*>(); !children.isEmpty())
	{
		for (auto c : children)
		{
			ui.timelineLayout->removeWidget(c);
			delete c;
		}
	}
}