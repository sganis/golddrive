#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <Python.h>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    setWindowTitle("SSH-Drive");
    QString location = QApplication::applicationDirPath() + "/style";
    QDir::setCurrent(location);
    QFile f(location + "/style.css");
    f.open(QFile::ReadOnly);
    qApp->setStyleSheet(f.readAll());
    f.close();
    qApp->setStyle("fusion");
    ui->cboParam->addItems(QString("    S:   Simulation,    I:   IT,    X:   Exploration").split(','));

    ui->lblStatus->setVisible(false);
    ui->progressBar->setVisible(false);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_lblSettings_linkActivated(const QString &/*link*/)
{
    QString path = QApplication::applicationDirPath();
    QFileInfo info(path);
    #if defined(Q_OS_WIN)
        QStringList args;
        if (!info.isDir())
            args << "/select,";
        args << QDir::toNativeSeparators(path);
        if (QProcess::startDetached("explorer", args))
            return;
    #endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir()? path : info.path()));
}

void MainWindow::on_pbConnect_clicked()
{
    PyRun_SimpleStringFlags("app.test()\n", nullptr);
    qDebug() << "app.test() was run";
}
