#include "cmath"
#include <QPainter>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>

#ifndef STAR_H
#define STAR_H

const int starsInSector = 1000;
const int topX0 = 100, topY0 = 100;
const int dim = 2;
const int STARS_TOP_COUNT = 100;
const int ADD_STARS_TOP_COUNT = 100;

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

class Sector{
private:
    Galaxy* gl;

public:
    Sector(Galaxy* gl);
    Star** stars;
    int stars_count = 0;
    int max_star_index = 0;
    std::queue<Star*> wait_add;
    void Move();
    ~Sector();
};

class Galaxy{
public:
    long time = 0;
    long step = 0;
    int star_counter = 0;
    double stars_mass = 0;
    std::mutex counter_mutex;
    int num;
    Star* sun;
    Sector*** sectors;

    int length;
    int sun_radius;
    double systemRadius = 1e12;
    int dt;

    double coefX;
    int centerX;
    double sector_global_h;
    int sectors_count;

    Galaxy(int count_stars, int size_sector, int size_rect, double system_radius, int dt);
    Galaxy();
    void Init();
    ~Galaxy();
    Galaxy& operator>>(std::ofstream &rhs);
    Galaxy& operator<<(std::ifstream &rhs);
    Sector* GetSectorByCoords(double x, double y);
    void CreateSectors();
    void GenerateStars();
    void DoRequests();
    void Move();
};

#endif // STAR_H
