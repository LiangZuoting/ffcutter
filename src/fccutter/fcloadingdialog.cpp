#include "fcloadingdialog.h"

FCLoadingDialog::FCLoadingDialog(QWidget *parent)
	: QDialog(parent)
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