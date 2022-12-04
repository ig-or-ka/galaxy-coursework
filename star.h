#include "cmath"
#include <QPainter>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>

#ifndef STAR_H
#define STAR_H

const int threadPoolSize = 20;
const int starsInSector = 1000;
const int sun_radius = 20;
const int topX0 = 100, topY0 = 100, h = 800, length = 800;

const double coefX = length / 2 / 1e12;
const int centerX = length / 2;
const double sector_global_h = sun_radius / coefX;
const int sectors_count = length / sun_radius;

const int dim = 2;
const int numStars = 100000;
const int STARS_TOP_COUNT = 100;
const int ADD_STARS_TOP_COUNT = 100;

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

class Galaxy;

class Star{
public:
    double x[dim];
    double v[dim];
    double m;
    double f[dim];
    Galaxy* galaxy;

    QColor col;
    int radius = 6;
    Star(double *coord, double *speed, double mass, Galaxy* gl);
    Star& operator+=(Star* &rhs);

    ~Star();
    void ChangeColourRadius();
};

const int CHANGE_SECTOR = 1;
const int REMOVED_STAR = 2;
const int CHANGE_MASS = 3;

struct ChangeRequest{
    int action;
    Star* star;
};

class Sector{
private:
    Galaxy* gl;

public:
    Sector(Galaxy* gl);
    Star** stars;
    int stars_count = 0;
    int max_star_index = 0;
    std::queue<Star*> wait_add;
    void Move(std::vector<ChangeRequest>& change_sector_requests_thread);
    ~Sector();
};

class Galaxy{
public:
    int star_counter = 0;
    int num;
    int thread_pool_size;
    bool work = true;
    bool stoped = false;
    int count_stoped = 0;
    std::mutex count_stoped_mutex;
    Star* sun;
    Sector* sectors[sectors_count][sectors_count];
    QueueWait<Sector*>* selectors_queue;
    std::mutex change_sector_requests_mutex;
    std::vector<ChangeRequest> change_status_requests;
    std::mutex stop_wait_trigger_lock;
    std::condition_variable stop_wait_trigger;
    std::vector<Star*> top_mass_stars;

    Galaxy(int n, int tps);
    ~Galaxy();
    Galaxy& operator>>(std::ofstream &rhs);
    Galaxy& operator<<(std::ifstream &rhs);
    Sector* GetSectorByCoords(double x, double y);
    void CreateSectors();
    void GenerateStars();
    void CreateThreadPool();
    void DoRequests();
    void Move();
    void Stop();
};

#endif // STAR_H
