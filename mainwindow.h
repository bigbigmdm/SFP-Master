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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include "qhexedit.h"
#include <QCryptographicHash>
#include "dialogpass.h"
extern "C" {
#include "ch341eeprom.h"
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void receiveID(uint8_t);
    void receiveUserPass(uint32_t addr, uint32_t passw);

private slots:
    void on_actionLoad_triggered();
    void on_pushButton_parsing_clicked();
    void on_pushButton_checksum_clicked();
    void on_pushButton_magic_clicked();
    void on_actionSave_triggered();
    void on_lineEdit_vendorid_editingFinished();
    void on_lineEdit_manid0_editingFinished();
    void on_lineEdit_manid1_editingFinished();
    void on_lineEdit_manid2_editingFinished();
    void on_comboBox_modtype_currentIndexChanged(int index);
    void on_comboBox_connector_currentIndexChanged(int index);
    void on_comboBox_wavelength_currentIndexChanged(int index);
    void on_lineEdit_manuf_editingFinished();
    void on_lineEdit_model_editingFinished();
    void on_lineEdit_revision_editingFinished();
    void on_lineEdit_sernum_editingFinished();
    void on_lineEdit_ifspeed_editingFinished();
    void on_lineEdit_925_editingFinished();
    void on_lineEdit_50125_editingFinished();
    void on_lineEdit_62125_editingFinished();
    void on_lineEdit_day_editingFinished();
    void on_lineEdit_mon_editingFinished();
    void on_lineEdit_year_editingFinished();
    void on_checkBox_3_clicked();
    void on_checkBox_2_clicked();
    void on_checkBox_1_clicked();
    void on_actionRead_SFP_triggered();
    void on_actionWrite_to_SFP_triggered();
    void on_actionSet_module_password_triggered();
    void on_actionAbout_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();

private:
    int statusCh341a;
    Ui::MainWindow *ui;
    QByteArray SFPData;
    QColor defaultTextColor;
    QHexEdit *hexEdit;
    bool cmdStarted;
    QString lastDirectory;
    QString fileName;
    QString bytePrint(unsigned char z);
    uint32_t hexToInt(QString str);
    QString checkSumLo();
    QString checkSumHi();
    QString hexiAddr(uint32_t a);
    void crc32show();
    void checkSumsUpdate();
    void writePassword();
    uint_least32_t Crc32(QByteArray &buf, size_t len);
    int calcSize();

public:
    struct libusb_device_handle *devHandle;
    struct passwords {
        uint8_t id;
        QString name;
        uint32_t address;
        uint32_t password;
    };
    passwords currentPass;
    passwords pass[5];
};


#endif // MAINWINDOW_H
