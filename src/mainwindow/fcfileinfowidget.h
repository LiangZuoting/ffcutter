#pragma once

#include <QWidget>
#include "ui_fcfileinfowidget.h"
#include <QTreeWidgetItem>
#include <QSharedPointer>
#include <fcservice.h>
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

	void setService(const QSharedPointer<FCService>& service);

Q_SIGNALS:
	void streamItemSelected(int streamIndex);

private Q_SLOTS:
	void onItemSelectionChanged();

private:
	Ui::FCFileInfoWidget ui;
	QTreeWidgetItem* _rootItem = nullptr;
	QSharedPointer<FCService> _service;
};