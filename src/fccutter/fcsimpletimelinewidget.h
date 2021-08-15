#pragma once

#include <QWidget>
#include "ui_fcsimpletimelinewidget.h"


class FCService;
class FCSimpleTimelineWidget : public QWidget
{
	Q_OBJECT

public:
	FCSimpleTimelineWidget(const QSharedPointer<FCService> &service, QWidget *parent = Q_NULLPTR);
	~FCSimpleTimelineWidget();

	void setCurrentStream(int streamIndex);

	double pos() const;

Q_SIGNALS:
	void seekRequest(double seconds);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
	Ui::FCSimpleTimelineWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex{ -1 };
	double _duration{ 0 }; // in seconds
	double _pos{ 0 };
};
