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
{
	ui.setupUi(this);

	connect(ui.fiWidget, SIGNAL(streamItemSelected(int)), this, SLOT(onStreamItemSelected(int)));
	connect(ui.fastSeekBtn, SIGNAL(clicked()), this, SLOT(onFastSeekClicked()));
	connect(ui.saveBtn, SIGNAL(clicked()), this, SLOT(onSaveClicked()));
	connect(ui.textColorBtn, SIGNAL(clicked()), this, SLOT(onTextColorClicked()));

	ui.durationUnitComboBox->addItems({ u8"Ãë", u8"Ö¡" });
	ui.fontSizeComboBox->addItems({ "10", "12", "14", "16", "18", "24", "32", "48", "64" });

	QDir fontsDir("fonts");
	auto ls = fontsDir.entryInfoList(QDir::Files);
	for (int i = 0; i < ls.size(); ++i)
	{
		auto filePath = ls[i].absoluteFilePath();
		QRawFont rawFont(filePath, 10);
		ui.fontComboBox->addItem(rawFont.familyName(), filePath);
	}
}

FCMainWidget::~FCMainWidget()
{
}

void FCMainWidget::openFile(const QString& filePath)
{
	_service.reset(new FCService());
	connect(_service.data(), SIGNAL(fileOpened(QList<AVStream *>)), this, SLOT(onFileOpened(QList<AVStream *>)));
	_service->openFileAsync(filePath);
}

void FCMainWidget::onFileOpened(QList<AVStream *> streams)
{
	ui.fiWidget->setService(_service);
	if (auto [err, des] = _service->lastError(); err < 0)
	{
		qDebug() << metaObject()->className() << " open file error " << des;
	}
}

void FCMainWidget::onStreamItemSelected(int streamIndex)
{
	_streamIndex = streamIndex;
	auto stream = _service->stream(streamIndex);
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		ui.widthEdit->setText(QString::number(stream->codecpar->width));
		ui.heightEdit->setText(QString::number(stream->codecpar->height));
		ui.fpsEdit->setText(QString::number(stream->avg_frame_rate.den / stream->avg_frame_rate.num));

		FCVideoTimelineWidget* widget = new FCVideoTimelineWidget(this);
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
		auto filePath = QFileDialog ::getSaveFileName(this, tr("±£´æÎÄ¼þ"), QString());
		if (!filePath.isEmpty())
		{
			int srcWidth = stream->codecpar->width;
			int srcHeight = stream->codecpar->height;
			int srcFps = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
			_muxEntry.filePath = filePath;
			_muxEntry.width = ui.widthEdit->text().toInt();
			if (_muxEntry.width <= 0)
			{
				_muxEntry.width = srcWidth;
			}
			_muxEntry.height = ui.heightEdit->text().toInt();
			if (_muxEntry.height <= 0)
			{
				_muxEntry.height = srcHeight;
			}
			_muxEntry.fps = ui.fpsEdit->text().toInt();
			if (_muxEntry.fps <= 0)
			{
				_muxEntry.fps = srcFps;
			}
			_muxEntry.startPts = ui.startPtsEdit->text().toDouble();
			_muxEntry.duration = ui.durationEdit->text().toDouble();
			_muxEntry.durationUnit = (FCDurationUnit)ui.durationUnitComboBox->currentIndex();
			_muxEntry.vStreamIndex = _streamIndex;
			QString filters;
			if (_muxEntry.width != srcWidth || _muxEntry.height != srcHeight)
			{
				filters = QString("scale=width=%1:height=%2").arg(_muxEntry.width).arg(_muxEntry.height);
			}
			if (_muxEntry.fps != srcFps)
			{
				if (!filters.isEmpty())
				{
					filters.append(',');
				}
				filters.append(QString("fps=fps=%1").arg(_muxEntry.fps));
			}
			auto text = ui.textEdit->toPlainText();
			if (!text.isEmpty())
			{
				if (!filters.isEmpty())
				{
					filters.append(',');
				}
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
				filters.append(QString("drawtext=fontfile=%1:fontsize=%2:fontcolor=%3:text=\'%4\':x=%5:y=%6")
					.arg(fontFile).arg(fontSize).arg(fontColor.name()).arg(text).arg(x).arg(y));
			}
			_muxEntry.filterString = filters;
			_service->saveAsync(_muxEntry);
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
		ui.startPtsEdit->setText(QString::number(widget->selectedPts()));
	}
}