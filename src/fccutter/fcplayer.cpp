#include "fcplayer.h"
#include <QPainter>
#include <QDateTime>
#include <QDebug>

FCPlayer::FCPlayer(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&_timer, &QTimer::timeout, this, &FCPlayer::onTimeout);
}

FCPlayer::~FCPlayer()
{}

void FCPlayer::setup(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames)
{
	if (!audioFrames.isEmpty())
	{
		_audioBuffer.open(QIODevice::ReadWrite);
		QDataStream out(&_audioBuffer);
		for (const auto& frame : audioFrames)
		{
			auto format = static_cast<AVSampleFormat>(frame->format);
			auto bytesPerSample = av_get_bytes_per_sample(format);
			auto channels = frame->ch_layout.nb_channels;
			auto samples = frame->nb_samples;
			auto size = samples * bytesPerSample * channels;

			if (av_sample_fmt_is_planar(format))
			{
				QByteArray buffer(size, 0);
				for (auto i = 0; i < samples; ++i)
				{
					for (auto channel = 0; channel < channels; ++channel)
					{
						memcpy(buffer.data() + (i * channels + channel) * bytesPerSample, frame->data[channel] + i * bytesPerSample, bytesPerSample);
					}
				}
				out.writeRawData(buffer.data(), size);
			}
			else
			{
				out.writeRawData(reinterpret_cast<const char*>(frame->data[0]), size);
			}
		}

		QAudioFormat fmt;
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleRate(audioFrames[0]->sample_rate);
		fmt.setChannelCount(audioFrames[0]->ch_layout.nb_channels);
		auto sampleFormat = static_cast<AVSampleFormat>(audioFrames[0]->format);
		fmt.setSampleSize(av_get_bytes_per_sample(sampleFormat) * 8);
		if (sampleFormat == AV_SAMPLE_FMT_S32 || sampleFormat == AV_SAMPLE_FMT_S32P)
		{
			fmt.setSampleType(QAudioFormat::SignedInt);
		}
		else if (sampleFormat == AV_SAMPLE_FMT_FLT || sampleFormat == AV_SAMPLE_FMT_FLTP)
		{
			fmt.setSampleType(QAudioFormat::Float);
		}

		auto s = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
		if (info.isFormatSupported(fmt))
		{
			_audioOutput = new QAudioOutput(fmt, this);
			_audioBuffer.seek(0);
			_audioOutput->start(&_audioBuffer);
		}
	}

	_videoFrames = videoFrames;
	_current = 0;
	_currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
	const auto& frame = videoFrames[_current];
	_currentPts = frame.second;
	_timer.start(5);
	_elapsedTimer.start();
	update();
}

void FCPlayer::paintEvent(QPaintEvent* event)
{
	qDebug() << "time:" << _elapsedTimer.elapsed();
	_elapsedTimer.start();
	QPainter painter(this);

	const auto& pixmap = _videoFrames[_current].first;
	auto xRatio = 1.0 * width() / pixmap.width();
	auto yRatio = 1.0 * height() / pixmap.height();
	auto ratio = std::min(xRatio, yRatio);
	auto width = pixmap.width() * ratio;
	auto x = (this->width() - width) / 2;
	auto height = pixmap.height() * ratio;
	auto y = (this->height() - height) / 2;
	painter.drawPixmap(x, y, width, height, pixmap);
}


void FCPlayer::onTimeout()
{
	auto next = _current + 1;
	if (next < _videoFrames.size())
	{
		auto nextPts = _videoFrames[next].second;
		auto now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
		auto timeDelta = now - _currentTime;
		auto ptsDelta = nextPts - _currentPts;
		if (timeDelta >= ptsDelta)
		{
			qDebug() << "timeDelta:" << timeDelta << "ptsDelta:" << ptsDelta;
			_currentPts = nextPts;
			_current = next;
			_currentTime = now;
			update();
		}
	}
}

