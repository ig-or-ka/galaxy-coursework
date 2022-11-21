#include "star.h"

void EventWait::wait(){
    std::unique_lock <std::mutex> setTriggerUniqueLock (setTriggerLock);
    setTrigger.wait (setTriggerUniqueLock);
}

void EventWait::set(){
    setTrigger.notify_all();
}

template <typename T>
void queue_wait<T>::unlock(){
    count_wait = 0;
    pop_wait.set();
}

template <typename T>
T queue_wait<T>::pop(){
    while (true) {
        push_pop_mutex.lock();

        T value = nullptr;
        if(!que.empty()){
            value = que.front();
            que.pop();
        }

        push_pop_mutex.unlock();

        if(value != nullptr){
            return value;
        }

        push_pop_mutex.lock();
        count_wait++;
        push_pop_mutex.unlock();

        if(count_wait == moveThreadPool){
            empty_wait.set();
        }
        pop_wait.wait();
    }
}

template <typename T>
void queue_wait<T>::wait_empty(){
    empty_wait.wait();
}

int star::starCounter = 0;
int star::radius(){
    return 6 + m / massEarth;;
}

star::star(double *coord, double *speed, double mass){
    for(int k = 0; k < dim; ++k){
        x[k] = coord[k];
        v[k] = speed[k];
    }

    m = mass;
    col = Qt::cyan;
    starCounter++;
}

Sector::Sector(galaxy* gl){
    this->gl = gl;
}

Sector::~Sector(){
    for(auto star : stars){
        delete star;
    }
}

void Sector::for_check_1(star* star_i, std::set<star*>& used_stars, std::vector<star*>& to_remove){
    double dist;
    double dCoord[dim];

    for(auto star_j : stars){
        if(!star_j->removed && !used_stars.count(star_j)){
            dist = 0;
            for(int k = 0; k < dim; ++k){
                dCoord[k] = star_i->x[k] - star_j->x[k];
                dist += dCoord[k] * dCoord[k];
            }
            if(sqrt(dist) < distConnect){
                double tmpM = star_i->m + star_j->m, tmpX[dim], tmpV[dim];
                for(int k = 0; k < dim; ++k){
                    tmpX[k] = (star_i->x[k] * star_i->m + star_j->x[k] * star_j->m)/tmpM;
                    tmpV[k] = (star_i->v[k] * star_i->m + star_j->v[k] * star_j->m)/tmpM;
                }

                star_j->removed = true;
                to_remove.push_back(star_j);

                star_i->m = tmpM;
                for(int k = 0; k < dim; ++k){
                    star_i->x[k] = tmpX[k];
                    star_i->v[k] = tmpV[k];
                }
            }
        }
    }
}

void Sector::for_check_2(star* star_i, std::set<star*>& used_stars){
    double dist;
    double dCoord[dim];

    for(auto star_j : stars){
        if(!used_stars.count(star_j)){
            dist = 0;
            for(int k = 0; k < dim; ++k){
                dCoord[k] = star_i->x[k] - star_j->x[k];
                dist += dCoord[k] * dCoord[k];
            }
            // dist = sqrt(dist); // для знаменателя пока квадрат, потом возьмем корень
            for(int k = 0; k < dim; ++k){
                double tmp = G * star_i->m * star_j->m / dist;
                star_i->f[k] -= tmp * dCoord[k] / sqrt(dist);
                star_j->f[k] += tmp * dCoord[k] / sqrt(dist);
            }
        }
    }
}

void Sector::move(){
    for(auto star_i : stars){ // force nullification
        for(int k = 0; k < dim; ++k){
            star_i->f[k] = 0;
        }
    }

    std::set<star*> used_stars;
    std::vector<star*> to_remove;

    used_stars.insert(gl->sun);
    for_check_1(gl->sun,used_stars,to_remove);

    for(auto star_i : stars){
        used_stars.insert(star_i);
        for_check_1(star_i,used_stars,to_remove);
    }
    used_stars.clear();

    for(auto star_i : to_remove){
        delete star_i;
        stars.erase(star_i);
    }

    used_stars.insert(gl->sun);
    for_check_2(gl->sun,used_stars);

    for(auto star_i : stars){
        used_stars.insert(star_i);
        for_check_2(star_i,used_stars);
    }

    for(auto star_i : stars){
        for(int k = 0; k < dim; ++k){
            star_i->v[k] += dt * star_i->f[k] / star_i->m; //можно не делить на массу, а выше суммировать ускорение
        }
        for(int k = 0; k < dim; ++k){
            star_i->x[k] += dt * star_i->v[k];
        }
    }

    to_remove.clear();
    for(auto star_i : stars){
        Sector* new_sector = gl->GetSectorByCoords(star_i->x[0],star_i->x[1]);

        if(new_sector == nullptr){
            star_i->removed = true;
            to_remove.push_back(star_i);
        }
        else if(new_sector != this){
            to_remove.push_back(star_i);

            gl->change_sector_requests_mutex.lock();
            gl->change_sector_requests.push_back(star_i);
            gl->change_sector_requests_mutex.unlock();
        }
    }

    for(auto star_i : to_remove){
        stars.erase(star_i);
        if(star_i->removed){
            delete star_i;
        }
    }
}

void move_thread(galaxy* gl){
    while(true){
        Sector* sec = gl->selectors_queue.pop();
        sec->move();
    }
}

galaxy::galaxy(int n):num(n){
    CreateSectors();
    CreateSun(); // самый массивный объект в начале координат

    double x1[dim] = {0}, v1[dim] = {0};
    double rad;
    for(int i = 1; i < num; ++i){
        rad = 0;
        double R = rand() * systemRadius / RAND_MAX,
        fi  = (2 * M_PI * rand()) / RAND_MAX,
        theta = (M_PI * rand()) / RAND_MAX;
        x1[0] = R  * cos(fi);
        x1[1] = R  * sin(fi);
        if(dim == 3){
            x1[0] *= sin(theta);
            x1[1] *= sin(theta);
            x1[3] = R * cos(theta);
        }
        for(int k = 0; k < dim; ++k){
            rad += x1[k] * x1[k];
        }
// вторая космическая скорость
        double absV = sqrt(G * sun->m / sqrt(rad)), alpha = (2 * M_PI * rand()) / RAND_MAX;
//если размерность 3, нужен еще один угол как для координат(два угла годятся и для плоскости, желающие могут сделать)
//            v1[0] = absV * cos(alpha);
//            v1[1] = absV * sin(alpha);
        v1[0] =  absV * sin(fi);
        v1[1] = -absV * cos(fi); // скорость направлена вдоль окружности с центром в начале координат
        star* this_star = new star(x1, v1, massEarth / num * (6 * i));

        Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
        if(sec){
            sec->stars.insert(this_star);
        }
        else{
            delete this_star;
        }
    }

    //create thream pool
    for(int i = 0; i < moveThreadPool; i++){
        std::thread th(move_thread,this);
        th.detach();
    }
};
galaxy::~galaxy(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            delete sectors[i][j];
        }
    }
};

Sector* galaxy::GetSectorByCoords(double x, double y){
    int dx = x / sector_global_h + 40;
    int dy = y / sector_global_h + 40;

    if(dx > 0 && dx < sectors_count && dy > 0 && dy < sectors_count){
        return sectors[dx][dx];
    }

    return nullptr;
}

void galaxy::CreateSun(){
    double x1[dim] = {0}, v1[dim] = {0};
    sun = new star(x1, v1, massSun);
}
void galaxy::CreateSectors(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            sectors[i][j] = new Sector(this);
        }
    }
}

void galaxy::move(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            if(sectors[i][j]->stars.size()){
                selectors_queue.que.push(sectors[i][j]);
            }
        }
    }
    selectors_queue.unlock();
    selectors_queue.wait_empty();

    for(auto star_i : change_sector_requests){
        Sector* sec = GetSectorByCoords(star_i->x[0],star_i->x[1]);
        sec->stars.insert(star_i);
    }
    change_sector_requests.clear();
}
