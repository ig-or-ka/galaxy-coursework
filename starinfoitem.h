#ifndef STARINFOITEM_H
#define STARINFOITEM_H

#include "star.h"
#include <QWidget>
#include <QTimer>

namespace Ui {
class StarInfoItem;
}

class StarInfoItem : public QWidget
{
    Q_OBJECT

public:
    Star* star = 0;
    explicit StarInfoItem(QWidget *parent = nullptr);
    ~StarInfoItem();

protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::StarInfoItem *ui;
    QTimer *timer = new QTimer(this);
};

#endif // STARINFOITEM_H
