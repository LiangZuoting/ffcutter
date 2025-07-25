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

	void setVideoStreamIndex(int streamIndex);
	int videoStreamIndex() const
	{
		return _videoStreamIndex;
	}
	void setAudioStreamIndex(int streamIndex);
	int audioStreamIndex() const
	{
		return _audioStreamIndex;
	}
	void setService(const QSharedPointer<FCService>& service);

	double startSec() const;
	double endSec() const;

	void beginSelect();
	void endSelect();

	void appendFrames(const QList<FCFrame> &frames);

	void clear();

public Q_SLOTS:
	void decodeOnce();

private Q_SLOTS:
	void onForwardBtnClicked();
	void onFrameDecoded(QList<FCFrame> frames, void *userData);
	void onDecodeFinished(void *userData);
	void onVideoFrameLeftClicked();
	void onVideoFrameRightClicked();
	void onPlayClicked();

Q_SIGNALS:
	void startSelected();
	void endSelected();
	void startSelect(const QPoint &);
	void stopSelect(const QPoint &);

private:
	Ui::FCVideoTimelineWidget ui;
	QSharedPointer<FCService> _service;
	int _videoStreamIndex = -1;
	int _audioStreamIndex{ -1 };
	FCVideoFrameWidget* _startFrame = nullptr;
	FCVideoFrameWidget *_endFrame = nullptr;
	FCLoadingDialog _loadingDialog;
	QVector<AVFrame*> _audioFrames;
};
