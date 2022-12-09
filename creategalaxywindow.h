#ifndef CREATEGALAXYWINDOW_H
#define CREATEGALAXYWINDOW_H

#include <QDialog>

namespace Ui {
class CreateGalaxyWindow;
}

class CreateGalaxyWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CreateGalaxyWindow(QWidget *parent = nullptr);
    ~CreateGalaxyWindow();

private:
    Ui::CreateGalaxyWindow *ui;

private slots:
    void create();
};

#endif // CREATEGALAXYWINDOW_H
