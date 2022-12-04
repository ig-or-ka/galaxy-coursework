#include "starsinfowindow.h"
#include "starinfoitem.h"
#include "ui_starsinfowindow.h"
#include <iostream>
#include <QLabel>
#include <QVBoxLayout>

StarsInfoWindow::StarsInfoWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StarsInfoWindow)
{
    ui->setupUi(this);    

    auto layout = new QVBoxLayout(ui->scrollInfoArea);
    ui->scrollInfoArea->setWidgetResizable(true);
    ui->scrollInfoArea->widget()->setLayout(layout);
    ui->scrollInfoArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    sun_item = new StarInfoItem(ui->scrollInfoArea->widget());
    layout->addWidget(sun_item);
}

void StarsInfoWindow::AddItems(int count){
    /*if(count > 0){
        std::cout << "Add item: " << count << std::endl;
    }*/
    for(int i = 0; i < count; i++){
        auto item = new StarInfoItem(ui->scrollInfoArea->widget());
        items_index.push_back(item);
        ui->scrollInfoArea->widget()->layout()->addWidget(item);
    }
}

StarsInfoWindow::~StarsInfoWindow()
{
    delete ui;
}
