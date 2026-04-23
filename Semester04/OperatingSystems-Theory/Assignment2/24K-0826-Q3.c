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

#define max_patients 100
#define seniors 2
#define juniors 2

typedef enum {critical, serious, normal} priority;

typedef struct{
   int id;
   priority type;
   time_t arrival_time;
   int is_treated;
   pthread_cond_t cv;
} patient;

typedef struct{
   int id;
   priority type;
} patient_arg;


patient p[max_patients];
int patient_count = 0;

pthread_mutex_t hospital = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_cv = PTHREAD_COND_INITIALIZER;

int doctor_consective_normal[seniors+juniors]= {0};

int oldest_patient(priority type){
   int index = -1;
   time_t mintime = time(NULL)+1000;

   for(int i=0; i<patient_count; i++){
       if(p[i].is_treated == 0 && p[i].type == type){
           if(p[i].arrival_time < mintime){
               mintime = p[i].arrival_time;
               index = i;
           }
       }
   }

   return index;
}

void* monitor(void*arg){
   while(1){
       sleep(1);
       pthread_mutex_lock(&hospital);

       time_t curr_time = time(NULL);
       int serious_cnt = 0;

       for(int i=0; i<patient_count; i++){
           if(p[i].is_treated == 0){
               if(curr_time -p[i].arrival_time >=30 && p[i].type != critical){
                   printf("patient %d waited for to long and promoted to critical\n", p[i].id);
                   p[i].type = critical;
                   pthread_cond_broadcast(&doctor_cv);
               }

               if(p[i].type == serious){
                   serious_cnt++;
               }
           }

       }

       if(serious_cnt >=5){
           int oldest = oldest_patient(serious);
           if(oldest != -1){
               printf("5 serious patients waiting therefore promoting oldest %d to critical\n", p[oldest].id);
               p[oldest].type = critical;
               pthread_cond_broadcast(&doctor_cv);
           }
       }
       pthread_mutex_unlock(&hospital);
   }
   return NULL;
}

void * doctor(void*arg){
   int id = *((int*)arg);
   int is_senior = (id<seniors);
   char * role;
   if(is_senior){
       role = "senior";
   }else{
       role = "junior";
   }

   while(1){
       pthread_mutex_lock(&hospital);
       int trg_patient = -1;

       while(1){
           if(doctor_consective_normal[id]>=3){
               trg_patient  =oldest_patient(serious);
               if(trg_patient == -1 && is_senior){
                   trg_patient = oldest_patient(critical);
               }
           }else{
               if(is_senior){
                   trg_patient = oldest_patient(critical);
               }
               if(trg_patient==-1){
                   trg_patient = oldest_patient(serious);
               }
               if(trg_patient==-1){
                   trg_patient = oldest_patient(normal);
               }
           }

           if(trg_patient!=-1){
               break;
           }
           pthread_cond_wait(&doctor_cv, &hospital);
       }
       p[trg_patient].is_treated=1;
       priority p_type=p[trg_patient].type;
       if(p_type==normal){
           doctor_consective_normal[id]++;
       }else{
           doctor_consective_normal[id]=0;
       }

       pthread_mutex_unlock(&hospital);

       printf("%s doctor %d threating patient %d type: %d, consective normals: %d\n", role, id, p[trg_patient].id, p_type, doctor_consective_normal[id]);
       sleep(2);

       pthread_mutex_lock(&hospital);
       pthread_cond_signal(&p[trg_patient].cv);
       pthread_mutex_unlock(&hospital);
   }

   return NULL;
}


void* thread_patient(void*arg){
   patient_arg *p_arg = (patient_arg*)arg
;    int p_id = p_arg->id;
   pthread_mutex_lock(&hospital);

   int index = patient_count++;
   p[index].id = p_id;
   p[index].type= p_arg->type;;
   p[index].arrival_time= time(NULL);
   p[index].is_treated =0;
   pthread_cond_init(&p[index].cv, NULL);

   char* t_str;

   if(p[index].type == critical){
       t_str = "critical";
   }else if(p[index].type == serious){
       t_str = "serious";
   }else if(p[index].type == normal){
       t_str = "normal";
   }

   printf("patient %d arrived as %s\n",p_id,t_str);

   pthread_cond_broadcast(&doctor_cv);
   while(p[index].is_treated == 0){
       pthread_cond_wait(&p[index].cv,&hospital);
   }

   printf("patient %d treatment finished\n",p_id);
   pthread_mutex_unlock(&hospital);
   return NULL;
}

void gen_patient(pthread_t *thread, patient_arg *arg, int id, priority ptype){
   arg->id = id;
   arg->type = ptype;
   pthread_create(thread,NULL, thread_patient, arg);
   usleep(100000);
}

int main(){

   pthread_t monitor_thread;
   pthread_create(&monitor_thread, NULL, monitor, NULL);

   int total_docs = seniors+juniors;
   pthread_t doctor_threads[total_docs];
   int doctor_id[total_docs];

   for(int i=0; i<total_docs; i++){
       doctor_id[i] = i;
       pthread_create(&doctor_threads[i], NULL, doctor, &doctor_id[i]);
   }

   pthread_t patients_threads[max_patients];
   patient_arg p_arg[max_patients];

   int pid = 1;

   printf("TESTING PRIORITY AND JUNIOR CONSTRAINTS\n");

   gen_patient(&patients_threads[pid], &p_arg[pid],pid,critical);
   pid++;
   gen_patient(&patients_threads[pid], &p_arg[pid],pid,serious);
   pid++;
  
   sleep(4);

   printf("TESTING THE 5-SERIOUS PROMOTION REQUIRMENT \n");

   for(int i=0; i<4; i++){
       gen_patient(&patients_threads[pid], &p_arg[pid], pid, normal);
       pid++;
   }

   for(int i=0; i<5; i++){
       gen_patient(&patients_threads[pid], &p_arg[pid], pid, serious);
       pid++;
   }

   sleep(4);

   printf("TESTING DOCTOR FATIGUE REQUIRMENT\n");
   for (int i=0; i<12; i++){
       gen_patient(&patients_threads[pid], &p_arg[pid], pid, normal);
       pid++;
   }

   gen_patient(&patients_threads[pid], &p_arg[pid], pid, normal);
   pid++;
   gen_patient(&patients_threads[pid], &p_arg[pid], pid, normal);
   pid++;
   gen_patient(&patients_threads[pid], &p_arg[pid], pid, serious);
   pid++;
   gen_patient(&patients_threads[pid], &p_arg[pid], pid, serious);
   pid++;
  
   sleep(8);

   printf("TESTIGN THE 30 SEC WAIT LIMIT REQUIRMENT\n");

   int starve_patient = pid;
   gen_patient(&patients_threads[pid], &p_arg[pid], pid, normal);
   pid++;

   for(int i=0; i<19; i++){
       gen_patient(&patients_threads[pid], &p_arg[pid], pid, critical);
       pid++;
       sleep(2);
   }

   for(int i=1; i<pid; i++){
       pthread_join(patients_threads[i],NULL);
   }
   return 0;
}

