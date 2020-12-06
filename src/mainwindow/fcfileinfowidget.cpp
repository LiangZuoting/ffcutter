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

void FCFileInfoWidget::setFile(AVFormatContext* fmtCtx)
{
	_rootItem = new QTreeWidgetItem(ui.infoTree);
	_rootItem->setText(0, fmtCtx->url);
	_rootItem->setData(0, FFDataRole, QVariant::fromValue((void*)fmtCtx));

	ui.infoTree->expandAll();
}

void FCFileInfoWidget::addStream(AVStream* stream)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(_rootItem);
	auto typeStr = av_get_media_type_string(stream->codecpar->codec_type);
	item->setText(0, typeStr);
	item->setData(0, FFDataRole, QVariant::fromValue((void*)stream));
}

void FCFileInfoWidget::onItemSelectionChanged()
{
	auto item = ui.infoTree->currentItem();
	if (item == _rootItem)
	{
		auto fmtCtx = (AVFormatContext*)item->data(0, FFDataRole).value<void*>();
		char* buff = nullptr;
		av_dict_get_string(fmtCtx->metadata, &buff, ':', '\n');
		QTime time(0, 0);
		time = time.addSecs(fmtCtx->duration / AV_TIME_BASE);
		auto text = QString("%1\nduration:%2").arg(buff).arg(time.toString("hh:mm:ss"));
		ui.detailLabel->setText(text);
		av_free(buff);
	}
	else
	{
		auto stream = (AVStream*)item->data(0, FFDataRole).value<void*>();
		emit parseStream(stream);

		char* buff = nullptr;
		av_dict_get_string(stream->metadata, &buff, ':', '\n');
		ui.detailLabel->setText(buff);
		av_free(buff);
	}
}