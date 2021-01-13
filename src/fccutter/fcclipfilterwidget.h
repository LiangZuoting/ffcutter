#pragma once

#include <QWidget>
#include "ui_fcclipfilterwidget.h"

class FCClipFilterWidget : public QWidget
{
	Q_OBJECT

public:
	FCClipFilterWidget(QWidget *parent = Q_NULLPTR);
	~FCClipFilterWidget();

	void setStartSec(double startSec);
	void setEndSec(double endInSec);

private Q_SLOTS:
	void onTextColorClicked();
	void onSubtitleBtnClicked();

private:
	void loadFontSize();
	void loadFonts();

	Ui::FCClipFilterWidget ui;
};
