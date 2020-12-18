#include "fcmainwidget.h"
#include <QTextCodec>
#include <QDebug>
#include <libavutil/error.h>
#include "fcvideotimelinewidget.h"


FCMainWidget::FCMainWidget(QWidget *parent)
	: QWidget(parent)
	, _loadingDialog(this)
{
	ui.setupUi(this);
}

FCMainWidget::~FCMainWidget()
{
}

void FCMainWidget::openFile(const QString& filePath)
{
	closeFile();

	_service.reset(new FCService());
	connect(_service.data(), SIGNAL(fileOpened(QList<AVStream *>)), this, SLOT(onFileOpened(QList<AVStream *>)));
	connect(_service.data(), SIGNAL(errorOcurred()), this, SLOT(onErrorOcurred()));
	_service->openFileAsync(filePath);
	_loadingDialog.exec2(tr(u8"打开文件..."));
}

void FCMainWidget::closeFile()
{
	if (_fiWidget)
	{
		delete _fiWidget;
		_fiWidget = nullptr;
	}
	if (_opWidget)
	{
		delete _opWidget;
		_opWidget = nullptr;
	}
	if (_vTimelineWidget)
	{
		delete _vTimelineWidget;
		_vTimelineWidget = nullptr;
	}
	_streamIndex = -1;
	_service.reset();
}

void FCMainWidget::onFileOpened(QList<AVStream *> streams)
{
	_opWidget = new FCEditWidget(_service, this);
	ui.layout->addWidget(_opWidget);
	connect(_opWidget, SIGNAL(seekFinished(int)), this, SLOT(onSeekFinished(int)));

	_fiWidget = new FCFileInfoWidget(this);
	ui.layout->addWidget(_fiWidget);
	connect(_fiWidget, SIGNAL(streamItemSelected(int)), this, SLOT(selectStreamItem(int)));
	_fiWidget->setService(_service);

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
		if (_vTimelineWidget && _vTimelineWidget->streamIndex() == _streamIndex)
		{
			return;
		}
		if (_vTimelineWidget && _vTimelineWidget->streamIndex() != _streamIndex)
		{
			delete _vTimelineWidget;
			_vTimelineWidget = nullptr;
		}
		_opWidget->setCurrentStream(_streamIndex);

		_vTimelineWidget = new FCVideoTimelineWidget(this);
		connect(_vTimelineWidget, SIGNAL(selectionChanged()), this, SLOT(onVideoFrameSelectionChanged()));
		ui.layout->addWidget(_vTimelineWidget);
		_vTimelineWidget->setStreamIndex(streamIndex);
		_vTimelineWidget->setService(_service);
		_vTimelineWidget->decodeOnce();
	}
}

void FCMainWidget::onVideoFrameSelectionChanged()
{
	if (_vTimelineWidget)
	{
		_opWidget->setStartSec(_vTimelineWidget->selectedSec());
	}
}

void FCMainWidget::onErrorOcurred()
{
	_loadingDialog.close();
}

void FCMainWidget::onSeekFinished(int streamIndex)
{
	if (_vTimelineWidget->streamIndex() == streamIndex)
	{
		_vTimelineWidget->decodeOnce();
	}
}