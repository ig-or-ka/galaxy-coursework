#include "star.h"
#include <iostream>
#include <thread>
#include <unordered_set>
#include <algorithm>
using namespace std;

const int borderMassC = 10;
const double G = 6.67408e-11, systemRadius = 1e12, distConnect = 1e9, dt = 30000;
const double massSun   = 1.98892e30,
             massEarth = 5.9742e24,
             massJup   = 1898.6e24,
             massUran  = 86.832e24;

const double borderMass[] = {borderMassC*massEarth, borderMassC*massUran, borderMassC*massJup, borderMassC*massSun};
const int nBorders = sizeof(borderMass) / sizeof(double);

const QColor colStar[] = {Qt::cyan, Qt::darkGreen, Qt::magenta, Qt::yellow, Qt::white};
const int nColor = sizeof(colStar) / sizeof(colStar[0]);

template <typename T>
void QueueWait<T>::unlock(){
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

    if(count_wait == thread_pool_size){
        while(empty_wait_flag){
            empty_wait_trigger.notify_all();
        }
    }
    wait_mutex.unlock();

    unique_lock <mutex> setTriggerUniqueLock (pop_wait_trigger_lock);
    pop_wait_trigger.wait (setTriggerUniqueLock);

    wait_mutex.lock();
    count_wait--;
    wait_mutex.unlock();
}

template <typename T>
void QueueWait<T>::wait_empty(){
    unique_lock <mutex> setTriggerUniqueLock (empty_wait_trigger_lock);
    empty_wait_trigger.wait (setTriggerUniqueLock);
    empty_wait_flag = false;
}

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
}

Star::~Star(){
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

void Sector::Move(vector<ChangeRequest>& change_sector_requests_thread){
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
                       change_sector_requests_thread.push_back({REMOVED_STAR,stars[j]});
                       (*stars[i]) += stars[j];
                       change_sector_requests_thread.push_back({CHANGE_MASS,stars[i]});
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
               change_sector_requests_thread.push_back({REMOVED_STAR,stars[i]});
               delete stars[i];
               stars[i] = nullptr;
               stars_count--;
           }
           else if(new_sector != this){
               change_sector_requests_thread.push_back({CHANGE_SECTOR,stars[i]});
               stars[i] = nullptr;
               stars_count--;
           }
       }
   }
}

void MoveThread(Galaxy* gl){
    vector<ChangeRequest> change_reqs;
    while(gl->work){
        Sector* sec = gl->selectors_queue->pop();
        if(sec == nullptr){
            gl->change_sector_requests_mutex.lock();

            gl->change_status_requests.insert(
                end(gl->change_status_requests),
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

    bool stop_wait_end = false;
    gl->count_stoped_mutex.lock();
    gl->count_stoped++;

    stop_wait_end = gl->thread_pool_size == gl->count_stoped;
    gl->count_stoped_mutex.unlock();

    if(stop_wait_end){
        while(!gl->stoped)
            gl->stop_wait_trigger.notify_all();
    }
}

Galaxy::Galaxy(int n, int tps):num(n),thread_pool_size(tps){
    selectors_queue = new QueueWait<Sector*>(tps);
    CreateThreadPool();
};

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

        Sector* sec = GetSectorByCoords(this_star->x[0],this_star->x[1]);
        if(sec){
            change_status_requests.push_back({CHANGE_SECTOR,this_star});
            if(i >= num - STARS_TOP_COUNT){
                top_mass_stars.push_back(this_star);
            }
        }
        else{
            delete this_star;
        }
    }

    DoRequests();
}

Galaxy::~Galaxy(){
    if(work){
        Stop();
    }

    delete selectors_queue;
    for(int i = 0; i < sectors_count; i++){
        for(int j = 0; j < sectors_count; j++){
            delete sectors[i][j];
        }
    }
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

bool sort_key_reqs(ChangeRequest a, ChangeRequest b){
    return a.star->m < b.star->m;
}

void Galaxy::DoRequests(){
    unordered_set<Star*> removed_stars;
    unordered_set<Star*> changed_mass_stars;

    for(auto request : change_status_requests){
        switch (request.action) {
            case CHANGE_SECTOR:
            {
                Sector* sec = GetSectorByCoords(request.star->x[0],request.star->x[1]);
                sec->wait_add.push(request.star);
            }
            break;

            case CHANGE_MASS:
                changed_mass_stars.insert(request.star);
            break;

            case REMOVED_STAR:
                removed_stars.insert(request.star);
            break;
        }
    }

    change_status_requests.clear();

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

    for(auto star : removed_stars){
        if(changed_mass_stars.count(star)){
            changed_mass_stars.erase(star);
        }
    }

    int count_removed = 0;
    for(size_t i = 0; i < top_mass_stars.size(); i++){
        if(removed_stars.count(top_mass_stars[i])){
            top_mass_stars[i] = 0;
            count_removed++;
        }
        else if(changed_mass_stars.count(top_mass_stars[i])){
            changed_mass_stars.erase(top_mass_stars[i]);
        }
    }

    if(changed_mass_stars.count(sun)){
        changed_mass_stars.erase(sun);
    }

    sort(top_mass_stars.begin(),top_mass_stars.end(),sort_key);

    for(int i = 0; i < count_removed; i++){
        top_mass_stars.pop_back();
    }

    for(auto star : changed_mass_stars){
        if(star->m > top_mass_stars[0]->m){
            top_mass_stars.push_back(star);
        }
    }

    sort(top_mass_stars.begin(),top_mass_stars.end(),sort_key);
    if(top_mass_stars.size() > ADD_STARS_TOP_COUNT + STARS_TOP_COUNT){
        for(size_t i = 0; i < top_mass_stars.size() - ADD_STARS_TOP_COUNT - STARS_TOP_COUNT; i++){
            top_mass_stars.erase(top_mass_stars.begin());
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
    for(auto star : all_stars){
        file.write((const char*)star->x,sizeof (double));
        file.write((const char*)&star->x[1],sizeof (double));
        file.write((const char*)star->v,sizeof (double));
        file.write((const char*)&star->v[1],sizeof (double));
        file.write((const char*)&star->m,sizeof (double));
    }
    file.flush();
    //cout << "Count saved stars: " << all_stars.size() << endl;
    return *this;
}

Galaxy& Galaxy::operator<<(ifstream &file){
    int stars_count = 0;
    file.read((char*)&stars_count,sizeof (int));
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
                change_status_requests.push_back({CHANGE_SECTOR,this_star});
            }
            else{
                delete this_star;
            }
        }
    }

    sort(change_status_requests.begin(),change_status_requests.end(),sort_key_reqs);
    for(int i = 0; i < STARS_TOP_COUNT; i++){
        top_mass_stars.push_back(change_status_requests[change_status_requests.size()-STARS_TOP_COUNT+i].star);
    }

    //cout << "Count loaded stars: " << stars_count << endl;
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

    DoRequests();
}

void Galaxy::Stop(){
    work = false;
    selectors_queue->unlock();
    unique_lock <mutex> setTriggerUniqueLock (stop_wait_trigger_lock);
    stop_wait_trigger.wait (setTriggerUniqueLock);
    stoped = true;
}
