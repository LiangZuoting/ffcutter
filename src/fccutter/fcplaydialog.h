#pragma once

#include <QDialog>
#include "ui_fcplaydialog.h"
#include "fcplayer.h"

extern "C"
{
#include <libavutil/frame.h>
}

class FCPlayDialog : public QDialog
{
	Q_OBJECT

public:
	FCPlayDialog(QWidget *parent = nullptr);
	~FCPlayDialog();

	void play(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames);

private:
	Ui::FCPlayDialogClass ui;
	FCPlayer* _player{};
};

