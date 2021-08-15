#include "fcsimpletimelinewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <fcservice.h>

FCSimpleTimelineWidget::FCSimpleTimelineWidget(const QSharedPointer<FCService> &service, QWidget *parent)
	: QWidget(parent)
	, _service(service)
{
	ui.setupUi(this);
}

FCSimpleTimelineWidget::~FCSimpleTimelineWidget()
{
}

void FCSimpleTimelineWidget::setCurrentStream(int streamIndex)
{
	_streamIndex = streamIndex;
	auto stream = _service->stream(streamIndex);
	_duration = stream->duration * av_q2d(stream->time_base);
}

double FCSimpleTimelineWidget::pos() const
{
	return _pos;
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

}

void FCSimpleTimelineWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	_pos = _duration * event->x() / width();
	emit seekRequest(_pos);
}