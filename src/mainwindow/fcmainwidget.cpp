#include "fcmainwidget.h"
#include <QTextCodec>
#include <QDebug>
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
	connect(_service.data(), SIGNAL(fileOpened(QList<AVStream *>)), this, SLOT(onFileOpened(QList<AVStream *>)));
	_service->openFileAsync(filePath);
}

void FCMainWidget::onFileOpened(QList<AVStream *> streams)
{
	ui.fiWidget->setService(_service);
	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " open file error " << des;
	}
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