#pragma once

#include <QWidget>
#include "ui_fcvideoframewidget.h"
#include "fcservice.h"
extern "C"
{
#include <libavutil/frame.h>
}

class FCVideoFrameWidget : public QWidget
{
	Q_OBJECT

public:
	FCVideoFrameWidget(QWidget *parent = Q_NULLPTR);
	~FCVideoFrameWidget();

	void setService(const QSharedPointer<FCService>& service);
	void setStreamIndex(int streamIndex);
	void setFrame(AVFrame* frame);

private Q_SLOTS:
	void onScaleFinished(QPixmap pixmap);

private:
	Ui::FCVideoFrameWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	AVFrame* _frame = nullptr;
};
