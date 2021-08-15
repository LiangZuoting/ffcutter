#include "fceditwidget.h"
#include <QFileDialog>
#include <QColorDialog>
#include <QRawFont>
#include <QTime>
#include "fcmainwidget.h"
#include "fcutil.h"

FCEditWidget::FCEditWidget(const QSharedPointer<FCService> &service, FCMainWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setService(service);

	connect(ui.fastSeekBtn, SIGNAL(clicked()), this, SLOT(onFastSeekClicked()));
	connect(ui.exactSeekBtn, SIGNAL(clicked()), this, SLOT(onExactSeekClicked()));
	connect(ui.saveBtn, SIGNAL(clicked()), this, SLOT(onSaveClicked()));
	connect(ui.textColorBtn, SIGNAL(clicked()), this, SLOT(onTextColorClicked()));
	connect(ui.subtitleBtn, SIGNAL(clicked()), this, SLOT(onSubtitleBtnClicked()));

	loadFontSize();
	loadFonts();
}

FCEditWidget::~FCEditWidget()
{
}

void FCEditWidget::setService(const QSharedPointer<FCService> &service)
{
	_service = service;
	connect(_service.data(), SIGNAL(seekFinished(int, QList<FCFrame>, void *)), this, SLOT(onSeekFinished(int, QList<FCFrame>, void *)));
	connect(_service.data(), SIGNAL(saveFinished(void *)), this, SLOT(onSaveFinished(void *)));
	connect(_service.data(), SIGNAL(errorOcurred(void *)), this, SLOT(onErrorOcurred(void *)));

	auto streams = _service->streams();
	ui.audioComboBox->addItem(tr(u8"没有音频"), -1);
	for (int i = 0; i < streams.size(); ++i)
	{
		auto stream = streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ui.audioComboBox->addItem(tr(u8"音频 %1").arg(stream->index), stream->index);
		}
	}
}

void FCEditWidget::setCurrentStream(int streamIndex)
{
	_streamIndex = streamIndex;
	auto stream = _service->stream(streamIndex);
	ui.widthEdit->setText(QString::number(stream->codecpar->width));
	ui.heightEdit->setText(QString::number(stream->codecpar->height));
	int fps = av_q2d(stream->avg_frame_rate) + 0.5;
	ui.fpsEdit->setText(QString::number(fps));
	if (stream->duration >= 0)
	{
		double msecs = (double)stream->duration * av_q2d(stream->time_base) * 1000;
		QTime t = QTime(0, 0).addMSecs(msecs);
		ui.endSecEdit->setTime(t);
	}
}

void FCEditWidget::setStartSec(double startSec)
{
	QTime t = QTime(0, 0).addMSecs(startSec * 1000);
	ui.startSecEdit->setTime(t);
}

void FCEditWidget::setEndSec(double endInSec)
{
	QTime t = QTime(0, 0).addMSecs(endInSec * 1000);
	ui.endSecEdit->setTime(t);
}

void FCEditWidget::fastSeek(double seconds)
{
	QTime t = QTime(0, 0).addMSecs(seconds * 1000);
	ui.seekEdit->setTime(t);

	onFastSeekClicked();
}

void FCEditWidget::onFastSeekClicked()
{
	if (_streamIndex < 0)
	{
		return;
	}

	_service->fastSeekAsync(_streamIndex, FCUtil::durationSecs(QTime(0,0), ui.seekEdit->time()), this);
	_loadingDialog.exec2(tr(u8"跳转..."));
}

void FCEditWidget::onExactSeekClicked()
{
	if (_streamIndex < 0)
	{
		return;
	}

	_service->exactSeekAsync(_streamIndex, FCUtil::durationSecs(QTime(0, 0), ui.seekEdit->time()), this);
	_loadingDialog.exec2(tr(u8"跳转..."));
}

void FCEditWidget::onSaveClicked()
{
	auto stream = _service->stream(_streamIndex);
	auto aStreamIndex = ui.audioComboBox->currentData().toInt();
	auto aStream = _service->stream(aStreamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		auto filePath = QFileDialog::getSaveFileName(this, tr("保存文件"), QString());
		if (!filePath.isEmpty())
		{
			FCMuxEntry muxEntry;
			muxEntry.filePath = filePath;
			muxEntry.startSec = FCUtil::durationSecs(QTime(0,0), ui.startSecEdit->time());
			muxEntry.endSec = FCUtil::durationSecs(QTime(0,0), ui.endSecEdit->time());
			muxEntry.vStreamIndex = _streamIndex;
			muxEntry.pixelFormat = (AVPixelFormat)stream->codecpar->format;
			muxEntry.vBitrate = stream->codecpar->bit_rate;
			muxEntry.aStreamIndex = aStreamIndex;
			if (aStream)
			{
				muxEntry.sampleFormat = (AVSampleFormat)aStream->codecpar->format;
				muxEntry.aBitrate = aStream->codecpar->bit_rate;
				muxEntry.sampleRate = aStream->codecpar->sample_rate;
				muxEntry.channel_layout = aStream->codecpar->channel_layout;
				muxEntry.channels = aStream->codecpar->channels;
			}

			QString vFilters;
			makeCropFilter(vFilters);
			makeScaleFilter(vFilters, muxEntry, stream);
			makeFpsFilter(vFilters, muxEntry, stream);
			makeTextFilter(vFilters);
			makeSubtitleFilter(vFilters);
			muxEntry.vFilterString = vFilters;
			_service->saveAsync(muxEntry, this);
			_loadingDialog.exec2(tr(u8"保存..."));
		}
	}
}

void FCEditWidget::onTextColorClicked()
{
	auto color = QColorDialog::getColor(QColor(ui.textColorBtn->text()));
	ui.textColorBtn->setStyleSheet("background: " + color.name());
	ui.textColorBtn->setText(color.name());
}

void FCEditWidget::onSubtitleBtnClicked()
{
	QString srtFile = QFileDialog::getOpenFileName(this, tr("choose srt file"), QString(), "字幕文件 (*.srt)");
	if (!srtFile.isEmpty())
	{
		ui.subtitleEdit->setText(srtFile);
	}
}

void FCEditWidget::onSeekFinished(int streamIndex, QList<FCFrame> frames, void *userData)
{
	if (userData == this)
	{
		_loadingDialog.close();
		emit seekFinished(streamIndex, frames, userData);
	}
}

void FCEditWidget::onSaveFinished(void *userData)
{
	_loadingDialog.accept();
}

void FCEditWidget::onErrorOcurred(void *userData)
{
	_loadingDialog.close();
}

void FCEditWidget::loadFontSize()
{
	QFile f("fontsizes.txt");
	f.open(QFile::ReadOnly);
	QString text = f.readAll();
	auto ls = text.split(',');
	ui.fontSizeComboBox->addItems(ls);
	ui.stFontSizeComboBox->addItems(ls);
}

void FCEditWidget::loadFonts()
{
	QDir fontsDir("fonts");
	auto ls = fontsDir.entryInfoList(QDir::Files);
	for (int i = 0; i < ls.size(); ++i)
	{
		auto filePath = ls[i].absoluteFilePath();
		QRawFont rawFont(filePath, 10);
		ui.fontComboBox->addItem(rawFont.familyName(), filePath);
	}
}

void FCEditWidget::makeCropFilter(QString &filters)
{
	int width = ui.cropWidthEdit->text().toInt();
	int height = ui.cropHeightEdit->text().toInt();
	if (width <= 0 || height <= 0)
	{
		return;
	}
	appendFilter(filters, QString("crop=x=%1:y=%2:w=%3:h=%4").arg(ui.cropXEdit->text().toInt())
		.arg(ui.cropYEdit->text().toInt()).arg(width).arg(height));
}

void FCEditWidget::makeScaleFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream)
{
	int srcWidth = stream->codecpar->width;
	int srcHeight = stream->codecpar->height;

	muxEntry.width = ui.widthEdit->text().toInt();
	if (muxEntry.width <= 0)
	{
		muxEntry.width = srcWidth;
	}
	muxEntry.height = ui.heightEdit->text().toInt();
	if (muxEntry.height <= 0)
	{
		muxEntry.height = srcHeight;
	}
	QSize scaledSize;
	if (ui.scaleAspectBtn->isChecked())
	{
		scaledSize = QSize(srcWidth, srcHeight).scaled(muxEntry.width, muxEntry.height, Qt::KeepAspectRatio);
	}
	appendFilter(filters, QString("scale=width=%1:height=%2,pad=%3:%4:-1:-1").arg(scaledSize.width()).arg(scaledSize.height()).arg(muxEntry.width).arg(muxEntry.height));
}

void FCEditWidget::makeFpsFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream)
{
	int srcFps = av_q2d(stream->avg_frame_rate) + 0.5;
	muxEntry.fps = ui.fpsEdit->text().toInt();
	if (muxEntry.fps <= 0)
	{
		muxEntry.fps = srcFps;
	}
	if (muxEntry.fps != srcFps)
	{
		appendFilter(filters, QString("fps=fps=%1").arg(muxEntry.fps));
	}
}

void FCEditWidget::makeTextFilter(QString &filters)
{
	auto text = ui.textEdit->toPlainText().trimmed();
	if (!text.isEmpty())
	{
		text = text.toUtf8();
		QString fontFile = ui.fontComboBox->currentData().toString().toUtf8();
		fontFile = fontFile.replace(':', "\\\\:");
		int fontSize = ui.fontSizeComboBox->currentText().toInt();
		QColor fontColor(ui.textColorBtn->text());
		QString x = "0";
		QString y = "0";
		if (ui.alignHCenterBtn->isChecked())
		{
			x = "(w-text_w)/2";
		}
		else if (ui.alignRightBtn->isChecked())
		{
			x = "w-text_w";
		}
		if (ui.alignVCenterBtn->isChecked())
		{
			y = "(h-text_h)/2";
		}
		else if (ui.alignBottomBtn->isChecked())
		{
			y = "h-text_h";
		}
		appendFilter(filters, QString("drawtext=fontfile=%1:fontsize=%2:fontcolor=%3:text=\'%4\':x=%5:y=%6")
			.arg(fontFile).arg(fontSize).arg(fontColor.name()).arg(text).arg(x).arg(y));
	}
}

void FCEditWidget::makeSubtitleFilter(QString& filters)
{
	auto srtFile = ui.subtitleEdit->text();
	if (srtFile.isEmpty())
	{
		return;
	}
	srtFile = srtFile.replace(':', "\\\\:");
	appendFilter(filters, QString("subtitles=filename=%1:force_style=\'fontsize=%2\'").arg(srtFile).arg(ui.stFontSizeComboBox->currentText().toInt()));
}

void FCEditWidget::makeCustomFilter(QString &filters)
{
	appendFilter(filters, ui.filterEdit->text());
}

void FCEditWidget::appendFilter(QString &filters, const QString &newFilter)
{
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	filters.append(newFilter);
}