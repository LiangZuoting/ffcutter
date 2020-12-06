#include "fcmainwidget.h"
#include <QTextCodec>
#include <libavutil/error.h>
#include "fcvideotimelinewidget.h"

FCMainWidget::FCMainWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.fiWidget, SIGNAL(parseStream(AVStream*)), this, SLOT(decodeStream(AVStream*)));
}

FCMainWidget::~FCMainWidget()
{
}

void FCMainWidget::openFile(const QString& filePath)
{
	int ret = 0;
	auto url = filePath.toStdString();
	if (ret = avformat_open_input(&_fmtCtx, url.data(), nullptr, nullptr))
	{
		assert(0);
		return;
	}
	if (ret = avformat_find_stream_info(_fmtCtx, nullptr))
	{
		assert(0);
		return;
	}
	ui.fiWidget->setFile(_fmtCtx);
	for (unsigned i = 0; i < _fmtCtx->nb_streams; ++i)
	{
		auto s = _fmtCtx->streams[i];
		ui.fiWidget->addStream(s);
	}
}

void FCMainWidget::decodeStream(AVStream* stream)
{
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		int ret = av_seek_frame(_fmtCtx, stream->index, 60 * 1000 * 20, AVSEEK_FLAG_BACKWARD);

		FCVideoTimelineWidget* widget = new FCVideoTimelineWidget(this);
		ui.layout->addWidget(widget);
		widget->setStream(stream);
		AVPacket* packet = av_packet_alloc();
		for (int i = 0; i < 10;)
		{
			ret = av_read_frame(_fmtCtx, packet);
			if (packet->stream_index == stream->index)
			{
				if (widget->addPacket(packet))
				{
					++i;
				}
			}
		}
		av_packet_unref(packet);
	}
}