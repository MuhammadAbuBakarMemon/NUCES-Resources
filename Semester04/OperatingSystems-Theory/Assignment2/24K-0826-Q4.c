/*
NAME: MUHAMMAD ABU BAKAR 
NUID: 24k-0826

LECTURER: SIR DR. GHUFRAN AHMED

SYLLABUS: OPERATING SYSTEMS 
SYLLABUS CODE: CS2006

OPERATING SYSTEMS - ASSIGNMENT NO#02
TA: SIR ALI MUBIN
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define runways 2
#define aging 4
#define sim_time 45
#define queue_size 100

#define cargo 0
#define takeoff 1
#define landing 2
#define fuel_low 3
#define emergency 4

typedef struct{
   int id;
   int type;
   time_t arrivalTime;
   int dynamicboost;
}flight;

typedef struct{
   int id;
   int busy;
   int flag;
}runway;

flight queue[queue_size];
int q_size = 0;
runway rw[runways];

pthread_mutex_t queueM = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueC = PTHREAD_COND_INITIALIZER;
pthread_mutex_t runwayM = PTHREAD_MUTEX_INITIALIZER;

int total_priority(flight*f){
   int baseP =0;
   if(f->type == emergency){
       baseP = 10000;
   }else if (f->type == fuel_low){
       baseP = 800;
   }else if(f->type == landing){
       baseP = 600;
   }else if(f->type == takeoff){
       baseP= 400;
   }else if(f->type== cargo){
       baseP = 200;
   }

   return baseP + f->dynamicboost;
}

char* flight_name(int type){
   if(type == emergency){
       return "emergency";
   }else if( type == fuel_low){
       return "fuel_low";
   }else if (type == landing){
       return "landing";
   }else if(type == cargo){
       return "cargo";
   }else if(type == takeoff){
       return "takeoff";
   }
}

void* gen_flight(void*arg){
   int counter = 1;
   while(1){
       sleep(rand()%3+1);
       flight newflight;
       newflight.id = counter++;

       int r = rand()%100;
       if(r<5){
           newflight.type = emergency;
       }else if(r<15){
           newflight.type = fuel_low;
       }else if(r<50){
           newflight.type = landing;
       }else if(r<85){
           newflight.type=takeoff;
       }else{
           newflight.type= cargo;
       }

       newflight.arrivalTime = time(NULL);
       newflight.dynamicboost = 0;

       pthread_mutex_lock(&queueM);
       if(q_size<queue_size){
           queue[q_size++] = newflight;
           printf("new flight %d of type %s added to queue\n", newflight.id, flight_name(newflight.type));
           pthread_cond_signal(&queueC);
       }else{
           printf("queue is full %d flight rejected\n", newflight.id);
       }
       pthread_mutex_unlock(&queueM);
   }
   return NULL;
}

void * controlRunway(void*arg){
   int id = *((int*)arg);

   while(1){
       pthread_mutex_lock(&queueM);

       while(q_size==0){
           pthread_cond_wait(&queueC, &queueM);
       }

       if(rand()%100 <10){
           printf("runway %d is gettign maintaied\n", id);
           pthread_mutex_unlock(&queueM);
           sleep(3);
           continue;
       }

       int index = 0;
       int priority = -1;

       for(int i=0; i<q_size; i++){
           int p = total_priority(&queue[i]);
           if(p>priority){
               priority = p;
               index = i;
           }
       }

       flight currflight = queue[index];
       for(int i=index; i<q_size-1; i++){
           queue[i] = queue[i+1];
       }
       q_size--;
       pthread_mutex_unlock(&queueM);

       pthread_mutex_lock(&runwayM);
       rw[id].busy = 1;
       rw[id].flag = 0;
       pthread_mutex_unlock(&runwayM);
      
       printf("runway %d started for flight %d %s\n",id, currflight.id ,flight_name(currflight.type));
      
       int aborted =0;
       for(int i=0; i<4; i++){
           sleep(1);
           pthread_mutex_lock(&runwayM);
           if(rw[id].flag){
               aborted = 1;
               pthread_mutex_unlock(&runwayM);
               break;
           }
           pthread_mutex_unlock(&runwayM);
       }
       pthread_mutex_lock(&runwayM);
       rw[id].busy = 0;
       pthread_mutex_unlock(&runwayM);
       if(aborted){
           printf("runway %d: aborted fligh %d due to emergency\n", id, currflight.id);
           pthread_mutex_lock(&queueM);
           if(q_size < queue_size){
               currflight.arrivalTime = time(NULL);
               queue[q_size++] = currflight;
               pthread_cond_signal(&queueC);
           }
           pthread_mutex_unlock(&queueM);
       }else{
           printf("runway %d:completed flight %d\n", id, currflight.id);

       }

   }
   return NULL;
}

void * emergency_monitor(void*arg){
   while(1){
       sleep(2);
       time_t currtime = time(NULL);
       int emergency_found = 0;
       pthread_mutex_lock(&queueM);

       for (int i=0; i<q_size;i++){
           double wait = difftime(currtime, queue[i].arrivalTime);
           if(wait> aging && queue[i].type != emergency){
               queue[i].dynamicboost += 50;
           }
           if(queue[i].type == emergency){
               emergency_found=1;
           }
       }
       pthread_mutex_unlock(&queueM);

       if(emergency_found){
           pthread_mutex_lock(&runwayM);
           int busy=1;
           int victim=-1;
      
           for(int i=0;i<runways; i++){
               if(rw[i].busy==0){
                   busy = 0;
                   break;
               }
               victim=i;
           }
          
           if(busy && victim !=-1){
               rw[victim].flag=1;
               printf("emergency detected sending preeemption signal to runway %d\n",victim);

           }
           pthread_mutex_unlock(&runwayM);
       }

   }
   return NULL;
}

void make_flight(int id, int type){
   pthread_mutex_lock(&queueM);
   if(q_size <queue_size){
       queue[q_size].id = id;
       queue[q_size].type = type;
       queue[q_size].arrivalTime = time(NULL);
       queue[q_size].dynamicboost = 0;
       printf("flight %d %s requires a runway\n", id, flight_name(type));
       q_size++;
       pthread_cond_signal(&queueC);
   }
   pthread_mutex_unlock(&queueM);
}

int main(){
   for(int i=0; i<runways; i++){
       rw[i].id=i;
       rw[i].busy=0;
       rw[i].flag=0;
   }

   pthread_t monitor_thread;
   pthread_t runway_thread[runways];
   int ids[runways];

   pthread_create(&monitor_thread,NULL, emergency_monitor, NULL);
   for(int i = 0; i < runways; i++){
       ids[i] = i;
       pthread_create(&runway_thread[i], NULL, controlRunway,&ids[i]);
   }

   usleep(100000);

   printf("TESTING PRIORITY QUEUE\n");

   make_flight(991, cargo);
   make_flight(992, cargo);
   sleep(1);

   make_flight(101,cargo);
   make_flight(102,takeoff);
   make_flight(103,landing);

   sleep(15);

   printf("TESTING AGING\n");

   make_flight(993, cargo);
   make_flight(994, cargo);
   sleep(1);

   make_flight(201,cargo);
   sleep(6);

   make_flight(202,takeoff);
   sleep(15);

   printf("TESTING PREEMPTION\n");
   make_flight(301,cargo);
   make_flight(302,cargo);
   sleep(1);
  
   make_flight(911,emergency);
   sleep(10);
  
   return 0;

}
