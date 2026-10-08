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
#ifndef DIALOGPASS_H
#define DIALOGPASS_H

#include <QDialog>
#include <QStringList>
#include <QList>

class QListWidgetItem;

namespace Ui {
class DialogPass;
}

class DialogPass : public QDialog
{
    Q_OBJECT

public:
    // Write-password mode shared with MainWindow (stored in currentPass.id).
    enum Mode { ModeNone = 0, ModeGuess = 1, ModeSaved = 2 };

    explicit DialogPass(QWidget *parent = nullptr);
    ~DialogPass();
    void setBuiltInList(const QStringList &items, const QStringList &names,
                        const QList<quint32> &values);
    void setSelection(int mode, uint32_t addr, uint32_t pass);
    uint32_t hexToInt(QString str);
    QString bytePrt(unsigned char z);

private slots:
    void on_pushButton_clicked();            // Ok
    void on_pushButton_add_clicked();
    void on_pushButton_remove_clicked();
    void onBuiltInDoubleClicked(QListWidgetItem *item);

signals:
    void sendSelection(int mode, uint32_t addr, uint32_t pass);

private:
    Ui::DialogPass *ui;
    QStringList builtInNames;
    QList<quint32> builtInValues;

    void loadSaved();
    void saveSaved();
    void addSavedRow(const QString &name, uint32_t addr, uint32_t pass);
};

#endif // DIALOGPASS_H
