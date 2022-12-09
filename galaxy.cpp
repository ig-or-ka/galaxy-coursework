#include "star.h"
#include <iostream>
#include <thread>
#include <unordered_set>
#include <algorithm>
using namespace std;

const int borderMassC = 10;
const double G = 6.67408e-11, distConnect = 1e9;
const double massSun   = 1.98892e30,
             massEarth = 5.9742e24,
             massJup   = 1898.6e24,
             massUran  = 86.832e24;

const double borderMass[] = {borderMassC*massEarth, borderMassC*massUran, borderMassC*massJup, borderMassC*massSun};
const int nBorders = sizeof(borderMass) / sizeof(double);

const QColor colStar[] = {Qt::cyan, Qt::darkGreen, Qt::magenta, Qt::yellow, Qt::white};
const int nColor = sizeof(colStar) / sizeof(colStar[0]);

void Star::ChangeColourRadius(){
    col = colStar[nColor-1];
    radius = 6;
    for(int i = 0; i < nBorders; i++){
        if(m < borderMass[i]){
            col = colStar[i];
            break;
        }
        radius += 3;
    }
}

Star& Star::operator+=(Star* &rhs) {
    for (int i = 0; i < dim;++i) {
        f[i] += rhs->f[i];
        v[i] = (v[i]*m + rhs->v[i]*rhs->m) / (m + rhs->m);
    }
    m += rhs->m;
    galaxy->stars_mass += rhs->m;
    ChangeColourRadius();

    delete rhs;
    rhs = nullptr;
    return *this;
}

Star::Star(double *coord, double *speed, double mass, Galaxy* gl){
    for(int k = 0; k < dim; ++k){
        x[k] = coord[k];
        v[k] = speed[k];
    }

    m = mass;
    ChangeColourRadius();
    galaxy = gl;
    gl->star_counter++;
    gl->stars_mass += m;
}

Star::~Star(){
    galaxy->stars_mass -= m;
    galaxy->star_counter--;
}

Sector::Sector(Galaxy* gl){
    this->gl = gl;
    stars = new Star*[starsInSector];
    stars[0] = gl->sun;

    for(int i = 1; i < starsInSector; i++){
        stars[i] = nullptr;
    }
}

Sector::~Sector(){
    for(int i = 1; i < starsInSector; i++){
        if(stars[i]){
            delete stars[i];
        }
    }
    delete [] stars;
}

void Sector::Move(){
   double dist;
   double dCoord[dim];
   for(int i = 0; i < max_star_index; ++i){ // force nullification
       for(int k = 0; k < dim; ++k){
           if(stars[i]){
               stars[i]->f[k] = 0;
           }
       }
   }
   for(int i = 0; i < max_star_index; i++){
       if(stars[i]){
           for(int j = i + 1; j < max_star_index; j++){
               if(i != j && stars[j]){
                   dist = 0;
                   for(int k = 0; k < dim; ++k){
                       dCoord[k] = stars[i]->x[k] - stars[j]->x[k];
                       dist += dCoord[k] * dCoord[k];
                   }
                   if(sqrt(dist) < distConnect){
                       (*stars[i]) += stars[j];
                   }
               }
           }
       }
   }
   for(int i = 0; i < max_star_index; i++){
       if(stars[i]){
           for(int j = i + 1; j < max_star_index; j++){
               if(i != j && stars[j]){
                   dist = 0;
                   for(int k = 0; k < dim; ++k){
                       dCoord[k] = stars[i]->x[k] - stars[j]->x[k];
                       dist += dCoord[k] * dCoord[k];
                   }

                   for(int k = 0; k < dim; ++k){
                       double tmp = G * stars[i]->m * stars[j]->m / dist;
                       stars[i]->f[k] -= tmp * dCoord[k] / sqrt(dist);
                       stars[j]->f[k] += tmp * dCoord[k] / sqrt(dist);
                   }
               }
           }
       }
   }
   for(int i = 1; i < max_star_index; ++i){
       if(stars[i]){
           for(int k = 0; k < dim; ++k){
               stars[i]->v[k] += gl->dt * stars[i]->f[k] / stars[i]->m;
           }
           for(int k = 0; k < dim; ++k){
               stars[i]->x[k] += gl->dt * stars[i]->v[k];
           }

           Sector* new_sector = gl->GetSectorByCoords(stars[i]->x[0],stars[i]->x[1]);

           if(new_sector == nullptr){
               delete stars[i];
               stars[i] = nullptr;
               stars_count--;
           }
           else if(new_sector != this){
               new_sector->wait_add.push(stars[i]);
               stars[i] = nullptr;
               stars_count--;
           }
       }
   }
}

Galaxy::Galaxy(int count_stars, int size_sector, int size_rect, double system_radius, int dt){
    this->dt = dt;
    num = count_stars;
    sun_radius = size_sector;
    length = size_rect;
    systemRadius = system_radius;

    Init();
};

void Galaxy::Init(){
    coefX = length / 2 / systemRadius;
    centerX = length / 2;
    sector_global_h = sun_radius / coefX;
    sectors_count = length / sun_radius;

    sectors = new Sector**[sectors_count];
    for(int i = 0; i < sectors_count; i++){
        sectors[i] = new Sector*[sectors_count];
    }
}

Galaxy::Galaxy(){}

void Galaxy::GenerateStars(){
    double x1[dim] = {0}, v1[dim] = {0};
    sun = new Star(x1, v1, massSun, this);

    CreateSectors();
    double rad;
    for(int i = 1; i < num; ++i){
        rad = 0;
        double R = rand() * systemRadius / RAND_MAX,
        fi  = (2 * M_PI * rand()) / RAND_MAX;
        x1[0] = R  * cos(fi);
        x1[1] = R  * sin(fi);
        for(int k = 0; k < dim; ++k){
            rad += x1[k] * x1[k];
        }
        // вторая космическая скорость
        double absV = sqrt(G * sectors[0][0]->stars[0]->m / sqrt(rad));
        v1[0] =  absV * sin(fi);
        v1[1] = -absV * cos(fi); // скорость направлена вдоль окружности с центром в начале координат
        Star* this_star = new Star(x1, v1, massEarth / num * (6 * i), this);
        //Star* this_star = new Star(x1, v1, massSun / 2, this);

        Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
        if(sec){
            sec->wait_add.push(this_star);
        }
        else{
            delete this_star;
        }
    }

    DoRequests();
}

Galaxy::~Galaxy(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            delete sectors[i][j];
        }
        delete [] sectors[i];
    }
    delete [] sectors;
    delete sun;
};

Sector* Galaxy::GetSectorByCoords(double x, double y){
    int dx = x / sector_global_h + sectors_count / 2;
    int dy = y / sector_global_h + sectors_count / 2;

    if(dx >= 0 && dx < sectors_count && dy >= 0 && dy < sectors_count){
        return sectors[dx][dy];
    }

    return nullptr;
}

bool sort_key(Star* a, Star* b){
    if(!a){
        return false;
    }
    if(!b){
        return true;
    }
    return a->m < b->m;
}

void Galaxy::DoRequests(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            for(int k = 1; k < starsInSector; k++){
                if(sectors[i][j]->wait_add.size() == 0){
                    break;
                }

                if(!sectors[i][j]->stars[k]){
                    sectors[i][j]->stars[k] = sectors[i][j]->wait_add.front();
                    sectors[i][j]->wait_add.pop();
                    sectors[i][j]->stars_count++;
                    if(k + 1 > sectors[i][j]->max_star_index){
                        sectors[i][j]->max_star_index = k + 1;
                    }
                }
            }
        }
    }
}

void Galaxy::CreateSectors(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            sectors[i][j] = new Sector(this);
        }
    }
}

Galaxy& Galaxy::operator>>(ofstream &file){
    vector<Star*> all_stars;
    all_stars.push_back(sun);

    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            if(sectors[i][j]->stars_count){
                auto sec = sectors[i][j];

                for(int k = 1; k < sec->max_star_index; k++){
                    auto star_i = sec->stars[k];
                    if(star_i){
                        all_stars.push_back(star_i);
                    }
                }
            }
        }
    }

    int stars_count = all_stars.size();
    file.write((const char*)&stars_count,sizeof (int));
    file.write((const char*)&length,sizeof (int));
    file.write((const char*)&sun_radius,sizeof (int));
    file.write((const char*)&systemRadius,sizeof (double));
    file.write((const char*)&dt,sizeof (int));

    for(auto star : all_stars){
        file.write((const char*)star->x,sizeof (double));
        file.write((const char*)&star->x[1],sizeof (double));
        file.write((const char*)star->v,sizeof (double));
        file.write((const char*)&star->v[1],sizeof (double));
        file.write((const char*)&star->m,sizeof (double));
    }
    file.flush();
    return *this;
}

Galaxy& Galaxy::operator<<(ifstream &file){
    int stars_count = 0;

    file.read((char*)&stars_count,sizeof (int));
    file.read((char*)&length,sizeof (int));
    file.read((char*)&sun_radius,sizeof (int));
    file.read((char*)&systemRadius,sizeof (double));
    file.read((char*)&dt,sizeof (int));
    Init();

    for(int i = 0; i < stars_count; i++){
        double star_vals[5];
        file.read((char*)&star_vals, sizeof(star_vals));
        double x[] = {star_vals[0],star_vals[1]};
        double v[] = {star_vals[2],star_vals[3]};
        Star* this_star = new Star(x, v, star_vals[4],this);

        if(i == 0){
            sun = this_star;
            CreateSectors();
        }
        else{
            Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
            if(sec){
                sec->wait_add.push(this_star);
            }
            else{
                delete this_star;
            }
        }
    }

    DoRequests();

    return *this;
}

void Galaxy::Move(){
    step++;
    time += dt;
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            if(sectors[i][j]->stars_count){
                sectors[i][j]->Move();
            }
        }
    }

    DoRequests();
}
