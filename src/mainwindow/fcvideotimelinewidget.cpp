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
	connect(_service.data(), SIGNAL(frameDeocded(AVFrame *)), this, SLOT(onFrameDecoded(AVFrame *)));
	connect(_service.data(), SIGNAL(decodeFinished()), this, SLOT(onDecodeFinished()));
}

void FCVideoTimelineWidget::decodeOnce()
{
	clear();
	_service->decodeFramesAsync(_streamIndex, MAX_LIST_SIZE);
}

void FCVideoTimelineWidget::onFrameDecoded(AVFrame *frame)
{
	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " decode frame error " << des;
	}
	else
	{
		FCVideoFrameWidget *widget = new FCVideoFrameWidget(this);
		ui.timelineLayout->addWidget(widget);
		widget->setService(_service);
		widget->setStreamIndex(_streamIndex);
		widget->setFrame(frame);
	}
}

void FCVideoTimelineWidget::onDecodeFinished()
{

}

void FCVideoTimelineWidget::clear()
{
	if (auto children = ui.timelineLayout->findChildren<FCVideoFrameWidget*>(); !children.isEmpty())
	{
		for (auto c : children)
		{
			ui.timelineLayout->removeWidget(c);
			delete c;
		}
	}
}