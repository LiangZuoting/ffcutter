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
			_muxEntry.filePath = filePath;
			_muxEntry.width = ui.widthEdit->text().toInt();
			if (_muxEntry.width <= 0)
			{
				_muxEntry.width = stream->codecpar->width;
			}
			_muxEntry.height = ui.heightEdit->text().toInt();
			if (_muxEntry.height <= 0)
			{
				_muxEntry.height = stream->codecpar->height;
			}
			_muxEntry.startPts = ui.startPtsEdit->text().toDouble();
			_muxEntry.duration = ui.durationEdit->text().toDouble();
			_muxEntry.durationUnit = (FCDurationUnit)ui.durationUnitComboBox->currentIndex();
			_muxEntry.vStreamIndex = _streamIndex;
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