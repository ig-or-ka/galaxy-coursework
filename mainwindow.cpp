#include "mainwindow.h"
#include "math.h"

#include "ui_mainwindow.h"
#include <QPainter>
#include <QTime>
#include <sys/time.h>

bool load_from_file = false;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    connect(ui->pushButtonStart,  SIGNAL(clicked()), this, SLOT(buttonText()));
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    timer->start(1);
}
MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::buttonText(){
    if(ui->pushButtonStart->text()==textB[0]){
        current_galaxy = new Galaxy(numStars,threadPoolSize);
        if(load_from_file){
            std::ifstream file("galaxy.txt",std::ios::in | std::ios::binary);
            (*current_galaxy) << file;
            file.close();
        }
        else{
            current_galaxy->GenerateStars();
        }

        ui->pushButtonStart->setText(textB[1]);
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    }
    else{
        current_galaxy->Stop();
        load_from_file = true;

        std::ofstream file("galaxy.txt",std::ios::out | std::ios::binary);
        (*current_galaxy) >> file;
        file.close();

        delete current_galaxy;
        current_galaxy = nullptr;
        ui->pushButtonStart->setText(textB[0]);
        disconnect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
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
        QPainter painter(this);
        QPen pen(Qt::black, 1, Qt::SolidLine);
        painter.setPen(pen);

        QBrush brush_sectors(Qt::black, Qt::CrossPattern);
        painter.setBrush(brush_sectors);
        for(int i = 0; i < length / sun_radius + 1; i++){
            painter.drawLine(topX0,topY0+sun_radius*i,topX0+length,topY0+sun_radius*i);
        }

        for(int i = 0; i < length / sun_radius + 1; i++){
            painter.drawLine(topX0+sun_radius*i,topY0,topX0+sun_radius*i,topY0+length);
        }

        QBrush brush;//(Qt::yellow);
        brush.setStyle(Qt::SolidPattern);

        for(int i1 = 0; i1 < sectors_count; i1++){
            for(int j1 = 0; j1 < sectors_count; j1++){
                Sector* sector = current_galaxy->sectors[i1][j1];

                for(int i = 1; i < sector->max_star_index; i++){
                    Star* star_i = sector->stars[i];

                    if(star_i){
                        brush.setColor(star_i->col);
                        painter.setBrush(brush);

                        int radius = star_i->Radius();
                        painter.drawEllipse(star_i->x[0] * coefX + centerX + topX0,
                                            star_i->x[1] * coefX + centerX + topY0,
                                            radius, radius);
                    }
                }
            }
        }

        brush.setColor(Qt::yellow);
        painter.setBrush(brush);
        painter.drawEllipse(current_galaxy->sun->x[0] * coefX + centerX + topX0,
                            current_galaxy->sun->x[1] * coefX + centerX + topY0,
                            10, 10);
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

        ui->lineEdit->setText(QString::number(Star::star_counter));
        ui->lineEdit_2->setText(QString::number(current_galaxy->sun->m));
        ui->lineEdit_3->setText(QString::number(current_galaxy->sun->x[0]));
    }
}
