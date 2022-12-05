#ifndef STARSINFOWINDOW_H
#define STARSINFOWINDOW_H

#include <QDialog>
#include <vector>
#include "starinfoitem.h"

namespace Ui {
class StarsInfoWindow;
}

class StarsInfoWindow : public QDialog
{
    Q_OBJECT

public:
    std::vector<StarInfoItem*> items_index;
    StarInfoItem* sun_item;
    explicit StarsInfoWindow(QWidget *parent = nullptr);
    ~StarsInfoWindow();
    void AddItems(int count);
    void SetParams(Galaxy* galaxy);

private:
    Ui::StarsInfoWindow *ui;
};

#endif // STARSINFOWINDOW_H
