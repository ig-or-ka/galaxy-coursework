#include "cmath"
#include <QPainter>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>

#ifndef STAR_H
#define STAR_H

const int threadPoolSize = 16;
const int starsInSector = 1000;
const int sun_radius = 20;
const int topX0 = 100, topY0 = 100, h = 800, length = 800;

const double coefX = length / 2 / 1e12;
const int centerX = length / 2;
const double sector_global_h = sun_radius / coefX;
const int sectors_count = length / sun_radius;

const int dim = 2;
const int numStars = 10000;
const int borderMassC = 10;
const double G = 6.67408e-11, systemRadius = 1e12, distConnect = 1e9, dt = 30000;
const double massSun   = 1.98892e30,
             massJup   = 1898.6e24,
             massUran  = 86.832e24,
             massEarth = 5.9742e24,
             massVenus = 4.867e24;
const double borderMass[] = {borderMassC*massEarth, borderMassC*massUran, borderMassC*massJup, borderMassC*massSun};

//const QColor colStar[] = {Qt::cyan, Qt::darkGreen, Qt::magenta, Qt::yellow, Qt::white};
//const int nColor = sizeof(colStar) / sizeof(colStar[0]);

template <typename T>
class QueueWait{
    std::mutex push_pop_mutex;
    std::mutex wait_mutex;

    std::mutex pop_wait_trigger_lock;
    std::condition_variable pop_wait_trigger;
    std::mutex empty_wait_trigger_lock;
    std::condition_variable empty_wait_trigger;

    bool empty_wait_flag = false;
    int count_wait = 0;
    int thread_pool_size;

public:
    QueueWait(int tps);
    std::queue<T> que;
    void unlock();
    T pop();
    void wait_pop();
    void wait_empty();
};

class Star{
public:
    static int star_counter;
    double x[dim];
    double v[dim];
    double m;
    double f[dim];

    QColor col;
    Star(double *coord, double *speed, double mass);
    Star& operator+=(Star* &rhs);
    ~Star(){star_counter--;}
    int Radius();
};

class Galaxy;

class Sector{
private:
    Galaxy* gl;

public:
    Sector(Galaxy* gl);
    Star** stars;
    int stars_count = 0;
    int max_star_index = 0;
    std::queue<Star*> wait_add;
    void Move(std::vector<Star*>& change_sector_requests_thread);
    ~Sector();
};

class Galaxy{
public:
    int num;
    int thread_pool_size;
    bool work = true;
    int count_stoped = 0;
    std::mutex count_stoped_mutex;
    Star* sun;
    Sector* sectors[sectors_count][sectors_count];
    QueueWait<Sector*>* selectors_queue;
    std::mutex change_sector_requests_mutex;
    std::vector<Star*> change_sector_requests;

    Galaxy(int n, int tps);
    ~Galaxy();
    Galaxy& operator>>(std::ofstream &rhs);
    Galaxy& operator<<(std::ifstream &rhs);
    Sector* GetSectorByCoords(double x, double y);
    void CreateSectors();
    void GenerateStars();
    void CreateThreadPool();
    void StarsToSectors();    
    void Move();
    void Stop();
};

#endif // STAR_H
