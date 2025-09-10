/*
 * Copyright (C) 2024 Mikhail Medvedev <e-ink-reader@yandex.ru>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "dialogpass.h"
#include "ui_dialogpass.h"
#include "mainwindow.h"
#include <QValidator>
//#include <QRegExp>
#include <QRegularExpression>

DialogPass::DialogPass(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPass)
{
    ui->setupUi(this);
    QRegularExpression reHex3( "[0-2][A-Fa-f0-9]{2,2}" );
    QRegularExpression reHex2( "[A-Fa-f0-9]{1,2}" );
    QRegularExpressionValidator *validatorA = new QRegularExpressionValidator(reHex3, this);
    QRegularExpressionValidator *validatorL = new QRegularExpressionValidator(reHex2, this);
    ui->lineEdit->setValidator(validatorA);
    ui->lineEdit_2->setValidator(validatorL);
    ui->lineEdit_3->setValidator(validatorL);
    ui->lineEdit_4->setValidator(validatorL);
    ui->lineEdit_5->setValidator(validatorL);
}

void DialogPass::setID(const uint id, uint32_t userAddr, uint32_t userPass)
{
   uint8_t firstDigit = 0;
   switch (id)
   {
     case 0:
       ui->radioButton_0->setChecked(true);
     break;
     case 1:
       ui->radioButton_1->setChecked(true);
     break;
     case 2:
       ui->radioButton_2->setChecked(true);
     break;
     case 3:
       ui->radioButton_3->setChecked(true);
     break;
     case 4:
       ui->radioButton_4->setChecked(true);
     break;
     default:
       ui->radioButton_0->setChecked(true);
     break;

   }
   firstDigit = static_cast<unsigned char>(userAddr >> 8) + 0x30;
   if (firstDigit > 0x32) firstDigit = 0x30;
   ui->lineEdit->setText(QString(static_cast<char>(firstDigit)) + bytePrt(static_cast<unsigned char>(userAddr & 0xff)));
   ui->lineEdit_2->setText(bytePrt(static_cast<unsigned char>((userPass >> 24) & 0xff)));
   ui->lineEdit_3->setText(bytePrt(static_cast<unsigned char>((userPass >> 16) & 0xff)));
   ui->lineEdit_4->setText(bytePrt(static_cast<unsigned char>((userPass >>  8) & 0xff)));
   ui->lineEdit_5->setText(bytePrt(static_cast<unsigned char>(userPass & 0xff)));
}

DialogPass::~DialogPass()
{
    delete ui;
}

void DialogPass::on_pushButton_clicked()
{
    uint8_t id = 0;
    if (ui->radioButton_0->isChecked()) id = 0;
    if (ui->radioButton_1->isChecked()) id = 1;
    if (ui->radioButton_2->isChecked()) id = 2;
    if (ui->radioButton_3->isChecked()) id = 3;
    if (ui->radioButton_4->isChecked())
    {
        id = 4;
        setUserPassword();

    }
    //return id
    emit sendID(id);
    DialogPass::close();
}

void DialogPass::setUserPassword()
{
    uint32_t userAddr = 0, userPass = 0;
    QString buf = "";
    if ((ui->lineEdit->text().isEmpty()) || (ui->lineEdit_2->text().isEmpty()) || (ui->lineEdit_3->text().isEmpty()) || (ui->lineEdit_4->text().isEmpty()) || (ui->lineEdit_5->text().isEmpty()))
    {
        QMessageBox::about(this, tr("Error"), tr("Invalid field value."));
        return;
    }
   if (ui->lineEdit_2->text().length() == 1) ui->lineEdit_2->setText("0" + ui->lineEdit_2->text());
   if (ui->lineEdit_3->text().length() == 1) ui->lineEdit_3->setText("0" + ui->lineEdit_3->text());
   if (ui->lineEdit_4->text().length() == 1) ui->lineEdit_4->setText("0" + ui->lineEdit_4->text());
   if (ui->lineEdit_5->text().length() == 1) ui->lineEdit_5->setText("0" + ui->lineEdit_5->text());
   userAddr = hexToInt(ui->lineEdit->text());
   buf = ui->lineEdit_2->text() +  ui->lineEdit_3->text() +  ui->lineEdit_4->text() +  ui->lineEdit_5->text();
   userPass = hexToInt(buf);
   emit sendUserPass(userAddr, userPass);
}

uint32_t DialogPass::hexToInt(QString str)
{
    unsigned char c;
    uint32_t len = static_cast<uint32_t>(str.length());
    QByteArray bstr = str.toLocal8Bit();
    if ((len > 0) && (len < 9))
    {
        uint32_t i, j = 1;
        uint32_t  addr = 0;
        for (i = len; i >0; i--)
        {
           c = static_cast<unsigned char>(bstr[i-1]);
           if ((c >= 0x30) && (c <=0x39)) addr =  addr + (c - 0x30) * j;
           if ((c >= 0x41) && (c <= 0x46)) addr = addr + (c - 0x37) * j;
           if ((c >= 0x61) && (c <= 0x66)) addr = addr + (c - 0x57) * j;
        j = j * 16;
        }
        return addr;
    }
    else return 0;
}

QString DialogPass::bytePrt(unsigned char z)
{
    unsigned char s;
    s = z / 16;
    if (s > 0x9) s = s + 0x37;
    else s = s + 0x30;
    z = z % 16;
    if (z > 0x9) z = z + 0x37;
    else z = z + 0x30;
    return QString(static_cast<char>(s)) + QString(static_cast<char>(z));
}
