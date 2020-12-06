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

void FCVideoTimelineWidget::setStream(AVStream* stream)
{
	_stream = stream;
	AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
	_codecCtx = avcodec_alloc_context3(codec);
	int ret = avcodec_parameters_to_context(_codecCtx, stream->codecpar);
	AVDictionary* opts = nullptr;
	ret = avcodec_open2(_codecCtx, codec, &opts);
}

bool FCVideoTimelineWidget::addPacket(AVPacket* packet)
{
	while (_listFrame.size() >= MAX_LIST_SIZE)
	{
		auto pfPair = _listFrame.front();
		av_packet_unref(pfPair.first);
		av_frame_unref(pfPair.second);
		_listFrame.pop_front();
	}
	int ret = avcodec_send_packet(_codecCtx, packet);
	AVFrame* frame = av_frame_alloc();
	ret = avcodec_receive_frame(_codecCtx, frame);
	if (ret)
	{
		return false;
	}
	_listFrame.append({ packet, frame });

	FCVideoFrameThemeWidget* widget = new FCVideoFrameThemeWidget(this);
	ui.timelineLayout->addWidget(widget);
	widget->setFrame(frame);
	return true;
}