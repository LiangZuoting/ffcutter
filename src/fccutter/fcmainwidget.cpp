#include "fcmainwidget.h"
#include <QTextCodec>
#include <QDebug>
#include <QFileDialog>
#include <libavutil/error.h>
#include "fcvideotimelinewidget.h"

FCMainWidget::FCMainWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.fiWidget, SIGNAL(streamItemSelected(int)), this, SLOT(onStreamItemSelected(int)));
	connect(ui.fastSeekBtn, SIGNAL(clicked()), this, SLOT(onFastSeekClicked()));
	connect(ui.saveBtn, SIGNAL(clicked()), this, SLOT(onSaveClicked()));

	ui.durationUnitComboBox->addItems({ u8"√Î", u8"÷°" });
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
		auto filePath = QFileDialog ::getSaveFileName(this, tr("±£¥ÊŒƒº˛"), QString());
		if (!filePath.isEmpty())
		{
			int srcWidth = stream->codecpar->width;
			int srcHeight = stream->codecpar->height;
			int srcFps = stream->avg_frame_rate.den / stream->avg_frame_rate.num;
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
				QString fontFile;
				int fontSize = ui.fontSizeComboBox->currentText().toInt();
				QColor fontColor(Qt::white);
				int x = 0;
				int y = 0;
				filters.append(QString("drawtext=fontfile=%1:fontsize=%2:fontcolor=%3:text=%4:x=%5:y=%6")
					.arg(fontFile).arg(fontSize).arg(fontColor.name()).arg(text).arg(x).arg(y));
			}
			_muxEntry.filterString = filters;
			_service->saveAsync(_muxEntry);
		}
	}
}

void FCMainWidget::onVideoFrameSelectionChanged()
{
	auto widget = qobject_cast<FCVideoTimelineWidget*>(sender());
	if (widget)
	{
		ui.startPtsEdit->setText(QString::number(widget->selectedPts()));
	}
}