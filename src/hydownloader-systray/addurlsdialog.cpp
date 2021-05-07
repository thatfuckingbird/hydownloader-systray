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
