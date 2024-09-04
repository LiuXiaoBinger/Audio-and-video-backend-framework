/********************************************************************************
** Form generated from reading UI file 'GbClient.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GBCLIENT_H
#define UI_GBCLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GbClientClass
{
public:
    QTreeWidget *treeWidget;
    QLabel *label;

    void setupUi(QWidget *GbClientClass)
    {
        if (GbClientClass->objectName().isEmpty())
            GbClientClass->setObjectName(QString::fromUtf8("GbClientClass"));
        GbClientClass->resize(787, 698);
        treeWidget = new QTreeWidget(GbClientClass);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        treeWidget->setHeaderItem(__qtreewidgetitem);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(__qtreewidgetitem1);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setGeometry(QRect(0, 0, 300, 700));
        label = new QLabel(GbClientClass);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(300, 0, 700, 700));

        retranslateUi(GbClientClass);

        QMetaObject::connectSlotsByName(GbClientClass);
    } // setupUi

    void retranslateUi(QWidget *GbClientClass)
    {
        GbClientClass->setWindowTitle(QCoreApplication::translate("GbClientClass", "GbClient", nullptr));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->topLevelItem(0);
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("GbClientClass", "\346\226\260\345\273\272\351\241\271\347\233\256", nullptr));
        QTreeWidgetItem *___qtreewidgetitem1 = ___qtreewidgetitem->child(0);
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("GbClientClass", "\346\226\260\345\273\272\345\255\220\351\241\271\347\233\256", nullptr));
        treeWidget->setSortingEnabled(__sortingEnabled);

        label->setText(QCoreApplication::translate("GbClientClass", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GbClientClass: public Ui_GbClientClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GBCLIENT_H
