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

	int64_t selectedPts() const;

public Q_SLOTS:
	void decodeOnce();

private Q_SLOTS:
	void onFrameDecoded(QList<FCFrame> frames);
	void onDecodeFinished();
	void onVideoFrameClicked();

Q_SIGNALS:
	void selectionChanged();

private:
	void clear();

	inline static const int MAX_LIST_SIZE = 20;

	Ui::FCVideoTimelineWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCVideoFrameWidget* _selected = nullptr;
};
