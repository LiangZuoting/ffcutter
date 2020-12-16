#include "fcfileinfowidget.h"
#include <QTime>

FCFileInfoWidget::FCFileInfoWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.infoTree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
}

FCFileInfoWidget::~FCFileInfoWidget()
{
}

void FCFileInfoWidget::setService(const QSharedPointer<FCService>& service)
{
	_service = service;
	auto formatContext = _service->formatContext();

	_rootItem = new QTreeWidgetItem(ui.infoTree);
	_rootItem->setText(0, formatContext->url);

	auto streams = _service->streams();
	for (auto stream : streams)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(_rootItem);
		auto typeStr = QString("%1(%2)").arg(av_get_media_type_string(stream->codecpar->codec_type)).arg(stream->index);
		item->setText(0, typeStr);
		item->setData(0, FFDataRole, stream->index);
	}

	ui.infoTree->expandAll();
}

void FCFileInfoWidget::onItemSelectionChanged()
{
	auto item = ui.infoTree->currentItem();
	if (item == _rootItem)
	{
		auto formatContext = _service->formatContext();
		char* buff = nullptr;
		av_dict_get_string(formatContext->metadata, &buff, ':', '\n');
		QTime time(0, 0);
		time = time.addSecs(formatContext->duration / AV_TIME_BASE);
		auto text = QString("%1\nduration:%2").arg(buff).arg(time.toString("hh:mm:ss"));
		ui.detailLabel->setText(text);
		av_free(buff);
	}
	else
	{
		auto streamIndex = item->data(0, FFDataRole).toInt();
		emit streamItemSelected(streamIndex);

		auto stream = _service->stream(streamIndex);
		char* buff = nullptr;
		av_dict_get_string(stream->metadata, &buff, ':', '\n');
		ui.detailLabel->setText(buff);
		av_free(buff);
	}
}