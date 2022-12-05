#include "starsinfowindow.h"
#include "mainwindow.h"
#include "math.h"

#include "ui_mainwindow.h"
#include <QPainter>
#include <QTime>
#include <iostream>
#include <sys/time.h>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    connect(ui->pushButtonStart,  SIGNAL(clicked()), this, SLOT(buttonText()));
    connect(ui->saveGalaxy,  SIGNAL(clicked()), this, SLOT(saveGalaxy()));
    connect(ui->createGalaxy,  SIGNAL(clicked()), this, SLOT(createGalaxy()));
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    timer->start(1);

    info_window = new StarsInfoWindow(this);
    info_window->show();
}
MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::Stop(bool save){
    current_galaxy->Stop();
    info_window->sun_item->star = nullptr;
    for(auto item : info_window->items_index){
        delete  item;
    }
    info_window->items_index.clear();

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
    info_window->sun_item->star = current_galaxy->sun;
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
//  QTime time = QTime::currentTime();
//  int mSec = time.msec();
//  int Sec = time.second();
//  system radius

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

        info_window->SetParams(current_galaxy);

        if(current_galaxy->top_mass_stars.size() < info_window->items_index.size()){
            int count_remove = info_window->items_index.size() - current_galaxy->top_mass_stars.size();
            for(int i = 0; i < count_remove; i++){
                delete info_window->items_index.back();
                info_window->items_index.pop_back();
            }
        }
        else if(info_window->items_index.size() < STARS_TOP_COUNT){
            size_t count_add;
            if(current_galaxy->top_mass_stars.size() < STARS_TOP_COUNT){
                count_add = current_galaxy->top_mass_stars.size() - info_window->items_index.size();
            }
            else{
                count_add = STARS_TOP_COUNT - info_window->items_index.size();
            }
            info_window->AddItems(count_add);
        }

        int star_point = 0;
        int count_out = current_galaxy->top_mass_stars.size();
        if(current_galaxy->top_mass_stars.size() >= STARS_TOP_COUNT){
            star_point = current_galaxy->top_mass_stars.size() - STARS_TOP_COUNT;
            count_out = STARS_TOP_COUNT;
        }
        for(int i = 0; i < count_out; i++){
            info_window->items_index[count_out-i-1]->star = current_galaxy->top_mass_stars[i+star_point];
        }
    }
}
