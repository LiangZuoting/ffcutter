#pragma once

#include <QWidget>
#include "ui_fcvideotimelinewidget.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class FCVideoTimelineWidget : public QWidget
{
	Q_OBJECT

public:
	FCVideoTimelineWidget(QWidget *parent = Q_NULLPTR);
	~FCVideoTimelineWidget();

	void setStream(AVStream* stream);
	bool addPacket(AVPacket* packet);

private:
	inline static const int MAX_LIST_SIZE = 10;

	Ui::FCVideoTimelineWidget ui;
	AVStream* _stream = nullptr;
	AVCodecContext* _codecCtx = nullptr;
	using PFPair = QPair<AVPacket*, AVFrame*>;
	using PFList = QList<PFPair>;
	PFList _listFrame;
};
