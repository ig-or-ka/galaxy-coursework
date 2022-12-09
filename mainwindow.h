#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "star.h"
#include "creategalaxywindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class StarsInfoWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool ellipse = false;
    const QString textB[2] = {"Start", "Stop"};
    QTimer *timer = new QTimer(this);
    CreateGalaxyWindow* create_window = nullptr;
    Galaxy* current_galaxy = nullptr;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void Start();
    void Stop(bool save);

private:
    Ui::MainWindow *ui;

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void buttonText();
    void saveGalaxy();
    void createGalaxy();
};

#endif // MAINWINDOW_H
