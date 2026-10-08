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
#include <QFont>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSettings>
#include <QMessageBox>

DialogPass::DialogPass(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPass)
{
    ui->setupUi(this);
    QFont mono("DejaVu Sans Mono");
    mono.setStyleHint(QFont::Monospace);
    ui->listWidget_builtin->setFont(mono);
    ui->tableSaved->horizontalHeader()->setStretchLastSection(true);
    connect(ui->listWidget_builtin, &QListWidget::itemDoubleClicked,
            this, &DialogPass::onBuiltInDoubleClicked);
    loadSaved();
}

DialogPass::~DialogPass()
{
    delete ui;
}

// Fill the read-only list of built-in passwords (reference + source for the
// double-click "add to my list" shortcut). Populated by MainWindow.
void DialogPass::setBuiltInList(const QStringList &items, const QStringList &names,
                                const QList<quint32> &values)
{
    ui->listWidget_builtin->clear();
    ui->listWidget_builtin->addItems(items);
    builtInNames = names;
    builtInValues = values;
}

// Preselect the write mode and, for a saved password, highlight a matching row.
void DialogPass::setSelection(int mode, uint32_t addr, uint32_t pass)
{
    if (mode == ModeGuess)      ui->radio_guess->setChecked(true);
    else if (mode == ModeSaved) ui->radio_saved->setChecked(true);
    else                        ui->radio_none->setChecked(true);

    if (mode == ModeSaved)
    {
        for (int r = 0; r < ui->tableSaved->rowCount(); r++)
        {
            uint32_t a = hexToInt(ui->tableSaved->item(r, 1)->text());
            uint32_t p = hexToInt(ui->tableSaved->item(r, 2)->text());
            if (a == addr && p == pass) { ui->tableSaved->selectRow(r); break; }
        }
    }
}

void DialogPass::addSavedRow(const QString &name, uint32_t addr, uint32_t pass)
{
    int r = ui->tableSaved->rowCount();
    ui->tableSaved->insertRow(r);
    ui->tableSaved->setItem(r, 0, new QTableWidgetItem(name));
    ui->tableSaved->setItem(r, 1, new QTableWidgetItem(QString("%1").arg(addr, 3, 16, QChar('0')).toUpper()));
    ui->tableSaved->setItem(r, 2, new QTableWidgetItem(QString("%1").arg(pass, 8, 16, QChar('0'))));
    ui->tableSaved->selectRow(r);
}

// Persisted set of user passwords (survives restarts).
void DialogPass::loadSaved()
{
    QSettings s;
    int n = s.beginReadArray("savedPasswords");
    for (int i = 0; i < n; i++)
    {
        s.setArrayIndex(i);
        addSavedRow(s.value("name").toString(),
                    s.value("addr").toUInt(),
                    s.value("pass").toUInt());
    }
    s.endArray();
    if (ui->tableSaved->rowCount() == 0)   // first run: seed a common default
        addSavedRow("Default", 0x17b, 0x00001011);
    ui->tableSaved->clearSelection();
}

void DialogPass::saveSaved()
{
    QSettings s;
    s.beginWriteArray("savedPasswords");
    for (int r = 0; r < ui->tableSaved->rowCount(); r++)
    {
        s.setArrayIndex(r);
        s.setValue("name", ui->tableSaved->item(r, 0) ? ui->tableSaved->item(r, 0)->text() : "");
        s.setValue("addr", hexToInt(ui->tableSaved->item(r, 1) ? ui->tableSaved->item(r, 1)->text() : "17B"));
        s.setValue("pass", hexToInt(ui->tableSaved->item(r, 2) ? ui->tableSaved->item(r, 2)->text() : "0"));
    }
    s.endArray();
}

void DialogPass::on_pushButton_add_clicked()
{
    addSavedRow(tr("New"), 0x17b, 0);
    ui->tableSaved->editItem(ui->tableSaved->item(ui->tableSaved->rowCount() - 1, 0));
}

void DialogPass::on_pushButton_remove_clicked()
{
    int r = ui->tableSaved->currentRow();
    if (r >= 0) ui->tableSaved->removeRow(r);
}

// Double-clicking a built-in password appends it to the user's saved list.
void DialogPass::onBuiltInDoubleClicked(QListWidgetItem *item)
{
    int row = ui->listWidget_builtin->row(item);
    if (row < 0 || row >= builtInValues.size()) return;
    QString name = (row < builtInNames.size()) ? builtInNames[row] : tr("Built-in");
    addSavedRow(name, 0x17b, builtInValues[row]);
    ui->radio_saved->setChecked(true);
}

void DialogPass::on_pushButton_clicked()
{
    int mode = ModeNone;
    if (ui->radio_guess->isChecked()) mode = ModeGuess;
    if (ui->radio_saved->isChecked()) mode = ModeSaved;

    uint32_t addr = 0x17b, pass = 0;
    if (mode == ModeSaved)
    {
        int r = ui->tableSaved->currentRow();
        if (r < 0)
        {
            QMessageBox::warning(this, tr("Password settings"),
                                 tr("Select a saved password, or choose another option."));
            return;
        }
        addr = hexToInt(ui->tableSaved->item(r, 1) ? ui->tableSaved->item(r, 1)->text() : "17B");
        pass = hexToInt(ui->tableSaved->item(r, 2) ? ui->tableSaved->item(r, 2)->text() : "0");
    }

    saveSaved();                 // persist any edits/additions
    emit sendSelection(mode, addr, pass);
    DialogPass::close();
}

uint32_t DialogPass::hexToInt(QString str)
{
    unsigned char c;
    str = str.trimmed();
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
