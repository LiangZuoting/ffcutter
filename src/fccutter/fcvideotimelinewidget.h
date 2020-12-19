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
#include "fcloadingdialog.h"

class FCVideoTimelineWidget : public QWidget
{
	Q_OBJECT

public:
	FCVideoTimelineWidget(QWidget *parent = Q_NULLPTR);
	~FCVideoTimelineWidget();

	void setStreamIndex(int streamIndex);
	int streamIndex() const
	{
		return _streamIndex;
	}
	void setService(const QSharedPointer<FCService>& service);

	double selectedSec() const;

	void appendFrames(const QList<FCFrame> &frames);

	void clear();

public Q_SLOTS:
	void decodeOnce();

private Q_SLOTS:
	void onForwardBtnClicked();
	void onFrameDecoded(QList<FCFrame> frames);
	void onDecodeFinished();
	void onVideoFrameClicked();

Q_SIGNALS:
	void selectionChanged();

private:
	inline static const int MAX_LIST_SIZE = 20;

	Ui::FCVideoTimelineWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCVideoFrameWidget* _selected = nullptr;
	FCLoadingDialog _loadingDialog;
};
