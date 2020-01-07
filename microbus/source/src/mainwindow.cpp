/*

  Copyright (c) 2015, 2016 Hubert Denkmair <hubert@denkmair.de>

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QMdiArea>
#include <QSignalMapper>
#include <QCloseEvent>
#include <QDomDocument>

#include <core/MeasurementSetup.h>
#include <core/CanTrace.h>
#include <window/TraceWindow/TraceWindow.h>
#include <window/SetupDialog/SetupDialog.h>
#include <window/LogWindow/LogWindow.h>
#include <window/CanStatusWindow/CanStatusWindow.h>
#include <window/RawTxWindow/RawTxWindow.h>

#if defined(__linux__)
#include <driver/SocketCanDriver/SocketCanDriver.h>
#else
#include <driver/CandleApiDriver/CandleApiDriver.h>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(showSetupDialog()));
    connect(ui->actionStart_Measurement, SIGNAL(triggered()), this, SLOT(startMeasurement()));
    connect(ui->actionStop_Measurement, SIGNAL(triggered()), this, SLOT(stopMeasurement()));

    connect(&backend(), SIGNAL(beginMeasurement()), this, SLOT(updateMeasurementActions()));
    connect(&backend(), SIGNAL(endMeasurement()), this, SLOT(updateMeasurementActions()));
    updateMeasurementActions();

    connect(ui->actionSave_Trace_to_file, SIGNAL(triggered(bool)), this, SLOT(saveTraceToFile()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));

#if defined(__linux__)
    Backend::instance().addCanDriver(*(new SocketCanDriver(Backend::instance())));
#else
    Backend::instance().addCanDriver(*(new CandleApiDriver(Backend::instance())));
#endif

    stopAndClearMeasurement();
    createTraceWindow();
    addStatusWidget();
    addLogWidget();
    addRawTxWidget();
    backend().setDefaultSetup();

    _setupDlg = new SetupDialog(Backend::instance(), 0); // NOTE: must be called after drivers/plugins are initialized
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateMeasurementActions()
{
    bool running = backend().isMeasurementRunning();
    ui->actionStart_Measurement->setEnabled(!running);
    ui->actionStop_Measurement->setEnabled(running);
}

void MainWindow::closeEvent(QCloseEvent *event){
    backend().stopMeasurement();
    event->accept();
}

Backend &MainWindow::backend()
{
    return Backend::instance();
}

QMainWindow *MainWindow::createTab(QString title)
{
    QMainWindow *mm = new QMainWindow(this);
    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(0xeb, 0xeb, 0xeb));
    mm->setAutoFillBackground(true);
    mm->setPalette(pal);
    ui->mainTabs->addTab(mm, title);
    return mm;
}

QMainWindow *MainWindow::currentTab()
{
    return (QMainWindow*)ui->mainTabs->currentWidget();
}

void MainWindow::stopAndClearMeasurement()
{
    backend().stopMeasurement();
    QCoreApplication::processEvents();
    backend().clearTrace();
    backend().clearLog();
}

QMainWindow *MainWindow::createTraceWindow(QString title)
{
    if (title.isNull()) {
        title = "Trace";
    }

    QMainWindow *mm = createTab(title);
    mm->setCentralWidget(new TraceWindow(mm, backend()));
    ui->mainTabs->setCurrentWidget(mm);

    return mm;
}

void MainWindow::addRawTxWidget(QMainWindow *parent)
{
    if (!parent) {
        parent = currentTab();
    }

    QDockWidget *dock = new QDockWidget("Transmit", parent);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->setWidget(new RawTxWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::addLogWidget(QMainWindow *parent)
{
    if (!parent) {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget("Log", parent);
    ui->menuView->addAction(dock->toggleViewAction());
    LogWindow *windo = new LogWindow(dock, backend());
    dock->setWidget(windo);
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::addStatusWidget(QMainWindow *parent)
{
    if (!parent) {
        parent = currentTab();
    }

    QDockWidget *dock = new QDockWidget("CAN Status", parent);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->setWidget(new CanStatusWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
}

bool MainWindow::showSetupDialog()
{
    MeasurementSetup new_setup(&backend());
    new_setup.cloneFrom(backend().getSetup());

    if (_setupDlg->showSetupDialog(new_setup)) {
        backend().setSetup(new_setup);
        return true;
    } else {
        return false;
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this,
       "About microbus",
       "microbus\n"
       "easy use can bus analyzer\n"
       "version 0.2.1\n"
       "https://codenocold.github.io/microbus/\n"
    );
}

void MainWindow::startMeasurement()
{
    if (showSetupDialog()) {
        backend().clearTrace();
        backend().startMeasurement();
    }
}

void MainWindow::stopMeasurement()
{
    backend().stopMeasurement();
}

void MainWindow::saveTraceToFile()
{
    QString filters("Vector ASC (*.asc);;Linux candump (*.candump))");
    QString defaultFilter("Vector ASC (*.asc)");

    QFileDialog fileDialog(0, "Save Trace to file", QDir::currentPath(), filters);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setConfirmOverwrite(true);
    fileDialog.selectNameFilter(defaultFilter);
    fileDialog.setDefaultSuffix("asc");
    if (fileDialog.exec()) {
        QString filename = fileDialog.selectedFiles()[0];
        QFile file(filename);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {

            if (filename.endsWith(".candump", Qt::CaseInsensitive)) {
                backend().getTrace()->saveCanDump(file);
            } else {
                backend().getTrace()->saveVectorAsc(file);
            }

            file.close();
        } else {
            // TODO error message
        }
    }
}


void MainWindow::on_action_TraceClear_triggered()
{
    backend().clearTrace();
    backend().clearLog();
}
