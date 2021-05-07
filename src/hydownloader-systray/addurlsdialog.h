#pragma once

#include <QDialog>

namespace Ui
{
    class AddURLsDialog;
}

class AddURLsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddURLsDialog(QWidget* parent = nullptr);
    QStringList getURLs() const;
    bool startPaused() const;
    void setStartPaused(bool paused);
    ~AddURLsDialog();

private:
    Ui::AddURLsDialog* ui;
};
