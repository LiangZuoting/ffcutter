#include "fcvideotimelinewidget.h"
#include <QDebug>

FCVideoTimelineWidget::FCVideoTimelineWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.forwardBtn, SIGNAL(clicked()), this, SLOT(decodeOnce()));
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

int64_t FCVideoTimelineWidget::selectedPts() const
{
	if (_selected)
	{
		return _selected->frame()->pts;
	}
	return 0;
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
			connect(widget, SIGNAL(clicked()), SLOT(onVideoFrameClicked()));
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

void FCVideoTimelineWidget::onVideoFrameClicked()
{
	auto widget = qobject_cast<FCVideoFrameWidget*>(sender());
	_selected = widget;

	if (auto children = findChildren<FCVideoFrameWidget*>(); !children.isEmpty())
	{
		for (auto c : children)
		{
			c->setSelection(c == widget);
		}
	}
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