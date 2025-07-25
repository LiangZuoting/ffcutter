#pragma once

#include <QAudioOutput>
#include <QDialog>
#include "ui_fcplaydialog.h"
#include <QTimer>
#include <QBuffer>
#include <qelapsedtimer.h>

extern "C"
{
#include <libavutil/frame.h>
}

class FCPlayDialog : public QDialog
{
	Q_OBJECT

public:
	FCPlayDialog(QWidget *parent = nullptr);
	~FCPlayDialog();

	void play(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames, int fps);

private Q_SLOTS:
	void onTimeout();

private:
	Ui::FCPlayDialogClass ui;
	QBuffer _audioBuffer;
	QAudioOutput* _audioOutput;
	QVector<QPair<QPixmap, double>> _videoFrames;
	int _current{ 0 };
	double _currentPts{ 0 };
	double _currentTime{ 0 };
	int _fps{ 0 };
	QTimer _timer;
	QElapsedTimer _elapsedTimer;
};

