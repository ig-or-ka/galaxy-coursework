#include "mainwindow.h"
#include "math.h"

#include "ui_mainwindow.h"
#include <QPainter>
#include <QTime>
#include <iostream>
#include <sys/time.h>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    setWindowTitle("Модель галактики");
    connect(ui->pushButtonStart,  SIGNAL(clicked()), this, SLOT(buttonText()));
    connect(ui->saveGalaxy,  SIGNAL(clicked()), this, SLOT(saveGalaxy()));
    connect(ui->createGalaxy,  SIGNAL(clicked()), this, SLOT(createGalaxy()));
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    timer->start(1);
}
MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::Stop(bool save){
    if(save){
        std::ofstream file(ui->fileaNameEdit->text().toStdString(),std::ios::out | std::ios::binary);
        (*current_galaxy) >> file;
        file.close();
    }

    delete current_galaxy;
    current_galaxy = nullptr;
    ui->pushButtonStart->setText(textB[0]);
    disconnect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
}

void MainWindow::saveGalaxy(){    
    Stop(true);
}

void MainWindow::createGalaxy(){
    if(create_window == nullptr && current_galaxy == nullptr){
        create_window = new CreateGalaxyWindow(this);
        create_window->show();
    }
}

void MainWindow::Start(){
    ui->pushButtonStart->setText(textB[1]);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
}

void MainWindow::buttonText(){
    if(ui->pushButtonStart->text()==textB[0]){
        current_galaxy = new Galaxy();
        std::ifstream file(ui->fileaNameEdit->text().toStdString(),std::ios::in | std::ios::binary);
        (*current_galaxy) << file;
        file.close();

        Start();
    }
    else{        
        Stop(false);
    }
    return;
}

void MainWindow::paintEvent(QPaintEvent *e) {    
  Q_UNUSED(e);

    if(current_galaxy != nullptr){
        Galaxy* gl = current_galaxy;

        QPainter painter(this);
        QPen pen(Qt::black, 1, Qt::SolidLine);
        painter.setPen(pen);

        QBrush brush_sectors(Qt::black, Qt::CrossPattern);
        painter.setBrush(brush_sectors);
        for(int i = 0; i < gl->length / gl->sun_radius + 1; i++){
            painter.drawLine(topX0,topY0+gl->sun_radius*i,topX0+gl->length,topY0+gl->sun_radius*i);
        }

        for(int i = 0; i < gl->length / gl->sun_radius + 1; i++){
            painter.drawLine(topX0+gl->sun_radius*i,topY0,topX0+gl->sun_radius*i,topY0+gl->length);
        }

        QBrush brush;//(Qt::yellow);
        brush.setStyle(Qt::SolidPattern);

        for(int i1 = 0; i1 < gl->sectors_count; i1++){
            for(int j1 = 0; j1 < gl->sectors_count; j1++){
                Sector* sector = current_galaxy->sectors[i1][j1];

                for(int i = 1; i < sector->max_star_index; i++){
                    Star* star_i = sector->stars[i];

                    if(star_i){
                        brush.setColor(star_i->col);
                        painter.setBrush(brush);

                        painter.drawEllipse(star_i->x[0] * gl->coefX + gl->centerX + topX0,
                                            star_i->x[1] * gl->coefX + gl->centerX + topY0,
                                            star_i->radius, star_i->radius);
                    }
                }
            }
        }

        brush.setColor(Qt::yellow);
        painter.setBrush(brush);
        painter.drawEllipse(current_galaxy->sun->x[0] * gl->coefX + gl->centerX + topX0,
                            current_galaxy->sun->x[1] * gl->coefX + gl->centerX + topY0,
                            gl->sun_radius, gl->sun_radius);
        /*struct timeval  tv;
        gettimeofday(&tv, NULL);

        double time_begin = ((double)tv.tv_sec) * 1000 +
                              ((double)tv.tv_usec) / 1000;*/
        current_galaxy->Move();

        /*gettimeofday(&tv, NULL);
        double time_end = ((double)tv.tv_sec) * 1000 +
                            ((double)tv.tv_usec) / 1000 ;

        double total_time_ms = time_end - time_begin;*/

        //std::cout << "Move time: " << total_time_ms << std::endl;
    }
}
