#include "fcplayer.h"
#include <QPainter>
#include <QDateTime>
#include <QDebug>

FCPlayer::FCPlayer(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&_timer, &QTimer::timeout, this, &FCPlayer::onTimeout);

	_dumpLabel = new QLabel(parent);
	_dumpLabel->move(8, 0);
	_dumpLabel->setStyleSheet("QLabel { color: red; font-size: 14pt; }");
}

FCPlayer::~FCPlayer()
{}

void FCPlayer::setup(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames)
{
	if (!audioFrames.isEmpty())
	{
		const auto& frame = audioFrames[0];
		auto sampleFormat = static_cast<AVSampleFormat>(frame->format);
		auto bytesPerSample = av_get_bytes_per_sample(sampleFormat);
		auto channels = frame->ch_layout.nb_channels;
		auto samples = frame->nb_samples;
		auto sampleRate = frame->sample_rate;

		_audioBuffer.open(QIODevice::ReadWrite);
		QDataStream stream(&_audioBuffer);
		for (const auto& frame : audioFrames)
		{
			if (av_sample_fmt_is_planar(sampleFormat))
			{
				for (auto i = 0; i < samples; ++i)
				{
					for (auto channel = 0; channel < channels; ++channel)
					{
						stream.writeRawData(reinterpret_cast<char*>(frame->data[channel]) + i * bytesPerSample, bytesPerSample);
					}
				}
			}
			else
			{
				auto size = samples * bytesPerSample * channels;
				stream.writeRawData(reinterpret_cast<const char*>(frame->data[0]), size);
			}
		}

		QAudioFormat audioFormat;
		audioFormat.setCodec("audio/pcm");
		audioFormat.setByteOrder(QAudioFormat::LittleEndian);
		audioFormat.setSampleRate(sampleRate);
		audioFormat.setChannelCount(channels);
		audioFormat.setSampleSize(av_get_bytes_per_sample(sampleFormat) * 8);
		if (sampleFormat == AV_SAMPLE_FMT_S32 || sampleFormat == AV_SAMPLE_FMT_S32P)
		{
			audioFormat.setSampleType(QAudioFormat::SignedInt);
		}
		else if (sampleFormat == AV_SAMPLE_FMT_FLT || sampleFormat == AV_SAMPLE_FMT_FLTP)
		{
			audioFormat.setSampleType(QAudioFormat::Float);
		}

		if (QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice()); info.isFormatSupported(audioFormat))
		{
			_audioOutput = new QAudioOutput(audioFormat, this);
			connect(_audioOutput, &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
				if (state == QAudio::IdleState)
				{
					_audioOutput->stop();
					emit finished();
				}
				});
		}
	}

	_videoFrames = videoFrames;
}

void FCPlayer::start()
{
	if (_audioOutput)
	{
		_audioBuffer.seek(0);
		_audioOutput->start(&_audioBuffer);
	}
	_current = 0;
	_currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
	const auto& frame = _videoFrames[_current];
	_currentPts = frame.second;
	_timer.start(5);
	_dumpLabel->setText(QString::number(_current + 1));
	update();
}

void FCPlayer::setVolume(int value)
{
	if (_audioOutput)
	{
		auto volume = value / 100.0;
		if (volume < 0.0) volume = 0.0;
		else if (volume > 1.0) volume = 1.0;
		_audioOutput->setVolume(volume);
	}
}

void FCPlayer::paintEvent(QPaintEvent* event)
{
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
			_currentPts = nextPts;
			_current = next;
			_currentTime = now;
			_dumpLabel->setText(QString::number(_current + 1));
			update();
		}
	}
	else
	{
		_timer.stop();
		emit finished();
	}
}

