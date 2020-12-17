#include "fcmainwidget.h"
#include <QTextCodec>
#include <QDebug>
#include <QFileDialog>
#include <QColorDialog>
#include <QStandardPaths>
#include <QRawFont>
#include <libavutil/error.h>
#include "fcvideotimelinewidget.h"


FCMainWidget::FCMainWidget(QWidget *parent)
	: QWidget(parent)
	, _loadingDialog(this)
{
	ui.setupUi(this);

	connect(ui.fiWidget, SIGNAL(streamItemSelected(int)), this, SLOT(selectStreamItem(int)));
	connect(ui.fastSeekBtn, SIGNAL(clicked()), this, SLOT(onFastSeekClicked()));
	connect(ui.saveBtn, SIGNAL(clicked()), this, SLOT(onSaveClicked()));
	connect(ui.textColorBtn, SIGNAL(clicked()), this, SLOT(onTextColorClicked()));

	loadFontSize();
	loadFonts();
}

FCMainWidget::~FCMainWidget()
{
}

void FCMainWidget::openFile(const QString& filePath)
{
	closeFile();

	_service.reset(new FCService());
	connect(_service.data(), SIGNAL(fileOpened(QList<AVStream *>)), this, SLOT(onFileOpened(QList<AVStream *>)));
	connect(_service.data(), SIGNAL(saveFinished()), this, SLOT(onSaveFinished()));
	connect(_service.data(), SIGNAL(errorOcurred()), this, SLOT(onErrorOcurred()));
	_service->openFileAsync(filePath);
	showLoading(tr(u8"打开文件..."));
}

void FCMainWidget::closeFile()
{
	if (auto timelineWidget = findChild<FCVideoTimelineWidget *>(); timelineWidget)
	{
		ui.layout->removeWidget(timelineWidget);
		delete timelineWidget;
	}
	ui.fiWidget->reset();
	_streamIndex = -1;
	_service.reset();
}

void FCMainWidget::onFileOpened(QList<AVStream *> streams)
{
	ui.fiWidget->setService(_service);

	ui.audioComboBox->addItem(tr(u8"没有音频"), -1);
	for (int i = 0; i < streams.size(); ++i)
	{
		auto stream = streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ui.audioComboBox->addItem(tr(u8"音频 %1").arg(stream->index), stream->index);
		}
	}

	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " open file error " << des;
	}
	_loadingDialog.close();
}

void FCMainWidget::selectStreamItem(int streamIndex)
{
	_streamIndex = streamIndex;
	auto stream = _service->stream(streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		auto w = findChild<FCVideoTimelineWidget *>();
		if (w && w->streamIndex() == _streamIndex)
		{
			return;
		}
		if (w && w->streamIndex() != _streamIndex)
		{
			ui.layout->removeWidget(w);
			delete w;
		}
		ui.widthEdit->setText(QString::number(stream->codecpar->width));
		ui.heightEdit->setText(QString::number(stream->codecpar->height));
		ui.fpsEdit->setText(QString::number(stream->avg_frame_rate.num / stream->avg_frame_rate.den));

		FCVideoTimelineWidget *widget = new FCVideoTimelineWidget(this);
		connect(widget, SIGNAL(selectionChanged()), this, SLOT(onVideoFrameSelectionChanged()));
		ui.layout->addWidget(widget);
		widget->setStreamIndex(streamIndex);
		widget->setService(_service);
		widget->decodeOnce();
	}
}

void FCMainWidget::onFastSeekClicked()
{
	if (_streamIndex < 0)
	{
		return;
	}
	
	_service->seekAsync(_streamIndex, ui.seekEdit->text().toDouble());

	auto stream = _service->stream(_streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		FCVideoTimelineWidget *widget = findChild<FCVideoTimelineWidget*>();
		if (widget)
		{
			widget->decodeOnce();
		}
	}
}

void FCMainWidget::onSaveClicked()
{
	auto stream = _service->stream(_streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		auto filePath = QFileDialog ::getSaveFileName(this, tr("保存文件"), QString());
		if (!filePath.isEmpty())
		{
			FCMuxEntry muxEntry;
			muxEntry.filePath = filePath;
			muxEntry.startSec = ui.startSecEdit->text().toDouble();
			muxEntry.durationSec = ui.durationSecEdit->text().toDouble();
			muxEntry.vStreamIndex = _streamIndex;
			muxEntry.aStreamIndex = ui.audioComboBox->currentData().toInt();

			QString filters;
			makeScaleFilter(filters, muxEntry, stream);
			makeFpsFilter(filters, muxEntry, stream);
			makeTextFilter(filters);
			muxEntry.filterString = filters;
			_service->saveAsync(muxEntry);
			showLoading(tr(u8"保存..."));
		}
	}
}

void FCMainWidget::onTextColorClicked()
{
	auto color = QColorDialog::getColor(QColor(ui.textColorBtn->text()));
	ui.textColorBtn->setStyleSheet("background: " + color.name());
	ui.textColorBtn->setText(color.name());
}

void FCMainWidget::onVideoFrameSelectionChanged()
{
	auto widget = qobject_cast<FCVideoTimelineWidget*>(sender());
	if (widget)
	{
		ui.startSecEdit->setText(QString::number(widget->selectedSec()));
	}
}

void FCMainWidget::onSaveFinished()
{
	_loadingDialog.accept();
}

void FCMainWidget::onErrorOcurred()
{
	_loadingDialog.close();
}

void FCMainWidget::makeScaleFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream)
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

void FCMainWidget::makeFpsFilter(QString &filters, FCMuxEntry &muxEntry, const AVStream *stream)
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

void FCMainWidget::makeTextFilter(QString &filters)
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

void FCMainWidget::appendFilter(QString &filters, const QString &newFilter)
{
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	filters.append(newFilter);
}

void FCMainWidget::loadFontSize()
{
	QFile f("fontsizes.txt");
	f.open(QFile::ReadOnly);
	QString text = f.readAll();
	ui.fontSizeComboBox->addItems(text.split(','));
}

void FCMainWidget::loadFonts()
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

void FCMainWidget::showLoading(const QString &labelText)
{
	_loadingDialog.setLabelText(labelText);
	_loadingDialog.exec();
}