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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qhexedit.h"
#include <QLabel>
#include <QMessageBox>
#include <QErrorMessage>
#include <QDragEnterEvent>
#include <QtGui>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QValidator>
#include <QRegExp>
#include <QtDebug>
#include "dialogabout.h"
#include "dialogpass.h"
#include <stdlib.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    SFPData.reserve(0x200);
    SFPData.resize(0x200);
    SFPData.fill(char(0xff));
    QFont heFont;
    heFont = QFont("DejaVu Sans Mono", 10);
    hexEdit = new QHexEdit(ui->frame);
    hexEdit->setGeometry(0,0,ui->frame->width(),ui->frame->height());
    hexEdit->setData(SFPData);
    hexEdit->setHexCaps(true);
    defaultTextColor = ui->label->palette().color(QPalette::Text);
    hexEdit->setAsciiFontColor(defaultTextColor);
    hexEdit->setAddressFontColor(defaultTextColor);
    hexEdit->setHexFontColor(defaultTextColor);
    hexEdit->setFont(heFont);
    ui->statusBar->addPermanentWidget(ui->lStatus,0);
    ui->statusBar->addPermanentWidget(ui->eStatus,0);
    ui->statusBar->addPermanentWidget(ui->cLabel,0);
    ui->statusBar->addPermanentWidget(ui->crcEdit,0);
    ui->comboBox_wavelength->addItem(" ", 0);
    ui->comboBox_wavelength->addItem("1270 nm", 1270);
    ui->comboBox_wavelength->addItem("1290 nm", 1290);
    ui->comboBox_wavelength->addItem("1310 nm", 1310);
    ui->comboBox_wavelength->addItem("1330 nm", 1330);
    ui->comboBox_wavelength->addItem("1350 nm", 1350);
    ui->comboBox_wavelength->addItem("1370 nm", 1370);
    ui->comboBox_wavelength->addItem("1390 nm", 1390);
    ui->comboBox_wavelength->addItem("1410 nm", 1410);
    ui->comboBox_wavelength->addItem("1430 nm", 1430);
    ui->comboBox_wavelength->addItem("1450 nm", 1450);
    ui->comboBox_wavelength->addItem("1470 nm", 1470);
    ui->comboBox_wavelength->addItem("1490 nm", 1490);
    ui->comboBox_wavelength->addItem("1510 nm", 1510);
    ui->comboBox_wavelength->addItem("1530 nm", 1530);
    ui->comboBox_wavelength->addItem("1550 nm", 1550);
    ui->comboBox_wavelength->addItem("1570 nm", 1570);
    ui->comboBox_wavelength->addItem("1590 nm", 1590);
    ui->comboBox_wavelength->addItem("1610 nm", 1610);
    ui->comboBox_modtype->addItem("Unknown", 0);
    ui->comboBox_modtype->addItem("GBIC", 1);
    ui->comboBox_modtype->addItem("SFP",  3);
    ui->comboBox_connector->addItem("Unknown", 0);
    ui->comboBox_connector->addItem("SC",      1);
    ui->comboBox_connector->addItem("BNC/TNC", 4);
    ui->comboBox_connector->addItem("LC",      7);
    ui->comboBox_connector->addItem("MT-RJ",   8);
    ui->comboBox_connector->addItem("MU",      9);
    ui->comboBox_connector->addItem("SG",     0xA);
    ui->comboBox_connector->addItem("RJ-45", 0x22);
    QRegExp reHex( "[A-Fa-f0-9]{1,2}" );
    QRegExp reDigits( "[0-9]{1,5}" );
    QRegExpValidator *validator = new QRegExpValidator(reHex, this);
    QRegExpValidator *valDigit = new QRegExpValidator(reDigits, this);
    ui->lineEdit_vendorid->setValidator(validator);
    ui->lineEdit_manid0->setValidator(validator);
    ui->lineEdit_manid1->setValidator(validator);
    ui->lineEdit_manid2->setValidator(validator);
    ui->lineEdit_925->setValidator(valDigit);
    ui->lineEdit_50125->setValidator(valDigit);
    ui->lineEdit_62125->setValidator(valDigit);
    ui->lineEdit_ifspeed->setValidator(valDigit);
    lastDirectory = QDir::homePath(); //"/home/";
    cmdStarted = false;
    crc32show();
    ui->checkBox_1->setChecked(true);
    ui->checkBox_2->setChecked(true);
    ui->checkBox_3->setChecked(true);
    ui->checkBox_4->setChecked(true);
    devHandle = NULL;
    //        id,  name,    address, password
    pass[0] = {0, "None",    0x17b,      0    };
    pass[1] = {1, "Default", 0x17b, 0x00001011};
    pass[2] = {2, "SNR-1"  , 0x17b, 0x22445588};
    pass[3] = {3, "SNR-2"  , 0x17b, 0x44554455};
    pass[4] = {4, "User"   , 0x17b,      0    };
    currentPass  = pass[0];
    // connect and status check
    statusCh341a = ch341aConnect();
    ch341StatusFlashing();
    ch341aShutdown();
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));
    timer->start(2000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionLoad_triggered()
{
    int i;
    QByteArray buf;
    ui->statusBar->showMessage(tr("Opening file"));
    if (!cmdStarted)
    {
        fileName = QFileDialog::getOpenFileName(this,
                                    QString(tr("Open file")),
                                    lastDirectory,
                                    "Data Images (*.bin *.BIN);;All files (*.*)");
    }
   else fileName = lastDirectory;
   cmdStarted = false;

    QFileInfo info(fileName);
    ui->statusBar->showMessage(tr("Current file: ") + info.fileName());
    lastDirectory = info.filePath();

    QFile file(fileName);
    if (info.size() > 0x200)
    {
      QMessageBox::about(this, tr("Error"), tr("The file size is incorrect!"));
      return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {

        return;
    }
    buf.resize(static_cast<int>(info.size()));
    buf = file.readAll();
    SFPData.resize(0x200);
    SFPData.fill(static_cast<char>(0xff));
    for (i = 0; i < static_cast<int>(info.size()); i++)
    {
       SFPData[i] = buf[i];
    }

    hexEdit->setData(SFPData);

    file.close();
    crc32show();
    on_pushButton_parsing_clicked();
}

void MainWindow::on_pushButton_parsing_clicked()
{
    //Parsing module parameters
    SFPData = hexEdit->data();
    int i, index;
    QString tmpstr = "";
    index = ui->comboBox_modtype->findData(static_cast<int>(SFPData[0] &0xff));
    if ( index != -1 )
    { // -1 for not found
       ui->comboBox_modtype->setCurrentIndex(index);
    }

    index = ui->comboBox_connector->findData(static_cast<int>(SFPData[2] &0xff));
    if ( index != -1 )
    { // -1 for not found
       ui->comboBox_connector->setCurrentIndex(index);
    }

    ui->lineEdit_ifspeed->setText(QString::number(static_cast<int>(SFPData[12] &0xff) * 100));

    ui->lineEdit_925->setText(QString::number(static_cast<int>(SFPData[14] &0xff)));

    ui->lineEdit_50125->setText(QString::number(static_cast<int>(SFPData[16] &0xff) * 10));

    ui->lineEdit_62125->setText(QString::number(static_cast<int>(SFPData[17] &0xff) * 10));

    for (i = 20; i<=35; i++) tmpstr.append( static_cast<char>(SFPData[i]));
    ui->lineEdit_manuf->setText(tmpstr);

    ui->lineEdit_manid0->setText(bytePrint(static_cast<unsigned char>(SFPData[37])));
    ui->lineEdit_manid1->setText(bytePrint(static_cast<unsigned char>(SFPData[38])));
    ui->lineEdit_manid2->setText(bytePrint(static_cast<unsigned char>(SFPData[39])));

    tmpstr ="";
    for (i = 40; i<=55; i++) tmpstr.append( static_cast<char>(SFPData[i]));
    ui->lineEdit_model->setText(tmpstr);

    tmpstr ="";
    for (i = 56; i<=59; i++) tmpstr.append( static_cast<char>(SFPData[i]));
    ui->lineEdit_revision->setText(tmpstr);

    index = ui->comboBox_wavelength->findData(static_cast<int>(SFPData[60] &0xff) * 256 + static_cast<int>(SFPData[61] &0xff));
    if ( index != -1 )
    { // -1 for not found
       ui->comboBox_wavelength->setCurrentIndex(index);
    }

    tmpstr ="";
    for (i = 68; i<=83; i++) tmpstr.append( static_cast<char>(SFPData[i]));
    ui->lineEdit_sernum->setText(tmpstr);
    tmpstr ="";
    tmpstr.append( static_cast<char>(SFPData[88]));
    tmpstr.append( static_cast<char>(SFPData[89]));
    tmpstr.append("-");
    tmpstr.append( static_cast<char>(SFPData[86]));
    tmpstr.append( static_cast<char>(SFPData[87]));
    tmpstr.append("-20");
    tmpstr.append( static_cast<char>(SFPData[84]));
    tmpstr.append( static_cast<char>(SFPData[85]));
    ui->lineEdit_day->setText(tmpstr.mid(0,2));
    ui->lineEdit_mon->setText(tmpstr.mid(3,2));
    ui->lineEdit_year->setText(tmpstr.mid(6,4));

    if (static_cast<int>(SFPData[0x62] &0xff) < 64) ui->lineEdit_vendorid->setText(bytePrint(static_cast<unsigned char>(SFPData[0x62])));
    else ui->lineEdit_vendorid->setText("");

    ui->lineEdit_crclow->setText(checkSumLo());
    ui->lineEdit_crchigh->setText(checkSumHi());
    SFPData = hexEdit->data();
    crc32show();
}


void MainWindow::on_pushButton_checksum_clicked()
{
    checkSumsUpdate();
    on_pushButton_parsing_clicked();
}

void MainWindow::checkSumsUpdate()
{
    SFPData = hexEdit->data();
    SFPData[63] = static_cast<char>(hexToInt(checkSumLo()));
    SFPData[95] = static_cast<char>(hexToInt(checkSumHi()));
    hexEdit->setData(SFPData);
}

void MainWindow::on_pushButton_magic_clicked()
{
    const uint8_t v_keys[64][16] {
        {0xE4, 0x44, 0xD4, 0xB9, 0x20, 0x33, 0xE8, 0xE0, 0xE1, 0x9D, 0x08, 0x89, 0x86, 0x57, 0x36, 0x59},
        {0xC4, 0xDA, 0xE7, 0x3B, 0xA0, 0x81, 0xA4, 0x5D, 0x67, 0x3C, 0x59, 0x10, 0x66, 0x00, 0x09, 0x1C},
        {0x72, 0x25, 0x19, 0x5B, 0x91, 0x36, 0x21, 0x09, 0xEF, 0xF4, 0x0E, 0x7A, 0xFA, 0x63, 0xC2, 0x54},
        {0x76, 0xDC, 0xD2, 0x90, 0x5A, 0xD9, 0xC7, 0x6D, 0xE1, 0x3D, 0x7E, 0x3D, 0x9A, 0xE9, 0x08, 0x96},
        {0x29, 0x57, 0xBA, 0x73, 0x83, 0xE4, 0x2E, 0x10, 0xF6, 0x8E, 0x40, 0xC2, 0xEF, 0x2A, 0x22, 0x4C},
        {0x21, 0x10, 0x19, 0x7B, 0x95, 0xEC, 0xCE, 0x7B, 0x84, 0x5F, 0xFE, 0xC0, 0x6F, 0x8D, 0xCA, 0xFE},
        {0xE8, 0xAD, 0xA7, 0x01, 0x16, 0x4B, 0x0F, 0x26, 0x15, 0x49, 0xFF, 0x90, 0x83, 0x9A, 0x57, 0x34},
        {0xF4, 0x86, 0xBD, 0xCD, 0x7F, 0xA9, 0x8A, 0x88, 0x3F, 0xB4, 0xCB, 0xD9, 0xC3, 0xCA, 0x61, 0x55},
        {0xCF, 0x24, 0xE1, 0x16, 0x38, 0x6E, 0xC5, 0x1A, 0x5C, 0x37, 0xE9, 0xE4, 0xA8, 0xB5, 0x60, 0x09},
        {0xDF, 0x2E, 0xAD, 0x84, 0xDA, 0x10, 0x29, 0x73, 0xF2, 0x4B, 0xE3, 0x48, 0xB8, 0xC2, 0xDC, 0xB9},
        {0xAE, 0xEE, 0x77, 0x80, 0xEB, 0x2A, 0x4F, 0x0C, 0xAB, 0x67, 0x30, 0x9E, 0x5C, 0x8A, 0x4D, 0xDD},
        {0xC3, 0xF9, 0xE9, 0xB2, 0xD5, 0x32, 0x9E, 0x6C, 0xDE, 0x04, 0x59, 0x3E, 0x3D, 0x74, 0x2B, 0xFC},
        {0xA5, 0xD9, 0x59, 0x72, 0x3E, 0xA1, 0xAD, 0xFC, 0x03, 0xCA, 0xC4, 0xCF, 0xA2, 0x19, 0xFE, 0x9F},
        {0xCE, 0x06, 0x71, 0x66, 0x60, 0x1E, 0xE6, 0x44, 0xC2, 0x00, 0x1A, 0xAA, 0x42, 0xE0, 0x3E, 0x3D},
        {0xB5, 0x07, 0x98, 0xE9, 0x12, 0xE1, 0xD0, 0xCB, 0x83, 0x5E, 0xC3, 0x66, 0x87, 0x52, 0x73, 0x5F},
        {0xE2, 0x45, 0x56, 0x71, 0x9B, 0xA4, 0xF3, 0x1A, 0xCE, 0x2E, 0x38, 0x7C, 0xF8, 0xF7, 0x26, 0x6C},
        {0xED, 0x58, 0x22, 0xB7, 0x85, 0xCD, 0xD6, 0x98, 0x0B, 0x15, 0x0F, 0x73, 0x0D, 0x56, 0xDD, 0xFD},
        {0x1E, 0xB7, 0x96, 0x02, 0x57, 0xE4, 0xE3, 0xDE, 0xD2, 0x8E, 0xA1, 0xC4, 0x3E, 0xC8, 0xE2, 0x8A},
        {0x1E, 0xEA, 0x19, 0x0A, 0x89, 0x62, 0xB1, 0x53, 0xAB, 0x1E, 0xA7, 0xF6, 0x03, 0xF4, 0x0B, 0x9A},
        {0x53, 0x6B, 0x23, 0x19, 0x93, 0xDE, 0xA8, 0x90, 0x0E, 0x20, 0x68, 0x82, 0x04, 0x53, 0x82, 0xA6},
        {0x66, 0xAF, 0x4D, 0xB5, 0x1D, 0xB2, 0x51, 0x0D, 0x54, 0x4A, 0x9B, 0xF0, 0xA9, 0x5C, 0x0E, 0x25},
        {0xB0, 0x31, 0x0D, 0x86, 0x8F, 0x83, 0x42, 0x41, 0x43, 0xE4, 0x8A, 0xC7, 0x7A, 0x88, 0xF6, 0xA0},
        {0xC7, 0x96, 0xDC, 0xF5, 0x41, 0xBB, 0xD4, 0xB4, 0x24, 0xA7, 0xDC, 0x60, 0xDF, 0x6F, 0xD4, 0xAE},
        {0x25, 0x49, 0x32, 0x79, 0xFC, 0xD2, 0xAF, 0xEF, 0x90, 0xEB, 0xF9, 0x62, 0x60, 0x78, 0x2F, 0xA8},
        {0x40, 0xC0, 0xB8, 0x9B, 0x06, 0x70, 0x3C, 0x5A, 0x0E, 0x37, 0x69, 0x46, 0xA6, 0x2B, 0x7F, 0x26},
        {0xA2, 0x83, 0xC4, 0xD3, 0x08, 0xFB, 0xE1, 0x7C, 0xF5, 0x14, 0xB3, 0x84, 0xF7, 0x11, 0x4C, 0xAF},
        {0xC1, 0x2B, 0xF0, 0xB8, 0x5B, 0xDE, 0x58, 0xEE, 0xFF, 0x0A, 0x52, 0xA3, 0xFD, 0xA2, 0xFE, 0x9B},
        {0x37, 0x00, 0xA2, 0xA3, 0xA5, 0xBF, 0xF7, 0x08, 0x63, 0x7F, 0xCB, 0x1B, 0x1E, 0x65, 0x2D, 0x84},
        {0x6B, 0xB9, 0x74, 0x4B, 0x40, 0xF7, 0x58, 0x61, 0xF9, 0x0E, 0x97, 0x65, 0xE4, 0xC3, 0x41, 0xFF},
        {0xD5, 0xBF, 0xDD, 0xF9, 0xD3, 0x2D, 0xE2, 0x81, 0xF9, 0x1D, 0x2E, 0x19, 0xD5, 0x64, 0xE2, 0x67},
        {0x1D, 0x79, 0x54, 0x45, 0xC5, 0xC9, 0x2C, 0xE1, 0x0A, 0x35, 0x28, 0xAE, 0x6B, 0xBF, 0x79, 0x42},
        {0x9B, 0x90, 0x63, 0xA6, 0x90, 0x55, 0x90, 0xF9, 0x96, 0xED, 0xEE, 0x9D, 0x2D, 0x2C, 0x7C, 0x38},
        {0xE7, 0x7C, 0x81, 0xB4, 0xDB, 0x57, 0xC5, 0x40, 0x35, 0x9D, 0x06, 0x6D, 0x73, 0x44, 0x65, 0x92},
        {0x69, 0xA4, 0x36, 0xE9, 0xFE, 0x37, 0x23, 0x6F, 0x5D, 0xEF, 0xFA, 0x87, 0x05, 0x9F, 0xDA, 0xF8},
        {0xB9, 0xA0, 0x0A, 0xAA, 0x71, 0x9E, 0x32, 0xBD, 0x77, 0x39, 0x40, 0x83, 0x2B, 0x84, 0x35, 0xD1},
        {0x4F, 0xE9, 0x65, 0x82, 0xEC, 0xD3, 0x8A, 0xB3, 0x1B, 0x13, 0x62, 0xE8, 0x7D, 0xBC, 0x0C, 0xB5},
        {0xA4, 0x07, 0xEF, 0x06, 0xB7, 0x80, 0x83, 0x09, 0xC2, 0x06, 0xD6, 0x2E, 0x63, 0x8F, 0xD9, 0x0E},
        {0x4E, 0x61, 0xF0, 0x91, 0x6A, 0x1A, 0xB6, 0x06, 0xF2, 0x7A, 0x16, 0xBF, 0x75, 0x84, 0x13, 0x52},
        {0xA6, 0x90, 0x11, 0xD9, 0x7D, 0x2B, 0xA9, 0x42, 0x24, 0x06, 0xB9, 0x30, 0x3B, 0x33, 0x41, 0x29},
        {0x55, 0x1B, 0xC8, 0x26, 0x78, 0x1B, 0xC5, 0x46, 0xE1, 0x12, 0x27, 0x0C, 0x1E, 0x15, 0xFD, 0xFC},
        {0xC2, 0x5B, 0x9E, 0x11, 0xD4, 0x72, 0x93, 0x8A, 0xA0, 0x38, 0xE8, 0xB9, 0x94, 0x92, 0x9E, 0x42},
        {0x74, 0xF8, 0xFC, 0x22, 0x27, 0xC6, 0xA9, 0x85, 0xE8, 0xED, 0x84, 0xCF, 0x46, 0x51, 0xAC, 0x94},
        {0xE5, 0x59, 0x68, 0xD0, 0xDB, 0x82, 0x61, 0xB0, 0x33, 0xAC, 0x73, 0xA7, 0x9D, 0xBB, 0xBF, 0x5A},
        {0xAC, 0x07, 0x7C, 0x94, 0x66, 0x3C, 0x51, 0xA3, 0x08, 0xEB, 0x3D, 0xF8, 0x0F, 0x47, 0x2F, 0x1B},
        {0x20, 0x89, 0x9E, 0x05, 0x52, 0x4D, 0x03, 0xE5, 0xDF, 0x42, 0x5A, 0x1C, 0x26, 0x8E, 0xA5, 0x50},
        {0xEB, 0x58, 0x48, 0x8C, 0x35, 0x4C, 0xDE, 0xCE, 0x3F, 0x2A, 0x42, 0x98, 0x69, 0xE7, 0x87, 0x90},
        {0x74, 0xFB, 0x11, 0xA1, 0x79, 0xB1, 0x79, 0xF7, 0x92, 0x1B, 0x8D, 0xF7, 0x4F, 0x0B, 0x6E, 0x44},
        {0x43, 0xDB, 0x61, 0xEB, 0x95, 0x16, 0x3E, 0xE8, 0x7F, 0x9C, 0x94, 0xBE, 0x42, 0x42, 0xB2, 0x04},
        {0xE0, 0x8F, 0xE0, 0xB2, 0x21, 0xE1, 0xB4, 0x08, 0x5F, 0x16, 0x0D, 0x48, 0xF9, 0x33, 0xFC, 0x37},
        {0xB3, 0x80, 0xE5, 0xBF, 0x95, 0x9A, 0x63, 0xE0, 0xC8, 0x30, 0x42, 0x4B, 0xCC, 0x57, 0xB2, 0x65},
        {0x55, 0x56, 0x0A, 0x4A, 0x68, 0xBA, 0xC3, 0x07, 0x33, 0x53, 0xE9, 0x1F, 0x43, 0x25, 0x6E, 0x17},
        {0x3C, 0x78, 0xB7, 0xFA, 0x24, 0xD8, 0x6C, 0xE6, 0x38, 0x06, 0x5C, 0x4D, 0xE6, 0x16, 0x97, 0xB5},
        {0xE1, 0x5F, 0x72, 0x48, 0x41, 0x4E, 0xB6, 0xF5, 0x20, 0xC2, 0x11, 0x4D, 0x2D, 0xB1, 0xA4, 0xE6},
        {0xDC, 0x82, 0xD4, 0xAC, 0x45, 0xB1, 0x39, 0xDB, 0xB1, 0x0F, 0xA2, 0xB6, 0x80, 0x9F, 0x3F, 0x03},
        {0x86, 0x8A, 0x45, 0xBD, 0xB9, 0x9B, 0x7E, 0xE0, 0x34, 0x64, 0x96, 0xF1, 0x98, 0x08, 0xCF, 0xA4},
        {0x85, 0xDE, 0x5D, 0xD3, 0xF5, 0x54, 0xEB, 0xBE, 0x42, 0x4A, 0x55, 0xA5, 0xCB, 0xB3, 0xBC, 0x40},
        {0x43, 0xF7, 0x65, 0x97, 0xB1, 0x84, 0x09, 0xCA, 0x52, 0x38, 0x67, 0x2B, 0x92, 0x18, 0xAE, 0x6F},
        {0x47, 0x6D, 0x13, 0x71, 0x46, 0xA1, 0x61, 0x8F, 0xEB, 0xB7, 0x54, 0xFA, 0x96, 0x90, 0x0D, 0x7A},
        {0x08, 0x97, 0xE1, 0xF8, 0x4A, 0x36, 0x79, 0x92, 0x77, 0x4E, 0x94, 0xBB, 0x3D, 0xC3, 0x61, 0x19},
        {0x10, 0x1D, 0x35, 0x95, 0x27, 0xA9, 0xBB, 0x5E, 0x9D, 0x56, 0xAF, 0xC6, 0xF1, 0x28, 0x32, 0xA3},
        {0xE6, 0x69, 0xA9, 0xCF, 0x63, 0x93, 0xAE, 0x59, 0xC4, 0x86, 0x1D, 0xC2, 0x58, 0x38, 0xF8, 0xB1},
        {0xF2, 0x00, 0xB4, 0x1F, 0x98, 0x6B, 0xD9, 0x1B, 0x66, 0x37, 0x56, 0x18, 0xEC, 0x7B, 0x2B, 0xCA},
        {0xDC, 0x6D, 0xCD, 0x0D, 0x2D, 0xA9, 0xB6, 0x0E, 0x1A, 0xF1, 0xE2, 0x4F, 0x24, 0x58, 0x53, 0x47},
        {0xFC, 0x26, 0x7E, 0x10, 0xA9, 0xD7, 0xDC, 0xC7, 0x48, 0x4B, 0x4A, 0xC0, 0x77, 0x67, 0xF9, 0xDF}
    };

    QByteArray hash_input, hash_result;
    int i;
    uint32_t crc =0;

    if ((static_cast<int>(SFPData[0x62]) > 63) || (static_cast<int>(SFPData[0x62]) < 0))
    {
        QMessageBox::about(this, tr("Error"), tr("Vendor ID is incorrect!"));
        return;
    }
    hash_input.resize(49);
    hash_input.fill(static_cast<char>(0xff));
    hash_input[0] = static_cast<char>(SFPData[0x62]);
    for (i = 1; i<=16; i++)   hash_input[i]  = SFPData[i - 1 + 20];
    for (i = 17; i<=32; i++)  hash_input[i]  = SFPData[i - 17 + 68];
    for (i = 33; i<=48; i++)  hash_input[i]  = static_cast<char>(~v_keys[static_cast<int>(SFPData[0x62])][i - 33]);
    hash_result = QCryptographicHash::hash(hash_input, QCryptographicHash::Md5);
    for (i = 0; i<=8; i++) hash_result.append(char(0));
    hash_result.push_front(SFPData[0x62]);
    hash_result.push_front(char(0));
    hash_result.push_front(char(0));
    crc = Crc32(hash_result, 28);
    hash_result.append(char(crc & 0xFF));
    hash_result.append(char((crc >> 8) & 0xFF));
    hash_result.append(char((crc >> 16) & 0xFF));
    hash_result.append(char((crc >> 24) & 0xFF));
    for (i = 0; i<32; i++) SFPData[i + 0x60] = hash_result[i];
    hexEdit->setData(SFPData);
    on_pushButton_parsing_clicked();
}

void MainWindow::crc32show()
{
    ui->crcEdit->clear();
    ui->crcEdit->setText( hexiAddr( Crc32(SFPData, 0x200) ));
}

void MainWindow::on_actionSave_triggered()
{
    int size = calcSize();
    SFPData = hexEdit->data();
    SFPData.resize(size);
    ui->statusBar->showMessage(tr("Saving file"));
    fileName = QFileDialog::getSaveFileName(this,
                                QString(tr("Save file")),
                                lastDirectory,
                                "Data Images (*.bin *.BIN);;All files (*.*)");
    if (fileName.isEmpty()) return;
    QFileInfo info(fileName);
    lastDirectory = info.filePath();

    if (QString::compare(info.suffix(), "bin", Qt::CaseInsensitive)) fileName = fileName + ".bin";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::about(this, tr("Error"), tr("Error saving file!"));
        return;
    }
    file.write(SFPData);
    file.close();
    SFPData.resize(0x200);
}

void MainWindow::on_lineEdit_vendorid_editingFinished()
{
    if (hexToInt(ui->lineEdit_vendorid->text()) > 63)
    {
        QMessageBox::about(this, tr("Error"), tr("Vendor ID is incorrect!"));
        return;
    }
    SFPData[0x62] = static_cast<char>(hexToInt(ui->lineEdit_vendorid->text()));
    hexEdit->setData(SFPData);
}

void MainWindow::on_lineEdit_manid0_editingFinished()
{
    SFPData[37] = static_cast<char>(hexToInt(ui->lineEdit_manid0->text()));
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_manid1_editingFinished()
{
    SFPData[38] = static_cast<char>(hexToInt(ui->lineEdit_manid1->text()));
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_manid2_editingFinished()
{
    SFPData[39] = static_cast<char>(hexToInt(ui->lineEdit_manid2->text()));
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_comboBox_modtype_currentIndexChanged(int index)
{
    if (index > 0)
    {
      SFPData[0] = static_cast<char>(ui->comboBox_modtype->itemData(index).toInt());
      hexEdit->setData(SFPData);
      checkSumsUpdate();
    }
}

void MainWindow::on_comboBox_connector_currentIndexChanged(int index)
{
    if (index > 0)
    {
      SFPData[2] = static_cast<char>(ui->comboBox_connector->itemData(index).toInt());
      hexEdit->setData(SFPData);
      checkSumsUpdate();
    }
}

void MainWindow::on_comboBox_wavelength_currentIndexChanged(int index)
{
    int val = 0;
    if (index > 0)
    {
      val = ui->comboBox_wavelength->itemData(index).toInt();
      SFPData[60] = static_cast<char>(val >> 8);
      SFPData[61] = static_cast<char>(val & 0xff);
      hexEdit->setData(SFPData);
      checkSumsUpdate();
    }
}

void MainWindow::on_lineEdit_manuf_editingFinished()
{
    int i;
    QString txt;
    QByteArray fromTxt;
    fromTxt.resize(16);
    txt = ui->lineEdit_manuf->text();
    fromTxt = txt.toLatin1();
    for (i=0; i < 16; i++)
    {
        if (i < fromTxt.size()) SFPData[i + 0x14] = fromTxt[i];
        else SFPData[i + 0x14] = 0x20;
    }
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_model_editingFinished()
{
    int i;
    QString txt;
    QByteArray fromTxt;
    fromTxt.resize(16);
    txt = ui->lineEdit_model->text();
    fromTxt = txt.toLatin1();
    for (i=0; i < 16; i++)
    {
        if (i < fromTxt.size()) SFPData[i + 0x28] = fromTxt[i];
        else SFPData[i + 0x28] = 0x20;
    }
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}
void MainWindow::on_lineEdit_revision_editingFinished()
{
    int i;
    QString txt;
    QByteArray fromTxt;
    fromTxt.resize(16);
    txt = ui->lineEdit_revision->text();
    fromTxt = txt.toLatin1();
    for (i=0; i < 4; i++)
    {
        if (i < fromTxt.size()) SFPData[i + 0x38] = fromTxt[i];
        else SFPData[i + 0x38] = 0x20;
    }
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_sernum_editingFinished()
{
    int i;
    QString txt;
    QByteArray fromTxt;
    fromTxt.resize(16);
    txt = ui->lineEdit_sernum->text();
    fromTxt = txt.toLatin1();
    for (i=0; i < 16; i++)
    {
        if (i < fromTxt.size()) SFPData[i + 0x44] = fromTxt[i];
        else SFPData[i + 0x44] = 0x20;
    }
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_ifspeed_editingFinished()
{
    SFPData[12] = static_cast<char>(ui->lineEdit_ifspeed->text().toInt() / 100);
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_925_editingFinished()
{
    SFPData[14] = static_cast<char>(ui->lineEdit_925->text().toInt());
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_50125_editingFinished()
{
    SFPData[16] = static_cast<char>(ui->lineEdit_50125->text().toInt() / 10);
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_62125_editingFinished()
{
    SFPData[17] = static_cast<char>(ui->lineEdit_62125->text().toInt() / 10);
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}
void MainWindow::on_lineEdit_day_editingFinished()
{
    QString tmp;
    QList<QString> matrix;
    tmp = ui->lineEdit_day->text();
    matrix.push_back(tmp);
    SFPData[88] = matrix[0][0].toLatin1();
    SFPData[89] = matrix[0][1].toLatin1();
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_mon_editingFinished()
{
    QString tmp;
    QList<QString> matrix;
    tmp = ui->lineEdit_mon->text();
    matrix.push_back(tmp);
    SFPData[86] = matrix[0][0].toLatin1();
    SFPData[87] = matrix[0][1].toLatin1();
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}

void MainWindow::on_lineEdit_year_editingFinished()
{
    QString tmp;
    QList<QString> matrix;
    tmp = ui->lineEdit_year->text();
    matrix.push_back(tmp);
    SFPData[84] = matrix[0][2].toLatin1();
    SFPData[85] = matrix[0][3].toLatin1();
    hexEdit->setData(SFPData);
    checkSumsUpdate();
}
void MainWindow::on_checkBox_1_clicked()
{
    if (!(ui->checkBox_1->isChecked())) ui->checkBox_1->setChecked(true);
}

void MainWindow::on_checkBox_2_clicked()
{
    if (!(ui->checkBox_2->isChecked()))
    {
        ui->checkBox_3->setChecked(false);
        ui->checkBox_4->setChecked(false);
        ui->checkBox_3->setEnabled(false);
        ui->checkBox_4->setEnabled(false);
    }
    else ui->checkBox_3->setEnabled(true);
}

void MainWindow::on_checkBox_3_clicked()
{
   if (!(ui->checkBox_3->isChecked()))
   {
       ui->checkBox_4->setChecked(false);
       ui->checkBox_4->setEnabled(false);
   }
   else ui->checkBox_4->setEnabled(true);
}

int MainWindow::calcSize()
{
 int ret = 0x200;
 if (ui->checkBox_1->isChecked()) ret = 0x80;
 if (ui->checkBox_2->isChecked()) ret = ret + 0x80;
 if (ui->checkBox_3->isChecked()) ret = ret + 0x80;
 if (ui->checkBox_4->isChecked()) ret = ret + 0x80;
 return ret;
}

void MainWindow::on_actionRead_SFP_triggered()
{
    doNotDisturb();
    int size = calcSize();
    int res = 0;
    int i = 0;
    uint8_t *buf;
    buf = (uint8_t *)malloc(static_cast<unsigned long>(0x200));
    for (i=0; i < 0x200; i++) buf[i] = 0xff;
    statusCh341a = ch341aConnect();
    ch341StatusFlashing();
    if (statusCh341a != 0)
      {
          QMessageBox::about(this, tr("Error"), tr("Programmer CH341a is not connected!"));
          ch341aShutdown();
          doNotDisturbCancel();
          return;
      }
    else
    {
            res = ch341readEEPROM_param(buf, 0, static_cast<uint32_t>(size), static_cast<uint32_t>(size), 0x08, 0x11);
            if (res < 0)
            {
                QMessageBox::about(this, tr("Error"), tr("Error reading SFP module data."));
                return;
            }
            for (i=0; i< 0x200; i++)
            {
                SFPData[i] = static_cast<char>(buf[i]);
            }
            hexEdit->setData(SFPData);
            on_pushButton_parsing_clicked();

        ch341aShutdown();
        doNotDisturbCancel();
    }

}

void MainWindow::on_actionWrite_to_SFP_triggered() //beta - no password...
{
    doNotDisturb();
    int size = calcSize();
    int res = 0;
    int i = 0;
    uint8_t *buf;
    buf = (uint8_t *)malloc(static_cast<unsigned long>(0x200));
    for (i=0; i < 0x200; i++) buf[i] = 0xff;
    if (currentPass.id > 0) writePassword();
    statusCh341a = ch341aConnect();
    ch341StatusFlashing();
    if (statusCh341a != 0)
      {
          QMessageBox::about(this, tr("Error"), tr("Programmer CH341a is not connected!"));
          ch341aShutdown();
          doNotDisturbCancel();
          return;
      }
    else
    {
        SFPData = hexEdit->data();
        for (i=0; i< size; i++)
        {
             buf[i] = static_cast<uint8_t>(SFPData[i]) ;
        }
        res = ch341writeEEPROM_param(buf, 0, static_cast<uint32_t>(size), 0x08, 0x11);  //- correct writting first 0x17f
        if (res < 0)
        {
            QMessageBox::about(this, tr("Error"), tr("Error writing SFP module data."));
            return;
        }
        ch341aShutdown();
        doNotDisturbCancel();
    }
}

void MainWindow::writePassword()
{
    if ((currentPass.id == 0) || (currentPass.id > 4))
        return;
    else
    {
        int res;
        uint32_t i = 0;
        uint8_t *buf, *buf1;

        buf = (uint8_t *)malloc(static_cast<unsigned long>(0x10));
        buf1 = (uint8_t *)malloc(static_cast<unsigned long>(0x02));

        buf[0] = static_cast<uint8_t>((currentPass.password >> 24) & 0xff);
        buf[1] = static_cast<uint8_t>((currentPass.password >> 16) & 0xff);
        buf[2] = static_cast<uint8_t>((currentPass.password >>  8) & 0xff);
        buf[3] = static_cast<uint8_t>(currentPass.password & 0xff);

           statusCh341a = ch341aConnect();
           ch341StatusFlashing();
           if (statusCh341a != 0)
             {
                 QMessageBox::about(this, tr("Error"), tr("Programmer CH341a is not connected!"));
                 ch341aShutdown();
                 return;
             }
           else
           {
              for (i= 0; i < 4; i++)
              {
                 buf1[0] = buf[i];
                 res = ch341writeEEPROM_param(buf1, currentPass.address+i, static_cast<uint32_t>(0x01), 0x08, 0x11);  //- correct writting first 0x17f
                 if (res < 0)
                 {
                     QMessageBox::about(this, tr("Error"), tr("Error writing SFP module data."));
                     return;
                 }

              }
              ch341aShutdown();
           }

        if (ui->checkBox_4->isChecked() == true)
        {
            for (i= 0; i < 4; i++)
            {
                SFPData[currentPass.address+i] = static_cast<char>(buf[i]);
            }
        }
        hexEdit->setData(SFPData);
        on_pushButton_parsing_clicked();
   }
}

void MainWindow::on_actionSet_module_password_triggered()
{
    DialogPass* passDialog = new DialogPass();
    passDialog->show();
    passDialog->setID(currentPass.id, currentPass.address, currentPass.password);
    connect(passDialog, SIGNAL(sendID(uint8_t)), this, SLOT(receiveID(uint8_t)));
    connect(passDialog, SIGNAL(sendUserPass(uint32_t, uint32_t)), this, SLOT(receiveUserPass(uint32_t, uint32_t)));
}

void MainWindow::on_actionAbout_triggered()
{
    DialogAbout* aboutDialog = new DialogAbout();
    aboutDialog->show();
}

 void MainWindow::receiveID(uint8_t id)
 {
   if (id < 5) currentPass = pass[id];
   else currentPass = pass[0];
 }

 void MainWindow::receiveUserPass(uint32_t addr, uint32_t passw)
 {
    pass[4].address = addr;
    pass[4].password = passw;
 }

 void MainWindow::on_actionUndo_triggered()
 {
     hexEdit->undo();
 }

 void MainWindow::on_actionRedo_triggered()
 {
     hexEdit->redo();
 }

 void MainWindow::resizeEvent(QResizeEvent* event)
 {
    QMainWindow::resizeEvent(event);
    hexEdit->setGeometry(0,0,ui->frame->width(),ui->frame->height());
 }

 void MainWindow::ch341StatusFlashing()
 {
     if (statusCh341a == 0)
     {
         ui->eStatus->setText(tr("Connected"));
         ui->eStatus -> setStyleSheet("QLineEdit {border: 2px solid gray;border-radius: 5px;color:#000;background:#9f0;font-weight:600;border-style:inset;}");
     }
     else
     {
         ui->eStatus->setText(tr("Not connected"));
         ui->eStatus -> setStyleSheet("QLineEdit {border: 2px solid gray;border-radius: 5px;color:#fff;background:#f00;font-weight:600;border-style:inset;}");
     }
 }

 void MainWindow::slotTimerAlarm()
 {
         statusCh341a = ch341aConnect();
         ch341StatusFlashing();
         ch341aShutdown();
 }

 void MainWindow::doNotDisturb()
 {
    ui->actionLoad->setDisabled(true);

    ui->actionSave->setDisabled(true);
    ui->actionRead_SFP->setDisabled(true);
    ui->actionWrite_to_SFP->setDisabled(true);
    ui->actionSet_module_password->setDisabled(true);
    ui->actionAbout->setDisabled(true);
    ui->actionUndo->setDisabled(true);
    ui->actionRedo->setDisabled(true);

    ui->pushButton_magic->blockSignals(true);
    ui->pushButton_parsing->blockSignals(true);
    ui->pushButton_checksum->blockSignals(true);

    hexEdit->blockSignals(true);
    timer->stop();
 }

 void MainWindow::doNotDisturbCancel()
 {
     ui->actionLoad->setDisabled(false);

     ui->actionSave->setDisabled(false);
     ui->actionRead_SFP->setDisabled(false);
     ui->actionWrite_to_SFP->setDisabled(false);
     ui->actionSet_module_password->setDisabled(false);
     ui->actionAbout->setDisabled(false);
     ui->actionUndo->setDisabled(false);
     ui->actionRedo->setDisabled(false);

     ui->pushButton_magic->blockSignals(false);
     ui->pushButton_parsing->blockSignals(false);
     ui->pushButton_checksum->blockSignals(false);

     hexEdit->blockSignals(false);
     timer->start();
 }

//*****************************************************
//       HEX ULTLITY by Mikhail Medvedev
//*****************************************************
QString MainWindow::bytePrint(unsigned char z)
{
    unsigned char s;
    s = z / 16;
    if (s > 0x9) s = s + 0x37;
    else s = s + 0x30;
    z = z % 16;
    if (z > 0x9) z = z + 0x37;
    else z = z + 0x30;
    return QString(s) + QString(z);
}

QString MainWindow::checkSumLo()
{
    int i, cur_buf;
    cur_buf = 0;
    for (i = 0; i<=62; i++)
    {
        cur_buf = cur_buf + SFPData[i];
        if (cur_buf>255) cur_buf = cur_buf - 256;
    }
    return bytePrint(static_cast<unsigned char>(cur_buf &0xff));
}

QString MainWindow::checkSumHi()
{
    int i, cur_buf;
    cur_buf = 0;
    for (i = 64; i<=94; i++)
    {
        cur_buf = cur_buf + SFPData[i];
        if (cur_buf>255) cur_buf = cur_buf - 256;
    }
    return bytePrint(static_cast<unsigned char>(cur_buf &0xff));
}

uint32_t MainWindow::hexToInt(QString str)
{
    unsigned char c;
    uint32_t len = static_cast<uint32_t>(str.length());
    QByteArray bstr = str.toLocal8Bit();
    if ((len > 0) && (len < 8))
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

uint_least32_t MainWindow::Crc32(QByteArray &buf, size_t len)
{
    const uint_least32_t Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};
    uint_least32_t crc = 0xFFFFFFFF;
    int i = 0;
    while (len--)
    {
        crc = (crc >> 8) ^ Crc32Table[(crc ^ static_cast<unsigned int>(buf[i])) & 0xFF];
        i++;
    }
    return crc ^ 0xFFFFFFFF;
}

QString MainWindow::hexiAddr(uint32_t add)
{
   QString rez = "";
   uint8_t A,B,C,D;
   D = 0xFF & add;
   add = add >> 8;
   C = 0xFF & add;
   add = add >> 8;
   B = 0xFF & add;
   add = add >> 8;
   A = 0xFF & add;
   rez = bytePrint(A) + bytePrint(B) + bytePrint(C) + bytePrint(D);
   return rez;
}
