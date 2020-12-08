#pragma once

#include <QWidget>
#include "ui_fcvideoframethemewidget.h"
#include "fcservice.h"
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

	void setService(const QSharedPointer<FCService>& service);
	void setFrame(AVFrame* frame);

private Q_SLOTS:
	void onScaleFinished(QPixmap pixmap);

private:
	Ui::FCVideoFrameThemeWidget ui;
	QSharedPointer<FCService> _service;
	AVFrame* _frame = nullptr;
};
