#pragma once
#include <QWidget>
#include "ui_inputEdit.h"
#include "GbClient.h"

class inputEdit : public QWidget
{
	Q_OBJECT;

public:
	inputEdit(QWidget* parent = nullptr);
	~inputEdit();

private slots:
	void on_pushButton_clicked();
	void on_pushButton_2_clicked();


private:
	Ui::Form ui;
	GbClient* w;
};
