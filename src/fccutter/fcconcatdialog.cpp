#include "fcconcatdialog.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QProcess>

FCConcatDialog::FCConcatDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);
	connect(ui.addBtn, SIGNAL(clicked()), this, SLOT(onAddClicked()));
	connect(ui.delBtn, SIGNAL(clicked()), this, SLOT(onDelClicked()));
	connect(ui.browseBtn, SIGNAL(clicked()), this, SLOT(onBrowseClicked()));
	connect(ui.execBtn, SIGNAL(clicked()), this, SLOT(onExecClicked()));
}

FCConcatDialog::~FCConcatDialog()
{
}

void FCConcatDialog::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
	}
	QWidget::dragEnterEvent(event);
}

void FCConcatDialog::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		auto urls = event->mimeData()->urls();
		int current = ui.inListWidget->currentRow();
		if (current < 0)
		{
			current = ui.inListWidget->count();
		}
		for (auto &url : urls)
		{
			ui.inListWidget->insertItem(current++, url.toLocalFile());
		}
		event->acceptProposedAction();
	}
	QWidget::dropEvent(event);
}

void FCConcatDialog::onAddClicked()
{
	auto filePaths = QFileDialog::getOpenFileNames(this, tr("select a file"));
	if (!filePaths.isEmpty())
	{
		int current = ui.inListWidget->currentRow();
		if (current < 0)
		{
			current = ui.inListWidget->count();
		}
		for (const auto &filePath : filePaths)
		{
			ui.inListWidget->insertItem(current++, filePath);
		}
	}
}

void FCConcatDialog::onDelClicked()
{
	auto item = ui.inListWidget->takeItem(ui.inListWidget->currentRow());
	if (item)
	{
		delete item;
	}
}

void FCConcatDialog::onBrowseClicked()
{
	auto filePath = QFileDialog::getSaveFileName();
	if (!filePath.isEmpty())
	{
		ui.lineEdit->setText(filePath);
	}
}

void FCConcatDialog::onExecClicked()
{
	QString dstFilePath = ui.lineEdit->text();
	if (dstFilePath.isEmpty())
	{
		return;
	}
	QStringList in;
	for (int i = 0; i < ui.inListWidget->count(); ++i)
	{
		in << ui.inListWidget->item(i)->text();
	}
	if (in.isEmpty())
	{
		return;
	}
	concat(in, dstFilePath);
}

void FCConcatDialog::concat(const QStringList &srcFilePaths, const QString &dstFilePath)
{
	QStringList args{ "-y" };
	QString complexFilter = "";
	for (int i = 0; i < srcFilePaths.size(); ++i)
	{
		args << "-i" << srcFilePaths[i];
		complexFilter.append(QString("[%1:v][%1:a]").arg(i));
	}
	complexFilter.append(QString("concat=n=%1:a=1:v=1[a][v]").arg(srcFilePaths.size()));
	args << "-filter_complex" << complexFilter << "-map" << "[a]" << "-map" << "[v]" << dstFilePath;

	int ret = QProcess::execute("../src/thirdparty/ffmpeg/bin/ffmpeg.exe", args);

}