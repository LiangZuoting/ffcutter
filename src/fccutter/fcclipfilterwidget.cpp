#include "fcclipfilterwidget.h"
#include <QFileDialog>
#include <QColorDialog>
#include <QRawFont>

FCClipFilterWidget::FCClipFilterWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.textColorBtn, SIGNAL(clicked()), this, SLOT(onTextColorClicked()));
	connect(ui.subtitleBtn, SIGNAL(clicked()), this, SLOT(onSubtitleBtnClicked()));
	loadFontSize();
	loadFonts();
}

FCClipFilterWidget::~FCClipFilterWidget()
{
}

void FCClipFilterWidget::setStartSec(double startSec)
{
	QTime t = QTime(0, 0).addMSecs(startSec * 1000);
	ui.startSecEdit->setTime(t);
}

void FCClipFilterWidget::setEndSec(double endInSec)
{
	QTime t = QTime(0, 0).addMSecs(endInSec * 1000);
	ui.endSecEdit->setTime(t);
}

void FCClipFilterWidget::onTextColorClicked()
{
	auto color = QColorDialog::getColor(QColor(ui.textColorBtn->text()));
	ui.textColorBtn->setStyleSheet("background: " + color.name());
	ui.textColorBtn->setText(color.name());
}

void FCClipFilterWidget::onSubtitleBtnClicked()
{
	QString srtFile = QFileDialog::getOpenFileName(this, tr("choose srt file"), QString(), "×ÖÄ»ÎÄ¼þ (*.srt)");
	if (!srtFile.isEmpty())
	{
		ui.subtitleEdit->setText(srtFile);
	}
}

void FCClipFilterWidget::loadFontSize()
{
	QFile f("fontsizes.txt");
	f.open(QFile::ReadOnly);
	QString text = f.readAll();
	auto ls = text.split(',');
	ui.fontSizeComboBox->addItems(ls);
	ui.stFontSizeComboBox->addItems(ls);
}

void FCClipFilterWidget::loadFonts()
{
	QDir fontsDir("fonts");
	auto ls = fontsDir.entryInfoList(QDir::Files);
	for (int i = 0; i < ls.size(); ++i)
	{
		auto filePath = ls[i].absoluteFilePath();
		QRawFont rawFont(filePath, 10);
		ui.fontComboBox->addItem(rawFont.familyName(), filePath);
	}
}