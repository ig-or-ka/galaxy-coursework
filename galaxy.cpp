#include "star.h"
#include <iostream>
#include <thread>
using namespace std;

template <typename T>
void QueueWait<T>::unlock(){
    count_wait = 0;
    empty_wait_flag = true;
    pop_wait_trigger.notify_all();
}

template <typename T>
T QueueWait<T>::pop(){
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
QueueWait<T>::QueueWait(int tps):thread_pool_size(tps){};

template <typename T>
void QueueWait<T>::wait_pop(){
    wait_mutex.lock();
    count_wait++;
    wait_mutex.unlock();

    if(count_wait == thread_pool_size){
        while(empty_wait_flag){
            empty_wait_trigger.notify_all();
        }
    }

    unique_lock <mutex> setTriggerUniqueLock (pop_wait_trigger_lock);
    pop_wait_trigger.wait (setTriggerUniqueLock);
}

template <typename T>
void QueueWait<T>::wait_empty(){
    unique_lock <mutex> setTriggerUniqueLock (empty_wait_trigger_lock);
    empty_wait_trigger.wait (setTriggerUniqueLock);
    empty_wait_flag = false;
}

int Star::star_counter = 0;
int Star::Radius(){
    //return 6 + m / massEarth;
    return 6;
}

Star& Star::operator+=(Star* &rhs) {
    for (int i = 0; i < dim;++i) {
        f[i] += rhs->f[i];
        v[i] = (v[i]*m + rhs->v[i]*rhs->m) / (m + rhs->m);
    }
    m += rhs->m;

    delete rhs;
    rhs = nullptr;
    return *this;
}

Star::Star(double *coord, double *speed, double mass){
    for(int k = 0; k < dim; ++k){
        x[k] = coord[k];
        v[k] = speed[k];
    }

    m = mass;
    col = Qt::cyan;
    star_counter++;
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

void Sector::Move(vector<Star*>& change_sector_requests_thread){
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
                       //cout << "add" << endl;
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

void MoveThread(Galaxy* gl){
    vector<Star*> change_reqs;
    while(gl->work){
        Sector* sec = gl->selectors_queue->pop();
        if(sec == nullptr){
            gl->change_sector_requests_mutex.lock();

            gl->change_sector_requests.insert(
                end(gl->change_sector_requests),
                begin(change_reqs),
                end(change_reqs)
            );

            gl->change_sector_requests_mutex.unlock();
            change_reqs.clear();

            gl->selectors_queue->wait_pop();
            continue;
        }
        sec->Move(change_reqs);
    }
    gl->count_stoped++;
}

Galaxy::Galaxy(int n, int tps):num(n),thread_pool_size(tps){
    selectors_queue = new QueueWait<Sector*>(tps);
    GenerateStars();
    StarsToSectors();
    CreateThreadPool();
};

void Galaxy::GenerateStars(){
    double x1[dim] = {0}, v1[dim] = {0};
    sun = new Star(x1, v1, massSun);

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
        Star* this_star = new Star(x1, v1, massEarth / num * (6 * i));

        Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
        if(sec){
            change_sector_requests.push_back(this_star);
        }
        else{
            delete this_star;
        }
    }
}

Galaxy::~Galaxy(){
    delete selectors_queue;
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            delete sectors[i][j];
        }
    }
    delete sun;
    work = false;
    selectors_queue->unlock();
    while (count_stoped != thread_pool_size);
};

Sector* Galaxy::GetSectorByCoords(double x, double y){
    int dx = x / sector_global_h + sectors_count / 2;
    int dy = y / sector_global_h + sectors_count / 2;

    if(dx > 0 && dx < sectors_count && dy > 0 && dy < sectors_count){
        return sectors[dx][dx];
    }

    return nullptr;
}

void Galaxy::StarsToSectors(){
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

void Galaxy::CreateSectors(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            sectors[i][j] = new Sector(this);
        }
    }
}

void Galaxy::CreateThreadPool(){
    for(int i = 0; i < thread_pool_size; i++){
        thread th(MoveThread,this);
        th.detach();
    }
}

Galaxy& Galaxy::operator>>(ofstream &file){
    cout << "Okda" << endl;
    return *this;
}

void Galaxy::Move(){
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            if(sectors[i][j]->stars_count){
                selectors_queue->que.push(sectors[i][j]);
            }
        }
    }
    selectors_queue->unlock();
    selectors_queue->wait_empty();

    StarsToSectors();
    change_sector_requests.clear();
}
