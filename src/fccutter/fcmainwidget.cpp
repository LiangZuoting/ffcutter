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

	_filePath = filePath;
	_service.reset(new FCService());
	connect(_service.data(), SIGNAL(fileOpened(QList<AVStream *>, void *)), this, SLOT(onFileOpened(QList<AVStream *>, void *)));
	connect(_service.data(), SIGNAL(errorOcurred(void *)), this, SLOT(onErrorOcurred(void *)));
	_service->openFileAsync(filePath, this);

	_loadingDialog.exec2(tr(u8"打开文件..."));
}

void FCMainWidget::closeFile()
{
	_filePath.clear();
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
	if (_simpleTimelineWidget)
	{
		delete _simpleTimelineWidget;
		_simpleTimelineWidget = nullptr;
	}
	_streamIndex = -1;
	_service.reset();
}

void FCMainWidget::onFileOpened(QList<AVStream *> streams, void *userData)
{
	_opWidget = new FCEditWidget(_service, this);
	ui.layout->addWidget(_opWidget);
	connect(_opWidget, SIGNAL(seekFinished(int, QList<FCFrame>, void *)), this, SLOT(onSeekFinished(int, QList<FCFrame>, void *)));
	connect(_opWidget, &FCEditWidget::delogoClicked, this, [=](int state) 
		{
			if (state == Qt::Checked)
			{
				_selectType = DelogoType;
				_vTimelineWidget->beginSelect();
			}
			else
			{
				_selectType = NoneType;
				_vTimelineWidget->endSelect();
			}
		});
	connect(_opWidget, &FCEditWidget::masaicClicked, this, [=](int state)
		{
			if (state == Qt::Checked)
			{
				_selectType = MasaicType;
				_vTimelineWidget->beginSelect();
			}
			else
			{
				_selectType = NoneType;
				_vTimelineWidget->endSelect();
			}
		});

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
		connect(_vTimelineWidget, SIGNAL(startSelected()), this, SLOT(onStartFrameSelected()));
		connect(_vTimelineWidget, SIGNAL(endSelected()), this, SLOT(onEndFrameSelected()));
		connect(_vTimelineWidget, SIGNAL(startSelect(const QPoint &)), this, SLOT(onStartSelect(const QPoint &)));
		connect(_vTimelineWidget, SIGNAL(stopSelect(const QPoint &)), this, SLOT(onStopSelect(const QPoint &)));
		ui.layout->addWidget(_vTimelineWidget);
		_vTimelineWidget->setStreamIndex(streamIndex);
		_vTimelineWidget->setService(_service);
		_vTimelineWidget->clear();
		_vTimelineWidget->decodeOnce();

		_simpleTimelineWidget = new FCSimpleTimelineWidget(this);
		connect(_simpleTimelineWidget, SIGNAL(seekRequest(double)), _opWidget, SLOT(fastSeek(double)));
		ui.layout->addWidget(_simpleTimelineWidget);
		_simpleTimelineWidget->loadFile(_filePath);
		_simpleTimelineWidget->setCurrentStream(streamIndex);
	}
}

void FCMainWidget::onStartFrameSelected()
{
	if (_vTimelineWidget)
	{
		_opWidget->setStartSec(_vTimelineWidget->startSec());
	}
}

void FCMainWidget::onEndFrameSelected()
{
	if (_vTimelineWidget)
	{
		_opWidget->setEndSec(_vTimelineWidget->endSec());
	}
}

void FCMainWidget::onErrorOcurred(void *userData)
{
	_loadingDialog.close();
}

void FCMainWidget::onSeekFinished(int streamIndex, QList<FCFrame> frames, void *userData)
{
	if (userData == _opWidget && _vTimelineWidget->streamIndex() == streamIndex)
	{
		_vTimelineWidget->clear();
		_vTimelineWidget->appendFrames(frames);
		_vTimelineWidget->decodeOnce();
	}
}

void  FCMainWidget::onStartSelect(const QPoint &pos)
{
	if (_selectType == DelogoType)
	{
		_opWidget->setDelogoStart(pos);
	}
	else if (_selectType == MasaicType)
	{
		_opWidget->setMasaicStart(pos);
	}
}

void FCMainWidget::onStopSelect(const QPoint &pos)
{
	if (_selectType == DelogoType)
	{
		_opWidget->setDelogoStop(pos);
	}
	else if (_selectType == MasaicType)
	{
		_opWidget->setMasaicStop(pos);
	}
	_vTimelineWidget->endSelect();
}