/*
hydownloader-systray
Copyright (C) 2021  thatfuckingbird

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <cstdlib>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QCommandLineParser p;
    QCommandLineOption opt{"settings", "Settings file", "filename"};
    p.addOption(opt);
    p.process(a);

    QString filename = p.value(opt);
    if(!QFile::exists(filename)) return EXIT_FAILURE;

    MainWindow w{filename};
    return a.exec();
}
