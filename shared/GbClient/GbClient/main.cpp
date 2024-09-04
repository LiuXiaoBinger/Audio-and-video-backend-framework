#include "GbClient.h"
#include <QtWidgets/QApplication>
#include "inputEdit.h"



#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	inputEdit ww;
	ww.show();
    return a.exec();
}
