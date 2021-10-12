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

#include "addurlsdialog.h"
#include "ui_addurlsdialog.h"

AddURLsDialog::AddURLsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AddURLsDialog)
{
    ui->setupUi(this);
}

QStringList AddURLsDialog::getURLs() const
{
    QStringList result;
    for(const auto& line: ui->urlTextEdit->toPlainText().split("\n", Qt::SkipEmptyParts)) {
        auto trimmed = line.trimmed();
        if(trimmed.size()) result.append(trimmed);
    }
    return result;
}

bool AddURLsDialog::startPaused() const
{
    return ui->startPausedCheckBox->isChecked();
}

void AddURLsDialog::setStartPaused(bool paused)
{
    ui->startPausedCheckBox->setChecked(paused);
}

QString AddURLsDialog::additionalData() const
{
    return ui->additionalDataLineEdit->text().trimmed();
}

AddURLsDialog::~AddURLsDialog()
{
    delete ui;
}
