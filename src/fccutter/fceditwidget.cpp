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
	connect(ui.saveBtn, SIGNAL(clicked()), this, SLOT(onSaveClicked()));
	connect(ui.textColorBtn, SIGNAL(clicked()), this, SLOT(onTextColorClicked()));

	loadFontSize();
	loadFonts();
}

FCEditWidget::~FCEditWidget()
{
}

void FCEditWidget::setService(const QSharedPointer<FCService> &service)
{
	_service = service;
	connect(_service.data(), SIGNAL(seekFinished(int)), this, SLOT(onSeekFinished(int)));

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
	ui.fpsEdit->setText(QString::number(stream->avg_frame_rate.num / stream->avg_frame_rate.den));
	double msecs = (double)stream->duration * stream->time_base.num / stream->time_base.den * 1000;
	QTime t = QTime(0,0).addMSecs(msecs);
	ui.durationSecEdit->setTime(t);
}

void FCEditWidget::setStartSec(double startSec)
{
	QTime t = QTime(0, 0).addMSecs(startSec * 1000);
	ui.startSecEdit->setTime(t);
}

void FCEditWidget::onFastSeekClicked()
{
	if (_streamIndex < 0)
	{
		return;
	}

	_service->seekAsync(_streamIndex, FCUtil::durationSecs(QTime(0,0), ui.seekEdit->time()));
	_loadingDialog.exec2(tr(u8"跳转..."));
}

void FCEditWidget::onSaveClicked()
{
	auto stream = _service->stream(_streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		auto filePath = QFileDialog::getSaveFileName(this, tr("保存文件"), QString());
		if (!filePath.isEmpty())
		{
			FCMuxEntry muxEntry;
			muxEntry.filePath = filePath;
			muxEntry.startSec = FCUtil::durationSecs(QTime(0,0), ui.startSecEdit->time());
			muxEntry.durationSec = FCUtil::durationSecs(QTime(0,0), ui.durationSecEdit->time());
			muxEntry.vStreamIndex = _streamIndex;
			muxEntry.aStreamIndex = ui.audioComboBox->currentData().toInt();

			QString filters;
			makeScaleFilter(filters, muxEntry, stream);
			makeFpsFilter(filters, muxEntry, stream);
			makeTextFilter(filters);
			muxEntry.filterString = filters;
			_service->saveAsync(muxEntry);
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

void FCEditWidget::onSeekFinished(int streamIndex)
{
	_loadingDialog.close();
	emit seekFinished(streamIndex);
}

void FCEditWidget::loadFontSize()
{
	QFile f("fontsizes.txt");
	f.open(QFile::ReadOnly);
	QString text = f.readAll();
	ui.fontSizeComboBox->addItems(text.split(','));
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
	appendFilter(filters, QString("scale=width=%1:height=%2").arg(muxEntry.width).arg(muxEntry.height));
}

void FCEditWidget::makeFpsFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream)
{
	int srcFps = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
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

void FCEditWidget::appendFilter(QString &filters, const QString &newFilter)
{
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	filters.append(newFilter);
}