#pragma once

#include <QWidget>
#include "ui_fcsimpletimelinewidget.h"
#include "fcconst.h"


class FCService;
class FCVideoFrameWidget;
class FCSimpleTimelineWidget : public QWidget
{
	Q_OBJECT

public:
	FCSimpleTimelineWidget(QWidget *parent = Q_NULLPTR);
	~FCSimpleTimelineWidget();

	void loadFile(const QString &filePath);

	void setCurrentStream(int streamIndex);

Q_SIGNALS:
	void seekRequest(double seconds);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void enterEvent(QEnterEvent*event) override;
	void leaveEvent(QEvent *event) override;

private Q_SLOTS:
	void onSeekFinished(int streamIndex, QList<FCFrame> frames, void *userData);
	void onFrameDecoded(QList<FCFrame> frames, void *userData);

private:
	Ui::FCSimpleTimelineWidget ui;
	QSharedPointer<FCService> _service;
	QScopedPointer<FCVideoFrameWidget> _frameWidget;
	int _streamIndex{ -1 };
	double _duration{ 0 }; // in seconds
	double _seekSeconds{ 0 };
	int _x{ 0 };
	bool _cursorIn{ false };
};
