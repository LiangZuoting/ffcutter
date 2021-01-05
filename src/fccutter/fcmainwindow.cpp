#include "fcmainwindow.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QProcess>

FCMainWindow::FCMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    connect(ui.actionConcat, SIGNAL(triggered()), this, SLOT(onConcatClicked()));
}

void FCMainWindow::openFile(const QString& filePath)
{
    ui.widget->openFile(filePath);
}

void FCMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    auto mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        event->acceptProposedAction();
    }
    QMainWindow::dragEnterEvent(event);
}

void FCMainWindow::dropEvent(QDropEvent* event)
{
	auto mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		auto urls = mimeData->urls();
        openFile(urls[0].toLocalFile());
        event->acceptProposedAction();
	}
    QMainWindow::dropEvent(event);
}

void FCMainWindow::onConcatClicked()
{
	concatAsync({"G:\\friends_clips\\phoebe_s1-3.mp4", "G:\\friends_clips\\phoebe_s1-3_3.mp4", "G:\\friends_clips\\phoebe_s1-8.mp4" }, "G:\\phoebe.mp4");
}

void FCMainWindow::concatAsync(const QStringList &srcFilePaths, const QString &dstFilePath)
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