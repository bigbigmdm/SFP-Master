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
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //If you need to change the system font - recomment this lines...
    //QFont font("Monospace");
    //font.setStyleHint(QFont::TypeWriter);
    //font.setPointSize(12);
    //QApplication::setFont(font);
    QApplication a(argc, argv);
    QTranslator translator;
        QString translateName = "SFP-Master_" + QLocale::system().name();
        if(translator.load(translateName, "/usr/share/sfp-master/")) a.installTranslator(&translator);
        a.installTranslator(&translator);
    MainWindow w;
    w.show();

    return a.exec();
}
