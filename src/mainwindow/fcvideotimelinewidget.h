#pragma once

#include <QWidget>
#include "ui_fcvideotimelinewidget.h"
#include "fcvideoframewidget.h"
#include "fcservice.h"
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

	void setStreamIndex(int streamIndex);
	void setService(const QSharedPointer<FCService>& service);

	void decodeOnce();

	int64_t selectedPts() const;

private Q_SLOTS:
	void onFrameDecoded(QList<AVFrame*> frames);
	void onDecodeFinished();
	void onVideoFrameClicked();

private:
	void clear();

	inline static const int MAX_LIST_SIZE = 20;

	Ui::FCVideoTimelineWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCVideoFrameWidget* _selected = nullptr;
};
