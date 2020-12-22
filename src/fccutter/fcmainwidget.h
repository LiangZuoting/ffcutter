#pragma once

#include <QWidget>
#include "ui_fcmainwidget.h"
#include <fcservice.h>
#include "fcmuxentry.h"
extern "C"
{
#include <libavformat/avformat.h>
}
#include "fcloadingdialog.h"
#include "fceditwidget.h"
#include "fcfileinfowidget.h"
#include "fcvideotimelinewidget.h"

class FCMainWidget : public QWidget
{
	Q_OBJECT

public:
	FCMainWidget(QWidget *parent = Q_NULLPTR);
	~FCMainWidget();

	void openFile(const QString& filePath);
	void closeFile();

private Q_SLOTS:
	void onFileOpened(QList<AVStream *> streams);
	void selectStreamItem(int streamIndex);
	void onStartFrameSelected();
	void onEndFrameSelected();
	void onErrorOcurred();
	void onSeekFinished(int streamIndex, QList<FCFrame> frames);

private:
	Ui::FCMainWidget ui;
	QSharedPointer<FCService> _service;
	FCFileInfoWidget *_fiWidget = nullptr;
	FCEditWidget *_opWidget = nullptr;
	FCVideoTimelineWidget *_vTimelineWidget = nullptr;
	int _streamIndex = -1;
	FCLoadingDialog _loadingDialog;
};
