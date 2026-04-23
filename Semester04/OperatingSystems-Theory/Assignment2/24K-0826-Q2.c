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
#include <math.h>

#define n 1000
#define m 5         
#define tile_size 100
#define num_of_tiles ((n / tile_size) * (n / tile_size))
#define hot 35.0
#define cold -10.0
#define threads 8

double satellites[m][n][n];
double matrix[n][n];
double normalize_matrix[n][n];
double risk[n][n];

int h[n][n] = {0};
int c[n][n] = {0};

double maximum = -1000.0;
double minimum = 1000.0;
double sum  = 0.0;
double variance_sum = 0.0;
int anomalies = 0, hotspot = 0, coldspot=0;

pthread_mutex_t statsM = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueM = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t b_cond = PTHREAD_COND_INITIALIZER;
int b_count = 0;
int b_generation = 0;
int next = 0;

void barrier_wait(){
   pthread_mutex_lock(&b_mutex);
   int current_gen = b_generation;
   b_count++;
   if(b_count == 3){
       b_generation++;
       b_count = 0;  
       pthread_cond_broadcast(&b_cond);
   }else{
       while(current_gen == b_generation){
           pthread_cond_wait(&b_cond, &b_mutex);
       }
   }
   pthread_mutex_unlock(&b_mutex);
}


void* prep(void*arg){
   int id = *((int*)arg);
   for(int i=0;i<n;i++){
       for(int j=0; j<n; j++){
           if(isnan(satellites[id][i][j])){
               double left, right;
               if(j>0){
                    left = satellites[id][i][j-1];
               }else{
                    left = 0;
               }

               if(j<n-1 && !isnan(satellites[id][i][j+1])){
                    right = satellites[id][i][j+1];
               }else{
                    right = 0;
               }

               satellites[id][i][j] = (left+right)/2.0;

           }
       }
   }
   pthread_exit(NULL);
}

void merge(){
   for(int i=0; i<n; i++){
       for(int j=0; j<n;j++){
           double sum=0;
           for(int s=0; s<m; s++){
               sum+=satellites[s][i][j];
           }
           matrix[i][j] = sum/m;
       }
   }
}

void* region_stats(void*arg){
   while(1){
       int id;
       pthread_mutex_lock(&queueM);

       if(next>=num_of_tiles){
           pthread_mutex_unlock(&queueM);
           break;
       }

       id = next;
       next++;
       pthread_mutex_unlock(&queueM);

       int row_tiles = n/tile_size;
       int row = (id/row_tiles)*tile_size;
       int col = (id%row_tiles)*tile_size;

       double max = -1000.0, min=1000.0, lsum=0,l_var_sum = 0;
       int lanomalies = 0;

       for(int i=row; i<row+tile_size; i++){
           for(int j=col; j<col+tile_size; j++){
               double val = matrix[i][j];
               if(val>max){
                   max = val;
               }

               if(val<min){
                   min=val;
               }

               lsum +=val;
           }
       }
       double mean = lsum/(tile_size*tile_size);

       for(int i =row; i < row + tile_size; i++){
           for (int j = col;j < col + tile_size; j++){
               double diff =matrix[i][j] - mean;
               l_var_sum+= (diff * diff);
           }
       }

       double stddev = sqrt(l_var_sum / (tile_size * tile_size));

       for(int i = row; i < row + tile_size; i++){
           for(int j = col; j < col + tile_size; j++){
               if(fabs(matrix[i][j] - mean)> 2 * stddev){
                   lanomalies ++;
               }
           }
       }

       pthread_mutex_lock(&statsM);
       if(max > maximum){
           maximum = max;
       }
       if(min < minimum){
           minimum = min;
       }
       sum += lsum;
       variance_sum += l_var_sum;
       anomalies += lanomalies;
       pthread_mutex_unlock(&statsM);
   }
   pthread_exit(NULL);
}

void* hotspots(void*arg) {
   for(int i= 0; i <n; i++){
       for(int j = 0;j < n; j++){
           if (matrix[i][j] > hot) {
               h[i][j] = 1;
               pthread_mutex_lock(&statsM);
               hotspot++;
               pthread_mutex_unlock(&statsM);
           }
       }
      
       barrier_wait();

   }
   pthread_exit(NULL);
}

void* coldspots(void*arg) {
   for(int i = 0; i < n; i++){
       for(int j = 0; j < n; j++){
           if(matrix[i][j] <cold){
               c[i][j] = 1;
               pthread_mutex_lock(&statsM);
               coldspot++;
               pthread_mutex_unlock(&statsM);
           }
       }
       barrier_wait();
   }
   pthread_exit(NULL);
}

void* normalization(void*arg) {
   double range = maximum-minimum;
   if(range == 0){
       range = 1;
   }

   for(int i= 0; i < n; i++){
       barrier_wait();
      
       for (int j =0; j <n; j++) {
           double norm = (matrix[i][j] - minimum) / range;
           int near_hot=0, near_cold=0;

           if(i >0 && h[i-1][j] || j > 0 &&  h[i][j-1]){
               near_hot = 1;
           }
           if(i > 0 && c[i-1][j] || j > 0 && c[i][j-1]){
               near_cold=1;
           }

           if(near_hot && near_cold){
               norm *= 1.2;
           }
           normalize_matrix[i][j] = norm;
       }
   }
   pthread_exit(NULL);
}

void* calc_risk(void* arg) {
   int id = *((int*)arg);
   int rows_thread = n / threads;
   int rowS = id * rows_thread;
   int rowE;
   if(id == threads - 1){
       rowE= n;
   }else{
       rowE=rowS+rows_thread;
   }

   for(int i = rowS; i < rowE; i++){
       for(int j = 0; j < n; j++){
           int prox_hot, prox_cold;
          
           if(c[i][j]){
               prox_hot =1;
           }else{
               prox_hot=0;
           }

           if(c[i][j]){
               prox_cold =1;
           }else{
               prox_cold=0;
           }

           risk[i][j] = (normalize_matrix[i][j] * prox_hot) / (prox_cold + 1.0);
       }
   }
   pthread_exit(NULL);
}


void initialize(){
   for(int s = 0; s < m; s++){
       for(int i = 0;i < n; i++){
           for(int j = 0; j < n; j++){
               double temp =(rand() %7000) /100.0- 20.0;
              
               if(rand() % 100 <5){
                   satellites[s][i][j] = NAN;
               }else {
                   satellites[s][i][j] = temp;
               }
           }
       }
   }
}

int main() {
   initialize();
  
   pthread_t thread[m];
   int ids[m];

   for(int i = 0; i < m; i++){
       ids[i] = i;
       pthread_create(&thread[i], NULL, prep, &ids[i]);
   }
   for(int i = 0; i < m;i++){
       pthread_join(thread[i], NULL);
   }

   merge();

   pthread_t pool[threads];
   for(int i= 0; i < threads; i++){
       pthread_create(&pool[i], NULL, region_stats, NULL);
   }
   for (int i = 0; i < threads; i++){
       pthread_join(pool[i], NULL);
   }

   pthread_t t_A, t_B, t_C;

   pthread_create(&t_A, NULL,hotspots,NULL);
   pthread_create(&t_B, NULL,coldspots, NULL);
   pthread_create(&t_C,NULL,normalization, NULL);

   pthread_join(t_A, NULL);
   pthread_join(t_B, NULL);
   pthread_join(t_C, NULL);
  
   int thread_ids[threads];
   for(int i = 0; i < threads; i++){
       thread_ids[i] = i;
       pthread_create(&pool[i], NULL, calc_risk,&thread_ids[i]);
   }
   for(int i = 0; i < threads; i++){
       pthread_join(pool[i], NULL);
   }

   printf("Global Maximum: %.2f, Global Minimum: %.2f\n", maximum, minimum);
   printf("Global Mean: %.2f\n", sum /(n *n));
   printf("Hotspots: %d, Coldspots: %d, Anomalies: %d\n", hotspot,coldspot, anomalies);

   return 0;
