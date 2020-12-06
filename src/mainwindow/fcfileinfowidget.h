#pragma once

#include <QWidget>
#include "ui_fcfileinfowidget.h"
#include <QTreeWidgetItem>
extern "C"
{
#include <libavformat/avformat.h>
}

class FCFileInfoWidget : public QWidget
{
	Q_OBJECT

	inline static const int FFDataRole = Qt::UserRole + 1;

public:
	FCFileInfoWidget(QWidget *parent = Q_NULLPTR);
	~FCFileInfoWidget();

	void setFile(AVFormatContext *fmtCtx);

	void addStream(AVStream* stream);

Q_SIGNALS:
	void parseStream(AVStream* stream);

private Q_SLOTS:
	void onItemSelectionChanged();

private:
	Ui::FCFileInfoWidget ui;
	QTreeWidgetItem* _rootItem = nullptr;
};