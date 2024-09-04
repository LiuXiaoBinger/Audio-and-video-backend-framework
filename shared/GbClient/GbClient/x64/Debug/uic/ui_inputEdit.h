/********************************************************************************
** Form generated from reading UI file 'inputEdit.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INPUTEDIT_H
#define UI_INPUTEDIT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QLabel *label;
    QLineEdit *IpEdit;
    QLabel *label_2;
    QLineEdit *portEdit;
    QPushButton *pushButton;
    QPushButton *pushButton_2;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(692, 414);
        label = new QLabel(Form);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(83, 181, 71, 21));
        IpEdit = new QLineEdit(Form);
        IpEdit->setObjectName(QString::fromUtf8("IpEdit"));
        IpEdit->setGeometry(QRect(160, 180, 113, 20));
        label_2 = new QLabel(Form);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(340, 182, 41, 20));
        portEdit = new QLineEdit(Form);
        portEdit->setObjectName(QString::fromUtf8("portEdit"));
        portEdit->setGeometry(QRect(390, 180, 113, 20));
        pushButton = new QPushButton(Form);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(170, 270, 75, 23));
        pushButton_2 = new QPushButton(Form);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(380, 270, 75, 23));

        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
        label->setText(QCoreApplication::translate("Form", "\346\234\215\345\212\241\345\231\250IP:", nullptr));
        IpEdit->setText(QCoreApplication::translate("Form", "192.168.67.130", nullptr));
        label_2->setText(QCoreApplication::translate("Form", "\347\253\257\345\217\243:", nullptr));
        portEdit->setText(QCoreApplication::translate("Form", "11300", nullptr));
        pushButton->setText(QCoreApplication::translate("Form", "\347\241\256\345\256\232", nullptr));
        pushButton_2->setText(QCoreApplication::translate("Form", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INPUTEDIT_H
