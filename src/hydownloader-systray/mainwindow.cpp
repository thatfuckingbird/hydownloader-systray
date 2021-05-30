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
#include <QJsonDocument>
#include <QMessageBox>
#include <QShortcut>
#include <QInputDialog>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QToolButton>
#include <QActionGroup>
#include <QFileInfo>
#include <QDir>
#include "jsonobjectdelegate.h"
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

    menuButton = new QToolButton{this};
    menuButton->setText("Menu");
    ui->mainTabWidget->setCornerWidget(menuButton, Qt::TopRightCorner);

    const bool shouldBeVisible = startVisible || settings->value("startVisible").toBool();

    QMenu* manageMenu = new QMenu{this};
    manageMenu->setTitle("Management actions");
    pauseSubsAction = manageMenu->addAction("Pause subscriptions");
    connect(pauseSubsAction, &QAction::triggered, [&] {
        currentConnection->pauseSubscriptions();
    });
    pauseSingleURLsAction = manageMenu->addAction("Pause single URL downloads");
    connect(pauseSingleURLsAction, &QAction::triggered, [&] {
        currentConnection->pauseSingleURLQueue();
    });
    manageMenu->addSeparator();
    pauseAllAction = manageMenu->addAction("Pause all");
    connect(pauseAllAction, &QAction::triggered, [&] {
        currentConnection->pauseSingleURLQueue();
        currentConnection->pauseSubscriptions();
    });
    resumeAllAction = manageMenu->addAction("Resume all");
    connect(resumeAllAction, &QAction::triggered, [&] {
        currentConnection->resumeSingleURLQueue();
        currentConnection->resumeSubscriptions();
    });
    manageMenu->addSeparator();
    abortSubAction = manageMenu->addAction("Force abort running subscription");
    connect(abortSubAction, &QAction::triggered, [&] {
        currentConnection->stopCurrentSubscription();
    });
    abortURLAction = manageMenu->addAction("Force abort running single URL download");
    connect(abortURLAction, &QAction::triggered, [&] {
        currentConnection->stopCurrentURL();
    });
    manageMenu->addSeparator();
    runTestsAction = manageMenu->addAction("Run download tests...");
    connect(runTestsAction, &QAction::triggered, [&] {
        bool ok = true;
        QStringList tests = QInputDialog::getText(this, "Run tests", "Tests:", QLineEdit::Normal, settings->value("defaultTests").toString(), &ok).split(",");
        for(auto& str: tests) str = str.trimmed();
        if(ok && !tests.isEmpty()) currentConnection->runTests(tests);
    });
    runReportAction = manageMenu->addAction("Generate report...");
    connect(runReportAction, &QAction::triggered, [&] {
        bool verbose = QMessageBox::question(this, "Report", "Run verbose report?") == QMessageBox::Yes;
        currentConnection->runReport(verbose);
    });
    manageMenu->addSeparator();
    shutdownAction = manageMenu->addAction("Shut down hydownloader");
    connect(shutdownAction, &QAction::triggered, [&] {
        currentConnection->shutdown();
    });

    QMenu* mainMenu = new QMenu{this};
    downloadURLAction = mainMenu->addAction("Download URL");
    connect(downloadURLAction, &QAction::triggered, [&] {
        launchAddURLsDialog(false);
    });

    mainMenu->addSeparator();
    subscriptionsAction = mainMenu->addAction("Manage subscriptions");
    connect(subscriptionsAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->subsTab);
        if(settings->value("aggressiveUpdates").toBool()) subModel->refresh(false);
        show();
        raise();
    });
    singleURLQueueAction = mainMenu->addAction("Manage single URL queue");
    connect(singleURLQueueAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->singleURLsTab);
        if(settings->value("aggressiveUpdates").toBool()) urlModel->refresh(false);
        show();
        raise();
    });
    checksAction = mainMenu->addAction("Review subscription checks");
    connect(checksAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->subChecksTab);
        if(settings->value("aggressiveUpdates").toBool()) subCheckModel->refresh(false);
        show();
        raise();
    });
    logsAction = mainMenu->addAction("Review logs");
    connect(logsAction, &QAction::triggered, [&] {
        ui->mainTabWidget->setCurrentWidget(ui->logsTab);
        if(settings->value("aggressiveUpdates").toBool()) logModel->refresh();
        show();
        raise();
    });
    mainMenu->addSeparator();
    mainMenu->addMenu(manageMenu);
    mainMenu->addSeparator();
    quitAction = mainMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

    trayIcon->setContextMenu(mainMenu);
    setIcon(QIcon{drawSystrayIcon({Qt::gray})});
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, [&] {
        if(isVisible()) {
            if(isMinimized()) {
                setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
                activateWindow();
                raise();
            } else {
                hide();
            }
        } else {
            show();
            activateWindow();
            raise();
        }
    });
    menuButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    menuButton->setMenu(mainMenu);
    menuButton->setPopupMode(QToolButton::InstantPopup);

    statusUpdateTimer = new QTimer{this};

    instanceNames = settings->value("instanceNames").toStringList();
    auto accessKeys = settings->value("accessKey").toStringList();
    auto apiURLs = settings->value("apiURL").toStringList();
    bool err = instanceNames.isEmpty();
    for(int i = 0; i < instanceNames.size(); ++i) {
        auto connection = new HyDownloaderConnection{this};
        connection->setCertificateVerificationEnabled(false);
        connection->setEnabled(false);
        connections[instanceNames[i]] = connection;

        connect(connection, &HyDownloaderConnection::networkError, [&](std::uint64_t, int status, QNetworkReply::NetworkError, const QString& errorText) {
            lastUpdateTime = QTime::currentTime();
            if(status == 404) return; //expected, if requesting logs that were not created yet
            setStatusText(HyDownloaderLogModel::LogLevel::Error, QString{"Network error (status %1): %2"}.arg(QString::number(status), errorText));
            setIcon(QIcon{drawSystrayIcon({Qt::red})});
        });
        connect(connection, &HyDownloaderConnection::sslErrors, [&](QNetworkReply*, const QList<QSslError>& errors) {
            QString errorString;
            for(const auto& error: errors) {
                errorString += error.errorString() + "\n";
            }
            setStatusText(HyDownloaderLogModel::LogLevel::Error, QString{"SSL errors:\n%1"}.arg(errorString.trimmed()));
            setIcon(QIcon{drawSystrayIcon({Qt::red})});
            lastUpdateTime = QTime::currentTime();
        });
        connect(connection, &HyDownloaderConnection::statusInformationReceived, [&](std::uint64_t, const QJsonObject& info) {
            setIcon(QIcon{drawSystrayIcon({statusToColor(info["subscription_worker_status"].toString()),
                                           queueSizeToColor(info["subscriptions_due"].toInt()),
                                           statusToColor(info["url_worker_status"].toString()),
                                           queueSizeToColor(info["urls_queued"].toInt())})});
            setStatusText(HyDownloaderLogModel::LogLevel::Info, QString{"Subscriptions due: %1, URLs queued: %2\nSubscription worker status: %3\nURL worker status: %4"}.arg(
                                                                  QString::number(info["subscriptions_due"].toInt()),
                                                                  QString::number(info["urls_queued"].toInt()),
                                                                  info["subscription_worker_status"].toString(),
                                                                  info["url_worker_status"].toString()));
            lastUpdateTime = QTime::currentTime();
        });
        connect(connection, &HyDownloaderConnection::replyReceived, [&](std::uint64_t requestID, const QJsonDocument& data) {
            bool openFolder = false;
            if(viewFolderSubReqs.contains(requestID)) {
                viewFolderSubReqs.remove(requestID);
                openFolder = true;
            }
            if(viewFolderURLReqs.contains(requestID)) {
                viewFolderURLReqs.remove(requestID);
                openFolder = true;
            }
            if(openFolder) {
                QString finalDir;
                const QJsonArray arr = data.array()[0].toObject()["paths"].toArray();
                for(const auto& path: arr) {
                    QDir dir = QFileInfo{path.toString()}.absoluteDir();
                    if(dir.exists()) {
                        finalDir = dir.absolutePath();
                    }
                }
                if(finalDir.isEmpty()) {
                    QMessageBox::warning(this, "Not found", "Could not identify folder!");
                } else {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(finalDir));
                }
            }
        });

        connect(statusUpdateTimer, &QTimer::timeout, connection, &HyDownloaderConnection::requestStatusInformation);

        if(i < apiURLs.size()) {
            connection->setAPIURL(apiURLs[i]);
        } else {
            err = true;
        }
        if(i < accessKeys.size()) {
            connection->setAccessKey(accessKeys[i]);
        } else {
            err = true;
        }
    }
    if(err) {
        QMessageBox::critical(this, "Invalid instance configuration", "The number of instance names, API URLs and access keys in the configuration must be the same and you must have at least 1 instance configured!");
        QApplication::instance()->quit();
    }
    if(instanceNames.size() > 1) {
        instanceSwitchMenu = new QMenu{this};
        instanceSwitchMenu->setTitle("Active instance");
        instanceSwitchActionGroup = new QActionGroup{this};
        instanceSwitchActionGroup->setExclusive(true);
        for(const auto& instance: instanceNames) {
            instanceSwitchActionGroup->addAction(instanceSwitchMenu->addAction(QString{"Switch to instance: %1"}.arg(instance), [&, instance] {
                                         setCurrentConnection(instance);
                                     }))
              ->setCheckable(true);
        }
        instanceSwitchActionGroup->actions()[0]->setChecked(true);
        QAction* before = mainMenu->actions()[0];
        mainMenu->insertMenu(before, instanceSwitchMenu);
        mainMenu->insertSeparator(before);
    } else {
        ui->instanceLabel->setVisible(false);
    }
    logModel = new HyDownloaderLogModel{};
    subModel = new HyDownloaderSubscriptionModel{};
    subCheckModel = new HyDownloaderSubscriptionChecksModel{};
    urlModel = new HyDownloaderSingleURLQueueModel{};
    if(settings->value("aggressiveUpdates").toBool()) {
        connect(statusUpdateTimer, &QTimer::timeout, [&] {
            urlModel->refresh(false);
            subModel->refresh(false);
            subCheckModel->refresh(false);
        });
    }

    logFilterModel = new QSortFilterProxyModel{};
    logFilterModel->setSourceModel(logModel);
    logFilterModel->setFilterKeyColumn(-1);
    ui->logTableView->setModel(logFilterModel);
    ui->logTableView->setItemDelegate(new JSONObjectDelegate{});
    connect(logModel, &QAbstractTableModel::rowsInserted, [&] {
        ui->logTableView->scrollToBottom();
    });

    urlFilterModel = new QSortFilterProxyModel{};
    urlFilterModel->setSourceModel(urlModel);
    urlFilterModel->setFilterKeyColumn(-1);
    ui->urlsTableView->setModel(urlFilterModel);
    ui->urlsTableView->setItemDelegate(new JSONObjectDelegate{});

    subFilterModel = new QSortFilterProxyModel{};
    subFilterModel->setSourceModel(subModel);
    subFilterModel->setFilterKeyColumn(-1);
    ui->subTableView->setModel(subFilterModel);
    auto subDelegate = new JSONObjectDelegate{};
    subDelegate->setItemListForColumn(1, settings->value("defaultDownloaders").toStringList());
    ui->subTableView->setItemDelegate(subDelegate);

    subCheckFilterModel = new QSortFilterProxyModel{};
    subCheckFilterModel->setSourceModel(subCheckModel);
    subCheckFilterModel->setFilterKeyColumn(-1);
    ui->subCheckTableView->setModel(subCheckFilterModel);
    ui->subCheckTableView->setItemDelegate(new JSONObjectDelegate{});

    connect(logModel, &HyDownloaderLogModel::statusTextChanged, [&](const QString& statusText) {
        ui->currentLogLabel->setText(statusText);
    });

    connect(subCheckModel, &HyDownloaderSubscriptionChecksModel::statusTextChanged, [&](const QString& statusText) {
        ui->currentSubChecksLabel->setText(statusText);
    });

    setCurrentConnection(instanceNames[0]);

    statusUpdateTimer->setInterval(settings->value("updateInterval").toInt());
    statusUpdateTimer->start();

    statusUpdateIntervalTimer = new QTimer{this};
    connect(statusUpdateIntervalTimer, &QTimer::timeout, [&] {
        if(lastUpdateTime.secsTo(QTime::currentTime()) >= 30) {
            setStatusText(HyDownloaderLogModel::LogLevel::Error, QString{"No status update received in the past 30 seconds"});
            setIcon(QIcon{drawSystrayIcon({Qt::red})});
        }
    });
    lastUpdateTime = QTime::currentTime();
    statusUpdateIntervalTimer->setInterval(30000);
    statusUpdateIntervalTimer->start();

    ui->mainTabWidget->setCurrentIndex(0);

    connect(ui->subTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateSubCountInfoAndButtons);
    connect(ui->urlsTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateURLCountInfoAndButtons);
    connect(ui->subCheckTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateSubCheckCountInfoAndButtons);
    connect(ui->logTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateLogCountInfo);

    connect(logModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateLogCountInfo);
    connect(logModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateLogCountInfo);
    connect(logModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateLogCountInfo);

    connect(subModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateSubCountInfoAndButtons);
    connect(subModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateSubCountInfoAndButtons);
    connect(subModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateSubCountInfoAndButtons);

    connect(urlModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateURLCountInfoAndButtons);
    connect(urlModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateURLCountInfoAndButtons);
    connect(urlModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateURLCountInfoAndButtons);

    connect(subCheckModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateSubCheckCountInfoAndButtons);
    connect(subCheckModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateSubCheckCountInfoAndButtons);
    connect(subCheckModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateSubCheckCountInfoAndButtons);

    QMenu* pauseSubsMenu = new QMenu{this};
    resumeSelectedSubsAction = pauseSubsMenu->addAction("Resume", [&] {
        auto indices = ui->subTableView->selectionModel()->selectedRows();
        QJsonArray rowData;
        for(auto& index: indices) {
            index = subFilterModel->mapToSource(index);
            auto row = subModel->getBasicRowData(index);
            row["paused"] = QJsonValue{0};
            rowData.append(row);
        }
        subModel->updateRowData(indices, rowData);
    });
    ui->pauseSubsButton->setMenu(pauseSubsMenu);

    QMenu* pauseURLsMenu = new QMenu{this};
    resumeSelectedURLsAction = pauseURLsMenu->addAction("Resume", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        QJsonArray rowData;
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getBasicRowData(index);
            row["paused"] = QJsonValue{0};
            rowData.append(row);
        }
        urlModel->updateRowData(indices, rowData);
    });
    ui->pauseURLsButton->setMenu(pauseURLsMenu);

    QMenu* archiveURLsMenu = new QMenu{this};
    unarchiveSelectedURLsAction = archiveURLsMenu->addAction("Unarchive", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        QJsonArray rowData;
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getBasicRowData(index);
            row["archived"] = QJsonValue{0};
            rowData.append(row);
        }
        urlModel->updateRowData(indices, rowData);
    });
    ui->archiveURLsButton->setMenu(archiveURLsMenu);

    QMenu* archiveSubChecksMenu = new QMenu{this};
    unarchiveSelectedSubChecksAction = archiveSubChecksMenu->addAction("Unarchive", [&] {
        auto indices = ui->subCheckTableView->selectionModel()->selectedRows();
        QJsonArray rowData;
        for(auto& index: indices) {
            index = subCheckFilterModel->mapToSource(index);
            auto row = subCheckModel->getBasicRowData(index);
            row["archived"] = QJsonValue{0};
            rowData.append(row);
        }
        subCheckModel->updateRowData(indices, rowData);
    });
    ui->archiveSubChecksButton->setMenu(archiveSubChecksMenu);

    QMenu* retryURLsMenu = new QMenu{this};
    retryAndForceOverwriteURLAction = retryURLsMenu->addAction("Retry and force overwrite", [&] {
        auto indices = ui->urlsTableView->selectionModel()->selectedRows();
        QJsonArray rowData;
        for(auto& index: indices) {
            index = urlFilterModel->mapToSource(index);
            auto row = urlModel->getBasicRowData(index);
            row["status"] = QJsonValue{-1};
            row["overwrite_existing"] = QJsonValue{1};
            rowData.append(row);
        }
        urlModel->updateRowData(indices, rowData);
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
            auto subID = subModel->getIDs({subFilterModel->mapToSource(ui->subTableView->selectionModel()->selectedRows()[0])})[0];
            if(settings->value("localConnection").toBool()) {
                popup.addAction("Open folder", [&, subID] {
                    viewFolderSubReqs.insert(currentConnection->requestLastFilesForSubscriptions({subID}));
                });
                popup.addSeparator();
            }
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
            auto urlID = urlModel->getIDs({urlFilterModel->mapToSource(ui->urlsTableView->selectionModel()->selectedRows()[0])})[0];
            if(settings->value("localConnection").toBool()) {
                popup.addAction("Open folder", [&, urlID] {
                    viewFolderURLReqs.insert(currentConnection->requestLastFilesForURLs({urlID}));
                });
                popup.addSeparator();
            }
        }
        popup.addAction("Retry", ui->retryURLsButton, &QToolButton::click);
        popup.addAction("Retry and force overwrite", retryAndForceOverwriteURLAction, &QAction::trigger);
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

    setStatusText(HyDownloaderLogModel::LogLevel::Info, "hydownloader-systray started");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateSubCountInfoAndButtons()
{
    const int selectionSize = ui->subTableView->selectionModel()->selectedRows().size();
    ui->recheckSubsButton->setEnabled(selectionSize > 0);
    ui->deleteSelectedSubsButton->setEnabled(selectionSize > 0);
    ui->viewLogForSubButton->setEnabled(selectionSize == 1);
    ui->viewChecksForSubButton->setEnabled(selectionSize == 1);
    ui->pauseSubsButton->setEnabled(selectionSize > 0);
    ui->subsLabel->setText(QString{"%1 selected, %2 total loaded subscriptions"}.arg(
      QLocale{}.toString(selectionSize),
      QLocale{}.toString(subModel->rowCount({}))));
}

void MainWindow::updateURLCountInfoAndButtons()
{
    const int selectionSize = ui->urlsTableView->selectionModel()->selectedRows().size();
    ui->pauseURLsButton->setEnabled(selectionSize > 0);
    ui->archiveURLsButton->setEnabled(selectionSize > 0);
    ui->deleteSelectedURLsButton->setEnabled(selectionSize > 0);
    ui->viewLogForURLButton->setEnabled(selectionSize == 1);
    ui->retryURLsButton->setEnabled(selectionSize > 0);
    ui->urlsLabel->setText(QString{"%1 selected, %2 total loaded URLs"}.arg(
      QLocale{}.toString(selectionSize),
      QLocale{}.toString(urlModel->rowCount({}))));
}

void MainWindow::updateSubCheckCountInfoAndButtons()
{
    const int selectionSize = ui->subCheckTableView->selectionModel()->selectedRows().size();
    ui->archiveSubChecksButton->setEnabled(selectionSize > 0);
    ui->subChecksLabel->setText(QString{"%1 selected, %2 total loaded subscription checks"}.arg(
      QLocale{}.toString(selectionSize),
      QLocale{}.toString(subCheckModel->rowCount({}))));
}

void MainWindow::updateLogCountInfo()
{
    ui->logSelectionLabel->setText(QString{"%1 selected, %2 total loaded log entries"}.arg(
      QLocale{}.toString(ui->logTableView->selectionModel()->selectedRows().size()),
      QLocale{}.toString(logModel->rowCount({}))));
}

void MainWindow::setCurrentConnection(const QString& id)
{
    if(currentConnection) {
        currentConnection->setEnabled(false);
    }
    ui->instanceLabel->setText(id);
    currentConnection = connections[id];
    currentConnection->setEnabled(true);
    logModel->setConnection(currentConnection);
    subModel->setConnection(currentConnection);
    subModel->refresh();
    subCheckModel->setConnection(currentConnection);
    urlModel->setConnection(currentConnection);
    urlModel->refresh();
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
    if(id > 0) logModel->loadSubscriptionLog(id);
}

void MainWindow::on_loadSingleURLQueueLogButton_clicked()
{
    int id = QInputDialog::getInt(this, "Load single URL log", "URL ID:", 0, 0);
    if(id > 0) logModel->loadSingleURLQueueLog(id);
}

void MainWindow::on_loadDaemonLogButton_clicked()
{
    logModel->loadDaemonLog();
}

void MainWindow::on_copyLogToClipboardButton_clicked()
{
    auto rows = ui->logTableView->selectionModel()->selectedRows();
    for(auto& index : rows) index = logFilterModel->mapToSource(index);
    logModel->copyToClipboard(rows);
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
        currentConnection->deleteSubscriptions(ids);
        subModel->refresh();
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
    currentConnection->addOrUpdateSubscriptions(arr);
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
        currentConnection->deleteURLs(ids);
        urlModel->refresh();
    }
}

void MainWindow::on_addURLButton_clicked()
{
    launchAddURLsDialog(true);
}

void MainWindow::setStatusText(HyDownloaderLogModel::LogLevel level, const QString& text)
{
    trayIcon->setToolTip(text);
    ui->statusBar->showMessage(text);
    ui->statusBar->setToolTip(text);
    logModel->addStatusLogLine(level, text);
}

Qt::GlobalColor MainWindow::statusToColor(const QString& statusText)
{
    QString status = statusText.left(statusText.indexOf(':'));
    if(status == "paused") return Qt::yellow;
    if(status == "no information" || status == "shut down") return Qt::gray;
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
            currentConnection->addOrUpdateURLs(arr);
            urlModel->refresh(false);
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
    if(currentConnection && settings->value("aggressiveUpdates").toBool()) {
        urlModel->refresh(false);
        subModel->refresh(false);
        subCheckModel->refresh(false);
        logModel->refresh();
    }
}

void MainWindow::on_recheckSubsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = subFilterModel->mapToSource(index);
        auto row = subModel->getBasicRowData(index);
        row["last_successful_check"] = QJsonValue::Null;
        row["last_check"] = QJsonValue::Null;
        rowData.append(row);
    }
    subModel->updateRowData(indices, rowData);
}

void MainWindow::on_pauseSubsButton_clicked()
{
    auto indices = ui->subTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = subFilterModel->mapToSource(index);
        auto row = subModel->getBasicRowData(index);
        row["paused"] = QJsonValue{1};
        rowData.append(row);
    }
    subModel->updateRowData(indices, rowData);
}

void MainWindow::on_retryURLsButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = urlFilterModel->mapToSource(index);
        auto row = urlModel->getBasicRowData(index);
        row["status"] = QJsonValue{-1};
        rowData.append(row);
    }
    urlModel->updateRowData(indices, rowData);
}

void MainWindow::on_pauseURLsButton_clicked()
{
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = urlFilterModel->mapToSource(index);
        auto row = urlModel->getBasicRowData(index);
        row["paused"] = QJsonValue{1};
        rowData.append(row);
    }
    urlModel->updateRowData(indices, rowData);
}

void MainWindow::on_loadSubChecksForAllButton_clicked()
{
    subCheckModel->loadDataForSubscription(0);
}

void MainWindow::on_loadSubChecksForSubButton_clicked()
{
    int id = QInputDialog::getInt(this, "Load check history for subscription", "Subscription ID:", 0, 0);
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
    auto indices = ui->urlsTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = urlFilterModel->mapToSource(index);
        auto row = urlModel->getBasicRowData(index);
        row["archived"] = QJsonValue{1};
        rowData.append(row);
    }
    urlModel->updateRowData(indices, rowData);
}

void MainWindow::on_archiveSubChecksButton_clicked()
{
    auto indices = ui->subCheckTableView->selectionModel()->selectedRows();
    QJsonArray rowData;
    for(auto& index: indices) {
        index = subCheckFilterModel->mapToSource(index);
        auto row = subCheckModel->getBasicRowData(index);
        row["archived"] = QJsonValue{1};
        rowData.append(row);
    }
    subCheckModel->updateRowData(indices, rowData);
}

void MainWindow::on_loadStatusHistoryButton_clicked()
{
    logModel->loadStatusLog();
}

void MainWindow::on_onlyLatestCheckBox_stateChanged(int arg1)
{
    logModel->setShowOnlyLatest(arg1);
}
