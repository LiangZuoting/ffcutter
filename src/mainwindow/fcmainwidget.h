#pragma once

#include <QWidget>
#include "ui_fcmainwidget.h"
#include <fcservice.h>
#include "fcmuxentry.h"
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
	void onFileOpened(QList<AVStream *> streams);
	void onStreamItemSelected(int streamIndex);
	void onFastSeekClicked();
	void onSetStartClicked();
	void onGifClicked();

private:
	Ui::FCMainWidget ui;

	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCMuxEntry _muxEntry;
};
