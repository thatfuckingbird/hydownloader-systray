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

#pragma once

#include <QMainWindow>
#include <QTime>

namespace Ui
{
    class MainWindow;
}

class QSystemTrayIcon;
class QAction;
class QTimer;
class QSettings;

class HyDownloaderConnection;
class HyDownloaderLogModel;
class HyDownloaderSingleURLQueueModel;
class HyDownloaderSubscriptionModel;
class QSortFilterProxyModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString& settingsFile, QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_logFilterLineEdit_textEdited(const QString& arg1);
    void on_refreshLogButton_clicked();
    void on_loadSubscriptionLogButton_clicked();
    void on_loadSingleURLQueueLogButton_clicked();
    void on_loadDaemonLogButton_clicked();
    void on_copyLogToClipboardButton_clicked();
    void on_subFilterLineEdit_textEdited(const QString& arg1);
    void on_viewLogForSubButton_clicked();
    void on_refreshSubsButton_clicked();
    void on_deleteSelectedSubsButton_clicked();
    void on_addSubButton_clicked();
    void on_urlsFilterLineEdit_textEdited(const QString& arg1);
    void on_viewLogForURLButton_clicked();
    void on_refreshURLsButton_clicked();
    void on_deleteSelectedURLsButton_clicked();
    void on_addURLButton_clicked();
    void setStatusText(const QString& text);
    Qt::GlobalColor statusToColor(const QString& statusText);
    Qt::GlobalColor queueSizeToColor(int size);
    void on_recheckSubsButton_clicked();
    void on_pauseSubsButton_clicked();
    void on_retryURLsButton_clicked();
    void on_pauseURLsButton_clicked();

private:
    Ui::MainWindow* ui = nullptr;
    QSettings* settings = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
    QAction* downloadURLAction = nullptr;
    QAction* subscriptionsAction = nullptr;
    QAction* singleURLQueueAction = nullptr;
    QAction* logsAction = nullptr;
    QAction* quitAction = nullptr;
    QAction* pauseSubsAction = nullptr;
    QAction* pauseSingleURLsAction = nullptr;
    QAction* pauseAllAction = nullptr;
    QAction* resumeAllAction = nullptr;
    QAction* shutdownAction = nullptr;
    QAction* runTestsAction = nullptr;
    QAction* runReportAction = nullptr;
    QTimer* statusUpdateTimer = nullptr;
    QTimer* statusUpdateIntervalTimer = nullptr;
    QTime lastUpdateTime;
    QAction* resumeSelectedURLsAction = nullptr;
    QAction* resumeSelectedSubsAction = nullptr;
    HyDownloaderConnection* connection = nullptr;
    HyDownloaderLogModel* logModel = nullptr;
    HyDownloaderSingleURLQueueModel* urlModel = nullptr;
    HyDownloaderSubscriptionModel* subModel = nullptr;
    QSortFilterProxyModel* logFilterModel = nullptr;
    QSortFilterProxyModel* urlFilterModel = nullptr;
    QSortFilterProxyModel* subFilterModel = nullptr;
    void setIcon(const QIcon& icon);

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
};
