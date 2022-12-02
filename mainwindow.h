#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "star.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool ellipse = false;
    const QString textB[2] = {"Start", "Stop"};
    QTimer *timer = new QTimer(this);

private:
    Ui::MainWindow *ui;
    Galaxy* current_galaxy = nullptr;

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void buttonText();
    void saveGalaxy();
};

#endif // MAINWINDOW_H
