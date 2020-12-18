#include "fcloadingdialog.h"

FCLoadingDialog::FCLoadingDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowFlags(Qt::Dialog | Qt::WindowTitleHint))
{
	ui.setupUi(this);
}

FCLoadingDialog::~FCLoadingDialog()
{
}

void FCLoadingDialog::setLabelText(const QString &text)
{
	ui.label->setText(text);
}

int FCLoadingDialog::exec2(const QString &text)
{
	ui.label->setText(text);
	return exec();
}