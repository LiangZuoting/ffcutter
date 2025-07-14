#pragma once

#include <QDialog>
#include "ui_fcplaydialog.h"
#include <QTimer>

class FCPlayDialog : public QDialog
{
	Q_OBJECT

public:
	FCPlayDialog(QWidget *parent = nullptr);
	~FCPlayDialog();

	void play(const QVector<QPixmap>& frames, int fps);

private Q_SLOTS:
	void onTimeout();

private:
	Ui::FCPlayDialogClass ui;
	QVector<QPixmap> _frames;
	int _current{ 0 };
	QTimer _timer;
};

