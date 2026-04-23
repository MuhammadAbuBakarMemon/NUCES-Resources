#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_WORKSTATIONS 2
#define STALE_THRESHOLD 4
#define SYSTEM_UPTIME 45
#define BUFFER_CAPACITY 100

#define INTERNAL_RESTOCK 0
#define STANDARD_BUILD 1
#define RUSH_BUILD 2
#define VIP_EXPEDITE 3
#define SAFETY_RECALL 4

typedef struct {
    int order_id;
    int category;
    time_t time_logged;
    int escalation_score;
} WorkOrder;

typedef struct {
    int station_id;
    int is_active;
    int halt_signal;
} Workstation;

WorkOrder backlog_buffer[BUFFER_CAPACITY];
int current_backlog_count = 0;
Workstation stations[NUM_WORKSTATIONS];

pthread_mutex_t backlog_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t backlog_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t station_mutex = PTHREAD_MUTEX_INITIALIZER;

int calculate_weight(WorkOrder* order) {
    int base_score = (order->category == SAFETY_RECALL) ? 10000 :
                     (order->category == VIP_EXPEDITE) ? 800 :
                     (order->category == RUSH_BUILD) ? 600 :
                     (order->category == STANDARD_BUILD) ? 400 : 200;

    return base_score + order->escalation_score;
}

const char* get_category_label(int cat) {
    static const char* labels[] = {
        "INTERNAL_RESTOCK", 
        "STANDARD_BUILD", 
        "RUSH_BUILD", 
        "VIP_EXPEDITE", 
        "SAFETY_RECALL"
    };
    return labels[cat];
}

// Retaining this function as it existed in your original code, 
// even though it was unused in main().
void* auto_generate_orders(void* dummy_arg) {
    int auto_id = 1;
    while(1) {
        sleep(rand() % 3 + 1);
        WorkOrder fresh_order;
        fresh_order.order_id = auto_id++;

        int rng = rand() % 100;
        fresh_order.category = (rng < 5)  ? SAFETY_RECALL :
                               (rng < 15) ? VIP_EXPEDITE :
                               (rng < 50) ? RUSH_BUILD :
                               (rng < 85) ? STANDARD_BUILD : INTERNAL_RESTOCK;

        fresh_order.time_logged = time(NULL);
        fresh_order.escalation_score = 0;

        pthread_mutex_lock(&backlog_mutex);
        if (current_backlog_count < BUFFER_CAPACITY) {
            backlog_buffer[current_backlog_count++] = fresh_order;
            printf("[GENERATOR] Order #%d [%s] entered the backlog.\n", fresh_order.order_id, get_category_label(fresh_order.category));
            pthread_cond_signal(&backlog_cond);
        } else {
            printf("[GENERATOR] System overloaded. Order #%d dropped.\n", fresh_order.order_id);
        }
        pthread_mutex_unlock(&backlog_mutex);
    }
    return NULL;
}

void* operate_station(void* arg) {
    int sid = *((int*)arg);

    while(1) {
        pthread_mutex_lock(&backlog_mutex);

        while(current_backlog_count == 0) {
            pthread_cond_wait(&backlog_cond, &backlog_mutex);
        }

        if (rand() % 100 < 10) {
            printf("[STATION %d] Entering automated calibration cycle...\n", sid);
            pthread_mutex_unlock(&backlog_mutex);
            sleep(3);
            continue;
        }

        int target_idx = 0;
        int highest_weight = -1;
        int iter = 0;

        while(iter < current_backlog_count) {
            int current_weight = calculate_weight(&backlog_buffer[iter]);
            if (current_weight > highest_weight) {
                highest_weight = current_weight;
                target_idx = iter;
            }
            iter++;
        }

        WorkOrder active_job = backlog_buffer[target_idx];
        
        int shift_idx = target_idx;
        while(shift_idx < current_backlog_count - 1) {
            backlog_buffer[shift_idx] = backlog_buffer[shift_idx + 1];
            shift_idx++;
        }
        current_backlog_count--;
        pthread_mutex_unlock(&backlog_mutex);

        pthread_mutex_lock(&station_mutex);
        stations[sid].is_active = 1;
        stations[sid].halt_signal = 0;
        pthread_mutex_unlock(&station_mutex);
       
        printf(">> [STATION %d] Commencing assembly for Order #%d [%s]\n", sid, active_job.order_id, get_category_label(active_job.category));
       
        int job_halted = 0;
        int processing_ticks = 0;
        
        while(processing_ticks < 4) {
            sleep(1);
            pthread_mutex_lock(&station_mutex);
            if(stations[sid].halt_signal) {
                job_halted = 1;
                pthread_mutex_unlock(&station_mutex);
                break;
            }
            pthread_mutex_unlock(&station_mutex);
            processing_ticks++;
        }

        pthread_mutex_lock(&station_mutex);
        stations[sid].is_active = 0;
        pthread_mutex_unlock(&station_mutex);

        if (job_halted) {
            printf("!! [STATION %d] HALTED. Order #%d kicked back to queue due to RECALL interrupt.\n", sid, active_job.order_id);
            pthread_mutex_lock(&backlog_mutex);
            if(current_backlog_count < BUFFER_CAPACITY) {
                active_job.time_logged = time(NULL);
                backlog_buffer[current_backlog_count++] = active_job;
                pthread_cond_signal(&backlog_cond);
            }
            pthread_mutex_unlock(&backlog_mutex);
        } else {
            printf("<< [STATION %d] Successfully manufactured Order #%d\n", sid, active_job.order_id);
        }
    }
    return NULL;
}

void* overseer_routine(void* arg) {
    while(1) {
        sleep(2);
        time_t timestamp = time(NULL);
        int recall_detected = 0;
        
        pthread_mutex_lock(&backlog_mutex);
        int idx = 0;
        while(idx < current_backlog_count) {
            double wait_duration = difftime(timestamp, backlog_buffer[idx].time_logged);
            
            if(wait_duration > STALE_THRESHOLD && backlog_buffer[idx].category != SAFETY_RECALL) {
                backlog_buffer[idx].escalation_score += 50;
            }
            if(backlog_buffer[idx].category == SAFETY_RECALL) {
                recall_detected = 1;
            }
            idx++;
        }
        pthread_mutex_unlock(&backlog_mutex);

        if(recall_detected) {
            pthread_mutex_lock(&station_mutex);
            int network_saturated = 1;
            int target_station = -1;
            int s = 0;
       
            while(s < NUM_WORKSTATIONS) {
                if(stations[s].is_active == 0) {
                    network_saturated = 0;
                    break;
                }
                target_station = s;
                s++;
            }
           
            if(network_saturated && target_station != -1) {
                stations[target_station].halt_signal = 1;
                printf("[OVERSEER] Safety Recall protocol triggered. Sending HALT signal to Station %d\n", target_station);
            }
            pthread_mutex_unlock(&station_mutex);
        }
    }
    return NULL;
}

void inject_work_order(int uid, int cat) {
    pthread_mutex_lock(&backlog_mutex);
    if(current_backlog_count < BUFFER_CAPACITY) {
        backlog_buffer[current_backlog_count].order_id = uid;
        backlog_buffer[current_backlog_count].category = cat;
        backlog_buffer[current_backlog_count].time_logged = time(NULL);
        backlog_buffer[current_backlog_count].escalation_score = 0;
        printf("[DISPATCH] Order #%d [%s] manually requested.\n", uid, get_category_label(cat));
        current_backlog_count++;
        pthread_cond_signal(&backlog_cond);
    }
    pthread_mutex_unlock(&backlog_mutex);
}

int main() {
    int i = 0;
    while(i < NUM_WORKSTATIONS) {
        stations[i].station_id = i;
        stations[i].is_active = 0;
        stations[i].halt_signal = 0;
        i++;
    }

    pthread_t overseer_thread;
    pthread_t worker_threads[NUM_WORKSTATIONS];
    int worker_ids[NUM_WORKSTATIONS];

    pthread_create(&overseer_thread, NULL, overseer_routine, NULL);
    
    int w = 0;
    while(w < NUM_WORKSTATIONS) {
        worker_ids[w] = w;
        pthread_create(&worker_threads[w], NULL, operate_station, &worker_ids[w]);
        w++;
    }

    usleep(100000);

    printf("\n=== DIAGNOSTIC: PRIORITY SCHEDULING ===\n");
    inject_work_order(991, INTERNAL_RESTOCK);
    inject_work_order(992, INTERNAL_RESTOCK);
    sleep(1);

    inject_work_order(101, INTERNAL_RESTOCK);
    inject_work_order(102, STANDARD_BUILD);
    inject_work_order(103, RUSH_BUILD);

    sleep(15);

    printf("\n=== DIAGNOSTIC: STAGNATION ESCALATION (AGING) ===\n");
    inject_work_order(993, INTERNAL_RESTOCK);
    inject_work_order(994, INTERNAL_RESTOCK);
    sleep(1);

    inject_work_order(201, INTERNAL_RESTOCK);
    sleep(6);

    inject_work_order(202, STANDARD_BUILD);
    sleep(15);

    printf("\n=== DIAGNOSTIC: STATION PREEMPTION ===\n");
    inject_work_order(301, INTERNAL_RESTOCK);
    inject_work_order(302, INTERNAL_RESTOCK);
    sleep(1);
  
    inject_work_order(911, SAFETY_RECALL);
    sleep(10);
  
    return 0;
}