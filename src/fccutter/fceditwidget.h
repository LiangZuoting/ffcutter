#pragma once

#include <QWidget>
#include <QSharedPointer>
#include "ui_fceditwidget.h"
#include "fcservice.h"
#include "fcloadingdialog.h"

class FCMainWidget;
class FCEditWidget : public QWidget
{
	Q_OBJECT

public:
	FCEditWidget(const QSharedPointer<FCService> &service, FCMainWidget *parent = Q_NULLPTR);
	~FCEditWidget();

	void setCurrentStream(int streamIndex);

	void setStartSec(double startSec);

Q_SIGNALS:
	void seekFinished(int streamIndex, QList<FCFrame> frames);

private Q_SLOTS:
	void onFastSeekClicked();
	void onExactSeekClicked();
	void onSaveClicked();
	void onTextColorClicked();
	void onSeekFinished(int streamIndex, QList<FCFrame> frames);
	void onSaveFinished();
	void onErrorOcurred();

private:
	void setService(const QSharedPointer<FCService> &service);
	void loadFontSize();
	void loadFonts();
	void makeScaleFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream);
	void makeFpsFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream);
	void makeTextFilter(QString &filters);
	void appendFilter(QString &filters, const QString &newFilter);

	Ui::FCEditWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	FCLoadingDialog _loadingDialog;
};
