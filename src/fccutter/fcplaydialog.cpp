#include "fcplaydialog.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <qdatetime.h>
#include <QDebug>
#include <qelapsedtimer.h>

FCPlayDialog::FCPlayDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(&_timer, &QTimer::timeout, this, &FCPlayDialog::onTimeout);
}

FCPlayDialog::~FCPlayDialog()
{}

void FCPlayDialog::play(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames, int fps)
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

			//out.writeRawData(reinterpret_cast<const char*>(frame->data[0]), size);
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
	_fps = fps;
	_current = 0;
	_currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
	const auto& frame = videoFrames[_current];
	_currentPts = frame.second;
	ui.player->setPixmap(frame.first);
	_timer.start(10);
	_elapsedTimer.start();
	exec();
}

void FCPlayDialog::onTimeout()
{
	qDebug() << "time:" << _elapsedTimer.elapsed();
	_elapsedTimer.start();
	auto next = _current + 1;
	if (next < _videoFrames.size())
	{
		auto nextPts = _videoFrames[next].second;
		auto now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
		auto timeDelta = now - _currentTime;
		auto ptsDelta = nextPts - _currentPts;
		if (timeDelta >= ptsDelta)
		{
			_currentPts = nextPts;
			_current = next;
			_currentTime = now;
			ui.player->setPixmap(_videoFrames[_current].first);
		}
	}
}

