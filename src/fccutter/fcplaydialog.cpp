#include "fcplaydialog.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <qdatetime.h>
#include <QDebug>
#include <qelapsedtimer.h>

FCPlayDialog::FCPlayDialog(QWidget *parent)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);

	_player = new FCPlayer(this);
	ui.verticalLayout->insertWidget(0, _player);

	connect(_player, &FCPlayer::finished, this, [this]
	{
			ui.playBtn->setEnabled(true);
	});

	connect(ui.playBtn, &QPushButton::clicked, this, [this]
	{
			ui.playBtn->setEnabled(false);
			_player->start();
	});
	connect(ui.volumeSlider, &QSlider::valueChanged, this, [this](int value)
		{
			_player->setVolume(value);
            ui.volumeLabel->setText(QString::number(value));
		});
}

FCPlayDialog::~FCPlayDialog()
{}

void FCPlayDialog::play(const QVector<AVFrame*>& audioFrames, const QVector<QPair<QPixmap, double>>& videoFrames)
{
	ui.playBtn->setEnabled(false);
	_player->setup(audioFrames, videoFrames);
	_player->setVolume(ui.volumeSlider->value());
	_player->start();
	exec();
}
