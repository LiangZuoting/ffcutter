#pragma once

#include <QWidget>
#include "ui_fcclipfilterwidget.h"

class FCClipFilterWidget : public QWidget
{
	Q_OBJECT

public:
	FCClipFilterWidget(QWidget *parent = Q_NULLPTR);
	~FCClipFilterWidget();

private:
	Ui::FCClipFilterWidget ui;
};
