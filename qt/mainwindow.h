#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void on_lblSettings_linkActivated(const QString &);
    void on_pbConnect_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
