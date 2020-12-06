#pragma once

#include <QWidget>
#include "ui_fcvideoframethemewidget.h"
extern "C"
{
#include <libavutil/frame.h>
}

class FCVideoFrameThemeWidget : public QWidget
{
	Q_OBJECT

public:
	FCVideoFrameThemeWidget(QWidget *parent = Q_NULLPTR);
	~FCVideoFrameThemeWidget();

	void setFrame(AVFrame* frame);

private:
	Ui::FCVideoFrameThemeWidget ui;
	AVFrame* _frame = nullptr;
};
