#pragma once

#include <QDialog>
#include "ui_fcloadingdialog.h"

class FCLoadingDialog : public QDialog
{
	Q_OBJECT

public:
	FCLoadingDialog(QWidget *parent = Q_NULLPTR);
	~FCLoadingDialog();

	void setLabelText(const QString &text);
	int exec2(const QString &text);

private:
	Ui::FCLoadingDialog ui;
};
