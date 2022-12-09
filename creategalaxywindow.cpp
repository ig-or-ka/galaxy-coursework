#include "creategalaxywindow.h"
#include "ui_creategalaxywindow.h"
#include "mainwindow.h"
#include "star.h"
#include <iostream>

CreateGalaxyWindow::CreateGalaxyWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateGalaxyWindow)
{
    ui->setupUi(this);
    connect(ui->createButton,  SIGNAL(clicked()), this, SLOT(create()));
}

void CreateGalaxyWindow::create(){
    auto mainwindow = (MainWindow*)parent();
    mainwindow->create_window = nullptr;

    int size_sector = ui->sectorSize->text().toInt();
    int size_rect = ui->rectSize->text().toInt();
    double system_radius = ui->systemRadius->text().toDouble();
    int count_stars = ui->countStars->text().toInt();
    int dt = ui->DT->text().toInt();

    mainwindow->current_galaxy = new Galaxy(count_stars,size_sector,size_rect,system_radius,dt);
    mainwindow->current_galaxy->GenerateStars();
    mainwindow->Start();

    delete this;
}

CreateGalaxyWindow::~CreateGalaxyWindow()
{
    delete ui;
}
