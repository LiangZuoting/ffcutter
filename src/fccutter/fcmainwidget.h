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
	void onFastSeekClicked();
	void onSaveClicked();
	void onTextColorClicked();
	void onVideoFrameSelectionChanged();
	void onSaveFinished();
	void onErrorOcurred();

private:
	void makeScaleFilter(QString &filters, FCMuxEntry& muxEntry, const AVStream *stream);
	void makeFpsFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream);
	void makeTextFilter(QString& filters);
	void appendFilter(QString &filters, const QString &newFilter);
	void loadFontSize();
	void loadFonts();
	void showLoading(const QString &labelText);

	Ui::FCMainWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCLoadingDialog _loadingDialog;
};
