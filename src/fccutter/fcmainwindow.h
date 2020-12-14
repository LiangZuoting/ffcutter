#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_fcmainwindow.h"

class FCMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    FCMainWindow(QWidget *parent = Q_NULLPTR);

    void openFile(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    Ui::FCMainWindowClass ui;
};
