#pragma once

#include <QWidget>
#include "ui_fcvideoframewidget.h"
#include "fcservice.h"
extern "C"
{
#include <libavutil/frame.h>
}

class FCVideoFrameWidget : public QWidget
{
	Q_OBJECT

public:
	FCVideoFrameWidget(QWidget *parent = Q_NULLPTR);
	~FCVideoFrameWidget();

	void setService(const QSharedPointer<FCService>& service);
	void setStreamIndex(int streamIndex);
	void setFrame(AVFrame* frame);
	void setStart(bool select);
	void setEnd(bool select);

	AVFrame* frame() const;
	int64_t pts() const;
	double sec() const;

	void beginSelect();
	void endSelect();

protected:
	void mouseDoubleClickEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

Q_SIGNALS:
	void leftDoubleClicked();
	void rightDoubleClicked();
	void startSelect(const QPoint &);
	void stopSelect(const QPoint &);

private Q_SLOTS:
	void onScaleFinished(AVFrame *src, QPixmap scaled, void *userData);

private:
	Ui::FCVideoFrameWidget ui;
	QSharedPointer<FCService> _service;
	int _streamIndex = -1;
	AVFrame* _frame = nullptr;
	bool _isStart = false;
	bool _isEnd = false;
	bool _selecting{ false };
};
