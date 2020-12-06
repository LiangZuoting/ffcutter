#pragma once

#include <QWidget>
#include "ui_fcmainwidget.h"
extern "C"
{
#include <libavformat/avformat.h>
}

class FCMainWidget : public QWidget
{
	Q_OBJECT

public:
	FCMainWidget(QWidget *parent = Q_NULLPTR);
	~FCMainWidget();

	void openFile(const QString& filePath);

private Q_SLOTS:
	void decodeStream(AVStream* stream);

private:
	Ui::FCMainWidget ui;

	// ffmpeg variables
	AVFormatContext* _fmtCtx = nullptr;
};
