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
#include "fcsimpletimelinewidget.h"

class FCMainWidget : public QWidget
{
	Q_OBJECT

public:
	FCMainWidget(QWidget *parent = Q_NULLPTR);
	~FCMainWidget();

	void openFile(const QString& filePath);
	void closeFile();

private Q_SLOTS:
	void onFileOpened(QList<AVStream *> streams, void *userData);
	void selectStreamItem(int streamIndex);
	void onStartFrameSelected();
	void onEndFrameSelected();
	void onErrorOcurred(void *userData);
	void onSeekFinished(int streamIndex, QList<FCFrame> frames, void *userData);
	void onStartSelect(const QPoint &pos);
	void onStopSelect(const QPoint &pos);

private:
	enum SelectType
	{
		NoneType,
		DelogoType,
		MasaicType,
	};

	Ui::FCMainWidget ui;
	QSharedPointer<FCService> _service;
	FCFileInfoWidget *_fiWidget = nullptr;
	FCEditWidget *_opWidget = nullptr;
	FCVideoTimelineWidget *_vTimelineWidget = nullptr;
	FCSimpleTimelineWidget *_simpleTimelineWidget{ nullptr };
	int _streamIndex = -1;
	FCLoadingDialog _loadingDialog;
	SelectType _selectType{ NoneType };
	QString _filePath;
};
