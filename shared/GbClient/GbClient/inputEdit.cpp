#include "inputEdit.h"
#include "common.h"

std::string servIp;
int port;

inputEdit::inputEdit(QWidget* parent)
	: QWidget(parent)
{
	w = NULL;
	ui.setupUi(this);
}

inputEdit::~inputEdit()
{

}

void inputEdit::on_pushButton_clicked()
{
	servIp = ui.IpEdit->text().toUtf8().toStdString();
	port = ui.portEdit->text().toInt();
	w = new GbClient();
	w->show();
	w->init();
	this->close();
}
void inputEdit::on_pushButton_2_clicked()
{
	this->close();
}
