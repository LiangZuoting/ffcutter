#include "fcplaydialog.h"

FCPlayDialog::FCPlayDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(&_timer, &QTimer::timeout, this, &FCPlayDialog::onTimeout);
}

FCPlayDialog::~FCPlayDialog()
{}

void FCPlayDialog::play(const QVector<QPixmap>& frames, int fps)
{
	_frames = frames;
	_current = 0;
	ui.player->setPixmap(frames[_current]);
	_timer.start(1000 / fps);
	exec();
}

void FCPlayDialog::onTimeout()
{
	++_current;
	if (_current >= _frames.size())
	{
		_current = 0;
	}
	ui.player->setPixmap(_frames[_current]);
}

