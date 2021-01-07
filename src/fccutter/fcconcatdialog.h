#pragma once

#include <QDialog>
#include "ui_FCConcatDialog.h"

class FCConcatDialog : public QDialog
{
	Q_OBJECT

public:
	FCConcatDialog(QWidget *parent = Q_NULLPTR);
	~FCConcatDialog();

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private Q_SLOTS:
	void onAddClicked();
	void onDelClicked();
	void onBrowseClicked();
	void onExecClicked();

private:
	void concat(const QStringList &srcFilePaths, const QString &dstFilePath);

	Ui::FCConcatDialog ui;
};
