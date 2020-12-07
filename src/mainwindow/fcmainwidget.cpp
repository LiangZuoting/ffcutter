#include "fcmainwidget.h"
#include <QTextCodec>
#include <libavutil/error.h>
#include "fcvideotimelinewidget.h"

FCMainWidget::FCMainWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.fiWidget, SIGNAL(streamItemSelected(int)), this, SLOT(onStreamItemSelected(int)));
}

FCMainWidget::~FCMainWidget()
{
}

void FCMainWidget::openFile(const QString& filePath)
{
	_service.reset(new FCService());
	auto f = _service->openFile(filePath);
	f.result();
	ui.fiWidget->setService(_service);
}

void FCMainWidget::onStreamItemSelected(int streamIndex)
{
	auto stream = _service->stream(streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		FCVideoTimelineWidget* widget = new FCVideoTimelineWidget(this);
		ui.layout->addWidget(widget);
		widget->setStreamIndex(streamIndex);
		widget->setService(_service);
		widget->decodeOnce();
	}
}