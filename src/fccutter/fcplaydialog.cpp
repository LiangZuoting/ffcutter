#include "fcplaydialog.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <qdatetime.h>
#include <QDebug>
#include <qelapsedtimer.h>

FCPlayDialog::FCPlayDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	_player = new FCPlayer(this);
	ui.verticalLayout->addWidget(_player);
}

FCPlayDialog::~FCPlayDialog()
{}

void FCPlayDialog::play(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames)
{
	_player->setup(audioFrames, videoFrames);

	exec();
}
