#include "cmath"
#include <QPainter>
#include <queue>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>

#ifndef STAR_H
#define STAR_H

const int moveThreadPool = 16;

const int topX0 = 100, topY0 = 100, h = 800, length = 800;
const double coefX = length / 2 / 1e12;
const int centerX = length / 2;
const int sun_radius = 10;
const double sector_global_h = sun_radius / coefX;
const int sectors_count = length / sun_radius;

const int dim = 2;
const int numStars = 100;
const int borderMassC = 10;
const double G = 6.67408e-11, systemRadius = 1e12, distConnect = 1e9, dt = 10000;
const double massSun   = 1.98892e30,
             massJup   = 1898.6e24,
             massUran  = 86.832e24,
             massEarth = 5.9742e24,
             massVenus = 4.867e24;
const double borderMass[] = {borderMassC*massEarth, borderMassC*massUran, borderMassC*massJup, borderMassC*massSun};

//const QColor colStar[] = {Qt::cyan, Qt::darkGreen, Qt::magenta, Qt::yellow, Qt::white};
//const int nColor = sizeof(colStar) / sizeof(colStar[0]);

class EventWait{
    std::mutex setTriggerLock;
    std::condition_variable setTrigger;
public:
    void wait();
    void set();
};

template <typename T>
class queue_wait{
    std::mutex push_pop_mutex;
    EventWait pop_wait;
    EventWait empty_wait;
    int count_wait = 0;

public:
    std::queue<T> que;
    void unlock();
    T pop();
    void wait_empty();
};

class star{
public:
    static int starCounter;
    bool removed = false;
    double x[dim];
    double v[dim];
    double m;
    double f[dim];

    QColor col;
    star(double *coord, double *speed, double mass);
    ~star(){starCounter--;}
    int radius();
};

class galaxy;

class Sector{
private:
    void for_check_1(star* star_i, std::set<star*>& used_stars, std::vector<star*>& to_remove);
    void for_check_2(star* star_i, std::set<star*>& used_stars);
    galaxy* gl;

public:
    Sector(galaxy* gl);
    std::set<star*> stars;
    void move();
    ~Sector();
};

class galaxy{
public:
    int num;
    Sector* sectors[sectors_count][sectors_count];
    star* sun;
    queue_wait<Sector*> selectors_queue;
    std::mutex change_sector_requests_mutex;
    std::vector<star*> change_sector_requests;

    galaxy(int n);
    Sector* GetSectorByCoords(double x, double y);
    void CreateSun();
    void CreateSectors();
    ~galaxy();
    void move();
};

#endif // STAR_H
