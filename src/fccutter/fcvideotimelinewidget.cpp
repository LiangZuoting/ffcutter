#include "fcvideotimelinewidget.h"
#include <QDebug>

FCVideoTimelineWidget::FCVideoTimelineWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.forwardBtn, SIGNAL(clicked()), this, SLOT(onForwardBtnClicked()));
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
	connect(_service.data(), SIGNAL(frameDeocded(QList<FCFrame>)), this, SLOT(onFrameDecoded(QList<FCFrame>)));
	connect(_service.data(), SIGNAL(decodeFinished()), this, SLOT(onDecodeFinished()));
}

void FCVideoTimelineWidget::decodeOnce()
{
	_service->decodePacketsAsync(_streamIndex, MAX_LIST_SIZE);
	_loadingDialog.exec2(tr(u8"½âÂë..."));
}

double FCVideoTimelineWidget::selectedSec() const
{
	if (_selected)
	{
		return _selected->sec();
	}
	return 0;
}

void FCVideoTimelineWidget::appendFrames(const QList<FCFrame>& frames)
{
	for (auto frame : frames)
	{
		if (_streamIndex == frame.streamIndex)
		{
			FCVideoFrameWidget* widget = new FCVideoFrameWidget(this);
			connect(widget, SIGNAL(doubleClicked()), SLOT(onVideoFrameClicked()));
			ui.timelineLayout->addWidget(widget);
			widget->setService(_service);
			widget->setStreamIndex(_streamIndex);
			widget->setFrame(frame.frame);
		}
	}
}

void FCVideoTimelineWidget::onForwardBtnClicked()
{
	clear();
	decodeOnce();
}

void FCVideoTimelineWidget::onFrameDecoded(QList<FCFrame> frames)
{
	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " decode frame error " << des;
	}
	else
	{
		appendFrames(frames);
	}
}

void FCVideoTimelineWidget::onDecodeFinished()
{
	_loadingDialog.close();
}

void FCVideoTimelineWidget::onVideoFrameClicked()
{
	auto widget = qobject_cast<FCVideoFrameWidget*>(sender());
	_selected = widget;
	emit selectionChanged();

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