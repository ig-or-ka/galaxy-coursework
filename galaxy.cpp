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
    empty_wait_flag = true;
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

        return value;
    }
}

template <typename T>
void queue_wait<T>::wait_pop(){
    wait_mutex.lock();
    count_wait++;
    wait_mutex.unlock();

    if(count_wait == moveThreadPool){
        while(empty_wait_flag){
            empty_wait.set();
        }
    }

    pop_wait.wait();
}

template <typename T>
void queue_wait<T>::wait_empty(){
    empty_wait.wait();
    empty_wait_flag = false;
}

int star::starCounter = 0;
int star::radius(){
    //return 6 + m / massEarth;
    return 6;
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
    stars = new star*[starsInSector];
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

void Sector::move(std::vector<star*>& change_sector_requests_thread){
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
                       double tmpM = stars[i]->m + stars[j]->m, tmpX[dim], tmpV[dim];
                       for(int k = 0; k < dim; ++k){
                           tmpX[k] = (stars[i]->x[k] * stars[i]->m + stars[j]->x[k] * stars[j]->m)/tmpM;
                           tmpV[k] = (stars[i]->v[k] * stars[i]->m + stars[j]->v[k] * stars[j]->m)/tmpM;
                       }

                       delete stars[j];
                       stars[j] = nullptr;
                       stars_count--;

                       stars[i]->m = tmpM;
                       for(int k = 0; k < dim; ++k){
                           stars[i]->x[k] = tmpX[k];
                           stars[i]->v[k] = tmpV[k];
                       }
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
                   // dist = sqrt(dist); // для знаменателя пока квадрат, потом возьмем корень
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
               stars[i]->v[k] += dt * stars[i]->f[k] / stars[i]->m; //можно не делить на массу, а выше суммировать ускорение
           }
           for(int k = 0; k < dim; ++k){
               stars[i]->x[k] += dt * stars[i]->v[k];
           }

           Sector* new_sector = gl->GetSectorByCoords(stars[i]->x[0],stars[i]->x[1]);

           if(new_sector == nullptr){
               delete stars[i];
               stars[i] = nullptr;
               stars_count--;
           }
           else if(new_sector != this){
               change_sector_requests_thread.push_back(stars[i]);
               stars[i] = nullptr;
               stars_count--;
           }
       }
   }
}

void move_thread(galaxy* gl){
    std::vector<star*> change_reqs;
    while(true){
        Sector* sec = gl->selectors_queue.pop();
        if(sec == nullptr){
            gl->change_sector_requests_mutex.lock();

            gl->change_sector_requests.insert(
                std::end(gl->change_sector_requests),
                std::begin(change_reqs),
                std::end(change_reqs)
            );

            gl->change_sector_requests_mutex.unlock();
            change_reqs.clear();

            gl->selectors_queue.wait_pop();
            continue;
        }
        sec->move(change_reqs);
    }
}

galaxy::galaxy(int n):num(n){
    // самый массивный объект в начале координат
    double x1[dim] = {0}, v1[dim] = {0};
    sun = new star(x1, v1, massSun);
    CreateSectors();

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
        double absV = sqrt(G * sectors[0][0]->stars[0]->m / sqrt(rad)), alpha = (2 * M_PI * rand()) / RAND_MAX;
//если размерность 3, нужен еще один угол как для координат(два угла годятся и для плоскости, желающие могут сделать)
//            v1[0] = absV * cos(alpha);
//            v1[1] = absV * sin(alpha);
        v1[0] =  absV * sin(fi);
        v1[1] = -absV * cos(fi); // скорость направлена вдоль окружности с центром в начале координат
        star* this_star = new star(x1, v1, massEarth / num * (6 * i));

        Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
        if(sec){
            change_sector_requests.push_back(this_star);
        }
        else{
            delete this_star;
        }
    }
    StarsToSectors();

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
    delete sun;
};

Sector* galaxy::GetSectorByCoords(double x, double y){
    int dx = x / sector_global_h + 40;
    int dy = y / sector_global_h + 40;

    if(dx > 0 && dx < sectors_count && dy > 0 && dy < sectors_count){
        return sectors[dx][dx];
    }

    return nullptr;
}
void galaxy::StarsToSectors(){
    for(auto star_i : change_sector_requests){
        Sector* sec = GetSectorByCoords(star_i->x[0],star_i->x[1]);
        sec->wait_add.push(star_i);
    }

    change_sector_requests.clear();

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
            if(sectors[i][j]->stars_count){
                selectors_queue.que.push(sectors[i][j]);
            }
        }
    }
    selectors_queue.unlock();
    selectors_queue.wait_empty();

    StarsToSectors();
    change_sector_requests.clear();
}
