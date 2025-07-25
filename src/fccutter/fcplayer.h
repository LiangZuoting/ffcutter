#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <QBuffer>
#include <QAudioOutput>
#include <qelapsedtimer.h>

extern "C"
{
#include <libavutil/frame.h>
}

class FCPlayer  : public QOpenGLWidget
{
    Q_OBJECT

public:
    FCPlayer(QWidget *parent);
    ~FCPlayer();

	void setup(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames);

protected:
    void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
	void onTimeout();

private:
	QBuffer _audioBuffer;
	QAudioOutput* _audioOutput;
	QVector<QPair<QPixmap, double>> _videoFrames;
	int _current{ 0 };
	double _currentPts{ 0 };
	double _currentTime{ 0 };
	QTimer _timer;
	QElapsedTimer _elapsedTimer;
};
