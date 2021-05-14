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

#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QSslError>
#include <QJsonObject>
#include <QMessageBox>
#include <QShortcut>
#include <QInputDialog>
#include <QCloseEvent>
#include "datetimeformatdelegate.h"
#include "hydownloaderconnection.h"
#include "hydownloaderlogmodel.h"
#include "hydownloadersubscriptionmodel.h"
#include "hydownloadersingleurlqueuemodel.h"
#include "hydownloadersubscriptionchecksmodel.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "addurlsdialog.h"

QPixmap drawSystrayIcon(const QVector<Qt::GlobalColor>& data)
{
    QPixmap iconPixmap(QSize(64, 64));
    iconPixmap.fill(Qt::black);

    double cellSize = 4;
    while(std::ceil(data.size() / std::floor(64.0 / (cellSize + 2))) * (cellSize + 2) <= 64) cellSize += 2;
    double cellsInARow = std::floor(64 / cellSize);
    double horizontalOffset = 0; //(64-cells_in_a_row*cell_size)/2.0;
    double verticalOffset = 0; //(64-std::ceil(data.size()/cells_in_a_row)*cell_size)/2.0;

    QPainter iconPainter(&iconPixmap);

    for(int i = 0; i < data.size(); ++i) {
        int currRow = std::floor(i / cellsInARow);
        int currCol = i % int(cellsInARow);
        iconPainter.fillRect(QRectF(horizontalOffset + currCol * cellSize, verticalOffset + currRow * cellSize, cellSize, cellSize), data[i]);
    }
    return iconPixmap;
}

MainWindow::MainWindow(const QString& settingsFile, bool startVisible, QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon)
{
    ui->setupUi(this);

    settings = new QSettings{settingsFile, QSettings::IniFormat};

    const bool shouldBeVisible = startVisible || settings->value("startVisible").toBool();

    QMenu* manageMenu = new QMenu{this};
    manageMenu->setTitle("Management actions");
    pauseSubsAction = manageMenu->addAction("Pause subscriptions");
    connect(pauseSubsAction, &QAction::triggered, [&] {
        connection->pauseSubscriptions();
    });
    pauseSingleURLsAction = manageMenu->addAction("Pause single URL downloads");
    connect(pauseSingleURLsAction, &QAction::triggered, [&] {
        connection->pauseSingleURLQueue();
    });
    manageMenu->addSeparator();
    pauseAllAction = manageMenu->addAction("Pause all");
    connect(pauseAllAction, &QAction::triggered, [&] {
        connection->pauseSingleURLQueue();
        connection->pauseSubscriptions();
    });
    resumeAllAction = manageMenu->addAction("Resume all");
    connect(resumeAllAction, &QAction::triggered, [&] {
        connection->resumeSingleURLQueue();
        connection->resumeSubscriptions();
    });
    manageMenu->addSeparator();
    runTestsAction = manageMenu->addAction("Run download tests...");
    connect(runTestsAction, &QAction::triggered, [&] {
        bool ok = true;
        QStringList tests = QInputDialog::getText(this, "Run tests", "Tests:", QLineEdit::Normal, settings->value("defaultTests").toString(), &ok).split(",");
        for(auto& str: tests) str = str.trimmed();
        if(ok && !tests.isEmpty()) connection->runTests(tests);
    });
    runReportAction = manageMenu->addAction("Generate report...");
    connect(runReportAction, &QAction::triggered, [&] {
        bool verbose = QMessageBox::question(this, "Report", "Run verbose report?") == QMessageBox::Yes;
        connection->runReport(verbose);
    });
    manageMenu->addSeparator();
    shutdownAction = manageMenu->addAction("Shut down hydownloader");
    connect(shutdownAction, &QAction::triggered, [&] {
        connection->shutdown();
    });

    QMenu* trayMenu = new QMenu{this};
    downloadURLAction = trayMenu->addAction("Download URL");
    connect(downloadURLAction, &QAction::triggered, [&] {
        launchAddURLsDialog(false);
    });

    trayMenu->addSeparator();
    subscriptionsAction = trayMenu->addAction("Manage subscriptions");
    connect(subscriptionsAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->subsTab);
        subModel->refresh();
        show();
        raise();
    });
    singleURLQueueAction = trayMenu->addAction("Manage single URL queue");
    connect(singleURLQueueAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->singleURLsTab);
        urlModel->refresh();
        show();
        raise();
    });
    checksAction = trayMenu->addAction("Review subscription checks");
    connect(checksAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->subChecksTab);
        subCheckModel->refresh();
        show();
        raise();
    });
    logsAction = trayMenu->addAction("Review logs");
    connect(logsAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->logsTab);
        logModel->refresh();
        show();
        raise();
    });
    trayMenu->addSeparator();
    trayMenu->addMenu(manageMenu);
    trayMenu->addSeparator();
    quitAction = trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

    trayIcon->setContextMenu(trayMenu);
    setIcon(QIcon{drawSystrayIcon({Qt::gray})});
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, [&] {
        if(isVisible()) {
            hide();
        } else {
            show();
            raise();
        }
    });

    connection = new HyDownloaderConnection{this};
    connection->setAccessKey(settings->value("accessKey").toString());
    connection->setAPIURL(settings->value("apiURL").toString());
    connection->setCertificateVerificationEnabled(false);
    logModel = new HyDownloaderLogModel{};
    subModel = new HyDownloaderSubscriptionModel{};
    subCheckModel = new HyDownloaderSubscriptionChecksModel{};
    urlModel = new HyDownloaderSingleURLQueueModel{};

    logFilterModel = new QSortFilterProxyModel{};
    logFilterModel->setSourceModel(logModel);
    logFilterModel->setFilterKeyColumn(-1);
    ui->logTableView->setModel(logFilterModel);
    ui->logTableView->setItemDelegate(new DateTimeFormatDelegate{});
    connect(logModel, &QAbstractTableModel::rowsInserted, [&] {
        ui->logTableView->scrollToBottom();
    });

    urlFilterModel = new QSortFilterProxyModel{};
    urlFilterModel->setSourceModel(urlModel);
    urlFilterModel->setFilterKeyColumn(-1);
    ui->urlsTableView->setModel(urlFilterModel);
    ui->urlsTableView->setItemDelegate(new DateTimeFormatDelegate{});

    subFilterModel = new QSortFilterProxyModel{};
    subFilterModel->setSourceModel(subModel);
    subFilterModel->setFilterKeyColumn(-1);
    ui->subTableView->setModel(subFilterModel);
    ui->subTableView->setItemDelegate(new DateTimeFormatDelegate{});

    subCheckFilterModel = new QSortFilterProxyModel{};
    subCheckFilterModel->setSourceModel(subCheckModel);
    subCheckFilterModel->setFilterKeyColumn(-1);
    ui->subCheckTableView->setModel(subCheckFilterModel);
    ui->subCheckTableView->setItemDelegate(new DateTimeFormatDelegate{});

    connect(logModel, &HyDownloaderLogModel::statusTextChanged, [&](const QString& statusText) {
        ui->currentLogLabel->setText(statusText);
    });

    connect(subCheckModel, &HyDownloaderSubscriptionChecksModel::statusTextChanged, [&](const QString& statusText) {
        ui->currentSubChecksLabel->setText(statusText);
    });

    logModel->setConnection(connection);
    subModel->setConnection(connection);
    subCheckModel->setConnection(connection);
    urlModel->setConnection(connection);

    connect(connection, &HyDownloaderConnection::networkError, [&](std::uint64_t, int status, QNetworkReply::NetworkError, const QString& errorText) {
        lastUpdateTime = QTime::currentTime();
        if(status == 404) return; //expected, if requesting logs that were not created yet
        setStatusText(QString{"Network error (status %1): %2"}.arg(QString::number(status), errorText));
        setIcon(QIcon{drawSystrayIcon({Qt::red})});
    });
    connect(connection, &HyDownloaderConnection::sslErrors, [&](QNetworkReply*, const QList<QSslError>& errors) {
        QString errorString;
        for(const auto& error: errors) {
            errorString += error.errorString() + "\n";
        }
        setStatusText(QString{"SSL errors:\n%1"}.arg(errorString.trimmed()));
        setIcon(QIcon{drawSystrayIcon({Qt::red})});
        lastUpdateTime = QTime::currentTime();
    });
    connect(connection, &HyDownloaderConnection::statusInformationReceived, [&](std::uint64_t, const QJsonObject& info) {
        setIcon(QIcon{drawSystrayIcon({statusToColor(info["subscription_worker_status"].toString()),
                                       queueSizeToColor(info["subscriptions_due"].toInt()),
                                       statusToColor(info["url_worker_status"].toString()),
                                       queueSizeToColor(info["urls_queued"].toInt())})});
        setStatusText(QString{"Subscriptions due: %1, URLs queued: %2\nSubscription worker status: %3\nURL worker status: %4"}.arg(
          QString::number(info["subscriptions_due"].toInt()),
          QString::number(info["urls_queued"].toInt()),
          info["subscription_worker_status"].toString(),
          info["url_worker_status"].toString()));
        lastUpdateTime = QTime::currentTime();
    });

    statusUpdateTimer = new QTimer{this};
    connect(statusUpdateTimer, &QTimer::timeout, connection, &HyDownloaderConnection::requestStatusInformation);
    statusUpdateTimer->setInterval(settings->value("updateInterval").toInt());
    statusUpdateTimer->start();

    statusUpdateIntervalTimer = new QTimer{this};
    connect(statusUpdateIntervalTimer, &QTimer::timeout, [&] {
        if(lastUpdateTime.secsTo(QTime::currentTime()) >= 30) {
            setStatusText(QString{"No status update received in the past 30 seconds"});
            setIcon(QIcon{drawSystrayIcon({Qt::red})});
        }
    });
    lastUpdateTime = QTime::currentTime();
    statusUpdateIntervalTimer->setInterval(30000);
    statusUpdateIntervalTimer->start();

    ui->mainTabWidget->setCurrentIndex(0);
    ui->urlsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->subTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    connect(ui->subTableView->selectionModel(), &QItemSelectionModel::selectionChanged, [&] {
        int selectionSize = ui->subTableView->selectionModel()->selectedRows().size();
        ui->recheckSubsButton->setEnabled(selectionSize > 0);
        ui->deleteSelectedSubsButton->setEnabled(selectionSize > 0);
        ui->viewLogForSubButton->setEnabled(selectionSize == 1);
        ui->viewChecksForSubButton->setEnabled(selectionSize == 1);
        ui->pauseSubsButton->setEnabled(selectionSize > 0);
    });
    connect(ui->urlsTableView->selectionModel(), &QItemSelectionModel::selectionChanged, [&] {
        int selectionSize = ui->urlsTableView->selectionModel()->selectedRows().size();
        ui->pauseURLsButton->setEnabled(selectionSize > 0);
        ui->archiveURLsButton->setEnabled(selectionSize > 0);
        ui->deleteSelectedURLsButton->setEnabled(selectionSize > 0);
        ui->viewLogForURLButton->setEnabled(selectionSize == 1);
        ui->retryURLsButton->setEnabled(selectionSize > 0);
    });
    connect(ui->subCheckTableView->selectionModel(), &QItemSelectionModel::selectionChanged, [&] {
        int selectionSize = ui->subCheckTableView->selectionModel()->selectedRows().size();
        ui->archiveSubChecksButton->setEnabled(selectionSize > 0);
    });
    QMenu* pauseSubsMenu = new QMenu{this};
    resumeSelectedSubsAction = pauseSubsMenu->addAction("Resume", [&] {
        auto indices = ui->subTableView->selectionModel()->selectedRows();
        for(auto& index: indices) {
            index = subFilterModel->mapToSource(index);
            auto row = subModel->getRowData(index);
            row["paused"] = QJsonValue{0};
            subModel->setRowData(index, row);
        }
    });
    ui->pauseSubsButton->setMenu(pauseSubsMenu);

    QMenu* pauseURLsMenu = new QMenu{this};
    resumeSelectedURLsAction = pauseURLsMenu->addAction("Resume", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getRowData(index);
            row["paused"] = QJsonValue{0};
            urlModel->setRowData(index, row);
        }
    });
    ui->pauseURLsButton->setMenu(pauseURLsMenu);

    QMenu* archiveURLsMenu = new QMenu{this};
    unarchiveSelectedURLsAction = archiveURLsMenu->addAction("Unarchive", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getRowData(index);
            row["archived"] = QJsonValue{0};
            urlModel->setRowData(index, row);
        }
    });
    ui->archiveURLsButton->setMenu(archiveURLsMenu);

    QMenu* archiveSubChecksMenu = new QMenu{this};
    unarchiveSelectedSubChecksAction = archiveSubChecksMenu->addAction("Unarchive", [&] {
        auto indices = ui->subCheckTableView->selectionModel()->selectedRows();
        for(auto& index: indices) {
            index = subCheckFilterModel->mapToSource(index);
            auto row = subCheckModel->getRowData(index);
            row["archived"] = QJsonValue{0};
            subCheckModel->setRowData(index, row);
        }
    });
    ui->archiveSubChecksButton->setMenu(archiveSubChecksMenu);

    QMenu* retryURLsMenu = new QMenu{this};
    retryURLsMenu->addAction("Retry and force overwrite", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getRowData(index);
            row["status"] = QJsonValue{-1};
            row["overwrite_existing"] = QJsonValue{1};
            urlModel->setRowData(index, row);
        }
    });
    ui->retryURLsButton->setMenu(retryURLsMenu);

    [[maybe_unused]] QShortcut* deleteSubsShortcut = new QShortcut{Qt::Key_Delete, ui->subTableView, ui->deleteSelectedSubsButton, &QToolButton::click};
    [[maybe_unused]] QShortcut* deleteURLsShortcut = new QShortcut{Qt::Key_Delete, ui->urlsTableView, ui->deleteSelectedURLsButton, &QToolButton::click};

    connect(ui->subTableView, &QTableView::customContextMenuRequested, [&](const QPoint& pos) {
        int selectionSize = ui->subTableView->selectionModel()->selectedRows().size();
        if(selectionSize == 0) return;

        QMenu popup;
        if(selectionSize == 1) {
            popup.addAction("View log", ui->viewLogForSubButton, &QToolButton::click);
            popup.addAction("View check history", ui->viewChecksForSubButton, &QToolButton::click);
            popup.addSeparator();
        }
        popup.addAction("Clear last checked time", ui->recheckSubsButton, &QToolButton::click);
        popup.addAction("Pause", ui->pauseSubsButton, &QToolButton::click);
        popup.addAction("Resume", resumeSelectedSubsAction, &QAction::trigger);
        popup.addSeparator();
        popup.addAction("Delete", ui->deleteSelectedSubsButton, &QToolButton::click);
        popup.exec(ui->subTableView->mapToGlobal(pos));
    });
    ui->subTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->urlsTableView, &QTableView::customContextMenuRequested, [&](const QPoint& pos) {
        int selectionSize = ui->urlsTableView->selectionModel()->selectedRows().size();
        if(selectionSize == 0) return;

        QMenu popup;
        if(selectionSize == 1) {
            popup.addAction("View log", ui->viewLogForURLButton, &QToolButton::click);
            popup.addSeparator();
        }
        popup.addAction("Retry", ui->retryURLsButton, &QToolButton::click);
        popup.addAction("Pause", ui->pauseURLsButton, &QToolButton::click);
        popup.addAction("Resume", this->resumeSelectedURLsAction, &QAction::trigger);
        popup.addSeparator();
        popup.addAction("Archive", ui->archiveURLsButton, &QToolButton::click);
        popup.addAction("Unarchive", this->unarchiveSelectedURLsAction, &QAction::trigger);
        popup.addSeparator();
        popup.addAction("Delete", ui->deleteSelectedURLsButton, &QToolButton::click);
        popup.exec(ui->urlsTableView->mapToGlobal(pos));
    });
    ui->urlsTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->logTableView, &QTableView::customContextMenuRequested, [&](const QPoint& pos) {
        QMenu popup;
        popup.addAction("Copy", ui->copyLogToClipboardButton, &QToolButton::clicked);
        popup.exec(ui->logTableView->mapToGlobal(pos));
    });
    ui->logTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->subCheckTableView, &QTableView::customContextMenuRequested, [&](const QPoint& pos) {
        int selectionSize = ui->subCheckTableView->selectionModel()->selectedRows().size();
        if(selectionSize == 0) return;

        QMenu popup;
        popup.addAction("Archive", ui->archiveSubChecksButton, &QToolButton::click);
        popup.addAction("Unarchive", this->unarchiveSelectedSubChecksAction, &QAction::trigger);
        popup.exec(ui->subCheckTableView->mapToGlobal(pos));
    });
    ui->subCheckTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    if(settings->value("applyDarkPalette").toBool()) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(0x47, 0x47, 0x47));
        darkPalette.setColor(QPalette::WindowText, QColor(0xae, 0xae, 0xae));
        darkPalette.setColor(QPalette::Base, QColor(0x40, 0x40, 0x40));
        darkPalette.setColor(QPalette::AlternateBase, QColor(0x30, 0x30, 0x30));
        darkPalette.setColor(QPalette::ToolTipBase, QColor(0x47, 0x47, 0x47));
        darkPalette.setColor(QPalette::ToolTipText, QColor(0xae, 0xae, 0xae));
        darkPalette.setColor(QPalette::Text, QColor(0xae, 0xae, 0xae));
        darkPalette.setColor(QPalette::Button, QColor(0x47, 0x47, 0x47));
        darkPalette.setColor(QPalette::ButtonText, QColor(0xae, 0xae, 0xae));
        darkPalette.setColor(QPalette::BrightText, QColor(0xee, 0xee, 0xee));
        darkPalette.setColor(QPalette::Link, QColor(0xa7, 0xc5, 0xf9));
        darkPalette.setColor(QPalette::LinkVisited, QColor(0xcb, 0xb4, 0xf9));
        darkPalette.setColor(QPalette::Highlight, QColor(0x53, 0x72, 0x8e));
        darkPalette.setColor(QPalette::HighlightedText, QColor(0xae, 0xae, 0xae));
        darkPalette.setColor(QPalette::PlaceholderText, QColor(0xaa, 0xaa, 0xaa));
        darkPalette.setColor(QPalette::Light, QColor(0x67, 0x67, 0x67));
        darkPalette.setColor(QPalette::Midlight, QColor(0x57, 0x57, 0x57));
        darkPalette.setColor(QPalette::Dark, QColor(0x30, 0x30, 0x30));
        darkPalette.setColor(QPalette::Mid, QColor(0x40, 0x40, 0x40));
        darkPalette.setColor(QPalette::Shadow, QColor(0x11, 0x11, 0x11));

        qApp->setPalette(darkPalette);
    }

    if(shouldBeVisible) show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_logFilterLineEdit_textEdited(const QString& arg1)
{
    logFilterModel->setFilterRegularExpression(arg1);
}

void MainWindow::on_refreshLogButton_clicked()
{
    logModel->refresh();
}

void MainWindow::on_loadSubscriptionLogButton_clicked()
{
    int id = QInputDialog::getInt(this, "Load subscription log", "Subscription ID:", 0, 0);
    logModel->loadSubscriptionLog(id);
}

void MainWindow::on_loadSingleURLQueueLogButton_clicked()
{
    int id = QInputDialog::getInt(this, "Load single URL log", "URL ID:", 0, 0);
    logModel->loadSingleURLQueueLog(id);
}

void MainWindow::on_loadDaemonLogButton_clicked()
{
    logModel->loadDaemonLog();
}

void MainWindow::on_copyLogToClipboardButton_clicked()
{
    logModel->copyToClipboard(ui->logTableView->selectionModel()->selectedRows());
}

void MainWindow::on_subFilterLineEdit_textEdited(const QString& arg1)
{
    subFilterModel->setFilterRegularExpression(arg1);
}

void MainWindow::on_viewLogForSubButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) index = subFilterModel->mapToSource(index);
    auto ids = subModel->getIDs(indices);
    if(ids.size() != 1) return;
    logModel->loadSubscriptionLog(ids[0]);
    ui->mainTabWidget->setCurrentWidget(ui->logsTab);
}

void MainWindow::on_refreshSubsButton_clicked()
{
    subModel->refresh();
}

void MainWindow::on_deleteSelectedSubsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) index = subFilterModel->mapToSource(index);
    auto ids = subModel->getIDs(indices);
    if(ids.isEmpty()) return;
    if(QMessageBox::question(this, "Delete subscriptions", QString{"Are you sure you want to delete the %1 selected subscriptions?"}.arg(QString::number(ids.size()))) == QMessageBox::Yes) {
        connection->deleteSubscriptions(ids);
    }
}

void MainWindow::on_addSubButton_clicked()
{
    bool ok = true;
    QString downloader = QInputDialog::getItem(this, "Choose downloader", "Downloader:", settings->value("defaultDownloaders").toStringList(), 0, false, &ok);
    if(!ok || downloader.isEmpty()) return;
    QString keywords = QInputDialog::getText(this, "Keywords", "Keywords:", QLineEdit::Normal, {}, &ok);
    if(!ok || keywords.isEmpty()) return;
    int checkHours = QInputDialog::getInt(this, "Check interval", "Check interval in hours:", settings->value("defaultSubCheckInterval").toInt(), 1, 1000000, 1, &ok);
    if(!ok || checkHours <= 0) return;

    QJsonObject newSub;
    newSub["keywords"] = keywords;
    newSub["downloader"] = downloader;
    newSub["paused"] = true;
    newSub["check_interval"] = checkHours * 3600;
    QJsonArray arr = {newSub};
    connection->addOrUpdateSubscriptions(arr);
    subModel->refresh();
}

void MainWindow::on_urlsFilterLineEdit_textEdited(const QString& arg1)
{
    urlFilterModel->setFilterRegularExpression(arg1);
}

void MainWindow::on_viewLogForURLButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    for(auto& index: indices) index = urlFilterModel->mapToSource(index);
    auto ids = urlModel->getIDs(indices);
    if(ids.size() != 1) return;
    logModel->loadSingleURLQueueLog(ids[0]);
    ui->mainTabWidget->setCurrentWidget(ui->logsTab);
}

void MainWindow::on_refreshURLsButton_clicked()
{
    urlModel->refresh();
}

void MainWindow::on_deleteSelectedURLsButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    for(auto& index: indices) index = urlFilterModel->mapToSource(index);
    auto ids = urlModel->getIDs(indices);
    if(ids.isEmpty()) return;
    if(QMessageBox::question(this, "Delete URLs", QString{"Are you sure you want to delete the %1 selected URLs?"}.arg(QString::number(ids.size()))) == QMessageBox::Yes) {
        connection->deleteURLs(ids);
    }
}

void MainWindow::on_addURLButton_clicked()
{
    launchAddURLsDialog(true);
}

void MainWindow::setStatusText(const QString& text)
{
    trayIcon->setToolTip(text);
    ui->statusBar->showMessage(text);
    ui->statusBar->setToolTip(text);
}

Qt::GlobalColor MainWindow::statusToColor(const QString& statusText)
{
    QString status = statusText.left(statusText.indexOf(':'));
    if(status == "paused") return Qt::yellow;
    if(status == "no information") return Qt::gray;
    if(status == "nothing to do") return Qt::green;
    if(status == "downloading URL" || status == "finished checking URL" || status == "checking subscription" || status == "finished checking subscription") return Qt::darkGreen;
    return Qt::red;
}

Qt::GlobalColor MainWindow::queueSizeToColor(int size)
{
    if(size == 0) return Qt::green;
    if(size <= 5) return Qt::darkGreen;
    if(size <= 10) return Qt::yellow;
    if(size <= 20) return Qt::darkYellow;
    if(size <= 50) return Qt::red;
    return Qt::darkRed;
}

void MainWindow::setIcon(const QIcon& icon)
{
    trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void MainWindow::launchAddURLsDialog(bool paused)
{
    AddURLsDialog diag{this};
    diag.setStartPaused(paused);
    if(diag.exec()) {
        QJsonArray arr;
        for(const auto& url: diag.getURLs()) {
            QJsonObject newURL;
            newURL["url"] = url;
            newURL["paused"] = diag.startPaused();
            if(const auto& additionalData = diag.additionalData(); additionalData.size()) {
                newURL["additional_data"] = additionalData;
            }
            arr.append(newURL);
        }
        if(arr.size()) {
            connection->addOrUpdateURLs(arr);
            urlModel->refresh();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}

void MainWindow::showEvent(QShowEvent*)
{
    if(connection) {
        urlModel->refresh();
        subModel->refresh();
        logModel->refresh();
    }
}

void MainWindow::on_recheckSubsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = subFilterModel->mapToSource(index);
        auto row = subModel->getRowData(index);
        row["last_successful_check"] = QJsonValue::Null;
        row["last_check"] = QJsonValue::Null;
        subModel->setRowData(index, row);
    }
}

void MainWindow::on_pauseSubsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = subFilterModel->mapToSource(index);
        auto row = subModel->getRowData(index);
        row["paused"] = QJsonValue{1};
        subModel->setRowData(index, row);
    }
}

void MainWindow::on_retryURLsButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = urlFilterModel->mapToSource(index);
        auto row = urlModel->getRowData(index);
        row["status"] = QJsonValue{-1};
        urlModel->setRowData(index, row);
    }
}

void MainWindow::on_pauseURLsButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = urlFilterModel->mapToSource(index);
        auto row = urlModel->getRowData(index);
        row["paused"] = QJsonValue{1};
        urlModel->setRowData(index, row);
    }
}

void MainWindow::on_loadSubChecksForAllButton_clicked()
{
    subCheckModel->loadDataForSubscription(0);
}

void MainWindow::on_loadSubChecksForSubButton_clicked()
{
    int id = QInputDialog::getInt(this, "Load check data for subscription", "Subscription ID:", 0, 0);
    subCheckModel->loadDataForSubscription(id);
}

void MainWindow::on_refreshSubChecksButton_clicked()
{
    subCheckModel->refresh();
}

void MainWindow::on_subCheckFilterLineEdit_textEdited(const QString& arg1)
{
    subCheckFilterModel->setFilterRegularExpression(arg1);
}

void MainWindow::on_viewChecksForSubButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) index = subFilterModel->mapToSource(index);
    auto ids = subModel->getIDs(indices);
    if(ids.size() != 1) return;
    subCheckModel->loadDataForSubscription(ids[0]);
    ui->mainTabWidget->setCurrentWidget(ui->subChecksTab);
}

void MainWindow::on_includeArchivedSubChecksCheckBox_toggled(bool checked)
{
    subCheckModel->setShowArchived(checked);
    subCheckModel->refresh();
}

void MainWindow::on_includeArchivedURLsCheckBox_toggled(bool checked)
{
    urlModel->setShowArchived(checked);
    urlModel->refresh();
}

void MainWindow::on_archiveURLsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = subFilterModel->mapToSource(index);
        auto row = subModel->getRowData(index);
        row["archived"] = QJsonValue{1};
        subModel->setRowData(index, row);
    }
}

void MainWindow::on_archiveSubChecksButton_clicked()
{
    auto indices = ui->subCheckTableView->selectionModel()->selectedRows();
    for(auto& index: indices) {
        index = subCheckFilterModel->mapToSource(index);
        auto row = subCheckModel->getRowData(index);
        row["archived"] = QJsonValue{1};
        subCheckModel->setRowData(index, row);
    }
}
