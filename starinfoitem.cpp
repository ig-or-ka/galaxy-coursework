#include "starinfoitem.h"
#include "ui_starinfoitem.h"
#include <QPainter>
#include <iostream>
using namespace std;

StarInfoItem::StarInfoItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StarInfoItem)
{
    ui->setupUi(this);

    connect(timer, &QTimer::timeout, this, QOverload<>::of(&StarInfoItem::update));
    timer->start(1);
}

void StarInfoItem::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);

    if(star){
        QPainter painter(this);
        QPen pen(Qt::black, 1, Qt::SolidLine);
        painter.setPen(pen);

        QBrush brush(star->col);
        brush.setStyle(Qt::SolidPattern);
        painter.setBrush(brush);
        painter.drawEllipse(0,0,30,30);

        ui->starMass->setText(QString::number(star->m));
    }
}

StarInfoItem::~StarInfoItem()
{
    delete ui;
}
