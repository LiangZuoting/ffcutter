#include "fcmainwindow.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "fcconcatdialog.h"

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
	FCConcatDialog dlg(this);
	dlg.exec();
}