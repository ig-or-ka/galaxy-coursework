#include "mainwindow.h"
#include "math.h"

#include "ui_mainwindow.h"
#include <QPainter>
#include <QTime>
#include "star.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
//    connect(ui->pushButtonStart,  SIGNAL(clicked()), this, SLOT(buttonText()));
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
//    connect(timer, &QTimer::timeout, this, SLOT(myTimer()));
    timer->start(1);
}
MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::buttonText(){
    if(ui->pushButtonStart->text()==textB[0]){
        ui->pushButtonStart->setText(textB[1]);
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    }else{
        ui->pushButtonStart->setText(textB[0]);
        disconnect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::update));
    }
    return;
}


galaxy *galactika = new galaxy(numStars);

void MainWindow::paintEvent(QPaintEvent *e) {
  Q_UNUSED(e);
  QPainter painter(this);
  QPen pen(Qt::black, 1, Qt::SolidLine);
  painter.setPen(pen);

//  QTime time = QTime::currentTime();
//  int mSec = time.msec();
//  int Sec = time.second();
   // system radius

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
          Sector* sector = galactika->sectors[i1][j1];

          for(auto star : sector->stars){
              brush.setColor(star->col);
              painter.setBrush(brush);

              int radius = star->radius();
              painter.drawEllipse(star->x[0] * coefX + centerX + topX0,
                                  star->x[1] * coefX + centerX + topY0,
                                  radius, radius);
          }
      }
  }

  brush.setColor(Qt::yellow);
  painter.setBrush(brush);
  painter.drawEllipse(galactika->sun->x[0] * coefX + centerX + topX0,
                      galactika->sun->x[1] * coefX + centerX + topY0,
                      10, 10);

  galactika->move();

  ui->lineEdit->setText(QString::number(star::starCounter));
  ui->lineEdit_2->setText(QString::number(galactika->sun->m));
  ui->lineEdit_3->setText(QString::number(galactika->sun->x[0]));
}
