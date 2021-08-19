#include "fcvideotimelinewidget.h"
#include <QDebug>
#include <QScrollBar>

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
	connect(_service.data(), SIGNAL(frameDeocded(QList<FCFrame>, void *)), this, SLOT(onFrameDecoded(QList<FCFrame>, void *)));
	connect(_service.data(), SIGNAL(decodeFinished(void *)), this, SLOT(onDecodeFinished(void *)));
}

void FCVideoTimelineWidget::decodeOnce()
{
	_service->decodePacketsAsync(_streamIndex, MAX_LIST_SIZE, this);
	_loadingDialog.exec2(tr(u8"½âÂë..."));
}

double FCVideoTimelineWidget::startSec() const
{
	if (_startFrame)
	{
		return _startFrame->sec();
	}
	return 0;
}

double FCVideoTimelineWidget::endSec() const
{
	if (_endFrame)
	{
		return _endFrame->sec();
	}
	return 0;
}

void FCVideoTimelineWidget::beginSelect()
{
	auto children = findChildren<FCVideoFrameWidget *>();
	for (auto child : children)
	{
		child->beginSelect();
	}
	setCursor(Qt::CrossCursor);
}

void FCVideoTimelineWidget::endSelect()
{
	auto children = findChildren<FCVideoFrameWidget *>();
	for (auto child : children)
	{
		child->endSelect();
	}
	setCursor(Qt::ArrowCursor);
}

void FCVideoTimelineWidget::appendFrames(const QList<FCFrame>& frames)
{
	for (auto frame : frames)
	{
		if (_streamIndex == frame.streamIndex)
		{
			FCVideoFrameWidget* widget = new FCVideoFrameWidget(this);
			connect(widget, SIGNAL(leftDoubleClicked()), SLOT(onVideoFrameLeftClicked()));
			connect(widget, SIGNAL(rightDoubleClicked()), SLOT(onVideoFrameRightClicked()));
			connect(widget, SIGNAL(startSelect(const QPoint &)), SIGNAL(startSelect(const QPoint &)));
			connect(widget, SIGNAL(stopSelect(const QPoint &)), SIGNAL(stopSelect(const QPoint &)));
			ui.timelineLayout->addWidget(widget);
			widget->setService(_service);
			widget->setStreamIndex(_streamIndex);
			widget->setFrame(frame.frame);
		}
	}
}

void FCVideoTimelineWidget::onForwardBtnClicked()
{
	auto hsb = ui.scrollArea->horizontalScrollBar();
	if (!hsb)
	{
		return;
	}
	if (hsb->value() != hsb->maximum())
	{
		hsb->setValue(hsb->maximum());
	}
	else
	{
		clear();
		decodeOnce();
	}
}

void FCVideoTimelineWidget::onFrameDecoded(QList<FCFrame> frames, void *userData)
{
	if (userData != this)
	{
		return;
	}

	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " decode frame error " << des;
	}
	else
	{
		appendFrames(frames);
	}
}

void FCVideoTimelineWidget::onDecodeFinished(void *userData)
{
	_loadingDialog.close();
}

void FCVideoTimelineWidget::onVideoFrameLeftClicked()
{
	auto widget = qobject_cast<FCVideoFrameWidget*>(sender());
	_startFrame = widget;
	emit startSelected();

	if (auto children = findChildren<FCVideoFrameWidget*>(); !children.isEmpty())
	{
		for (auto c : children)
		{
			c->setStart(c == widget);
		}
	}
}

void FCVideoTimelineWidget::onVideoFrameRightClicked()
{
	auto widget = qobject_cast<FCVideoFrameWidget *>(sender());
	_endFrame = widget;
	emit endSelected();

	if (auto children = findChildren<FCVideoFrameWidget *>(); !children.isEmpty())
	{
		for (auto c : children)
		{
			c->setEnd(c == widget);
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