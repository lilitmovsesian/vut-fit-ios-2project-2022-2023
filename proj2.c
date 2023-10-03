//Lilit Movsesian
//xmovse00
//29.03.2023
//2. project IOS

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include<time.h>
#include<pthread.h>

typedef struct args {
    int NZ; //number of customers
    int NU; //number of clerks
    int TZ; //max time [ms] before entering
    int TU; //max time [ms] of break
    int F; //max time [ms] of working hours
} args_t;

int shm_init();
int semaphores_init();
int clean();
void print_action(int cnt_action, char actor, int id, char *action, int *req_t);
int random_range(int min, int max, int id);
void customer(args_t *args, int id);
void clerk(args_t *args, int id);
void serving_a_request(int request_type, int id);
int is_closed_and_served(int NZ);


FILE *proj2_log;

sem_t *customers_num_letter = NULL;
sem_t *customers_num_parcel = NULL;
sem_t *customers_num_money = NULL;
sem_t *output = NULL;
sem_t *mutex = NULL;
sem_t *service_accepted = NULL;
sem_t *current_service_letter = NULL;
sem_t *current_service_parcel = NULL;
sem_t *current_service_money = NULL;

int *post_office_open = NULL;
int *counter_of_actions = NULL;
int *counter_of_happy_clients = NULL;
int *counter_of_clients_left = NULL;

//Initialization of shared memory.
int shm_init(){
    if((customers_num_letter = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (customers_num_parcel = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (customers_num_money = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (output = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (service_accepted = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (current_service_letter = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (current_service_parcel = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (current_service_money = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED ||
    (post_office_open = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
    (counter_of_actions = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
    (counter_of_clients_left = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
    (counter_of_happy_clients = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
        return 1;
    return 0;
}

//Semaphore initialization.
int semaphores_init(){
    if((sem_init(customers_num_letter, 1, 0)) == -1 || 
    (sem_init(customers_num_parcel, 1, 0)) == -1 || 
    (sem_init(customers_num_money, 1, 0)) == -1 || 
    (sem_init(output, 1, 1)) == -1 ||
    (sem_init(mutex, 1, 1)) == -1 || 
    (sem_init(service_accepted, 1, 0)) == -1 ||
    (sem_init(current_service_letter, 1, 0)) == -1 || 
    (sem_init(current_service_parcel, 1, 0)) == -1 || 
    (sem_init(current_service_money, 1, 0)) == -1)
        return 1;
    return 0;
}

//Semaphore destruction and memory unmapping.
int clean(){
    if((munmap(customers_num_letter, sizeof(sem_t))) == -1 ||
    (munmap(customers_num_parcel, sizeof(sem_t))) == -1 ||
    (munmap(customers_num_money, sizeof(sem_t))) == -1 ||
    (munmap(output, sizeof(sem_t))) == -1 ||
    (munmap(mutex, sizeof(sem_t))) == -1 ||
    (munmap(service_accepted, sizeof(sem_t))) == -1 ||
    (munmap(current_service_letter, sizeof(sem_t))) == -1 ||
    (munmap(current_service_parcel, sizeof(sem_t))) == -1 ||
    (munmap(current_service_money, sizeof(sem_t))) == -1 ||
    (munmap(post_office_open, sizeof(int))) == -1 ||
    (munmap(counter_of_actions, sizeof(int))) == -1 ||
    (munmap(counter_of_clients_left, sizeof(int))) == -1 ||
    (munmap(counter_of_happy_clients, sizeof(int))) == -1 ||
    (sem_destroy(customers_num_letter)) == -1 ||
    (sem_destroy(customers_num_parcel)) == -1 || 
    (sem_destroy(customers_num_money)) == -1 || 
    (sem_destroy(output)) == -1 ||
    (sem_destroy(mutex)) == -1 || 
    (sem_destroy(service_accepted)) == -1 ||
    (sem_destroy(current_service_letter)) == -1 || 
    (sem_destroy(current_service_parcel)) == -1 ||
    (sem_destroy(current_service_money)) == -1)
        return 1;
    return 0;
}


//A function to print actions to the log file.
void print_action(int cnt_action, char actor, int id, char *action, int *req_t){
    if ((proj2_log=fopen("proj2.out", "a+")) == NULL){
        fprintf(stderr, "Error: Failed to open log file.\n");
        clean();
        exit(EXIT_FAILURE);
    }
    if (req_t)
    	fprintf(proj2_log,"%d: %c %d: %s %d\n", cnt_action, actor, id, action, *req_t);
    else
    	fprintf(proj2_log,"%d: %c %d: %s\n", cnt_action, actor, id, action);
    fflush(proj2_log);
    fclose(proj2_log);
}

//A randomizer for a number between min and max.
int random_range(int min, int max, int id){
    srand(time(NULL)+id);
    return (rand() % (max - min + 1)) + min;
}

//Customer process
void customer(args_t *args, int id){
        
    int request_type;
    
    sem_wait(output);
    print_action((*counter_of_actions)++,'Z', id, "started", NULL);
    sem_post(output);

    usleep(rand() % (args->TZ +1 )*1000);
    
    //Going home if post office is closed or there are no clerks.
    sem_wait(mutex);
    if (*post_office_open == 0 || args->NU == 0){
    
        sem_wait(output);
        print_action((*counter_of_actions)++,'Z', id, "going home", NULL);
        sem_post(output);
        
        *counter_of_clients_left += 1;
        sem_post(mutex); 
        exit(EXIT_SUCCESS);
    }
    
    //Enters the post office, randomly chooses service type. I use 
    //quite a lot of semaphores to make sure the queue service works properly
    //and every customer thread is synchronized with correct clerk service type.
    //I use several counters for customers' status to ensure the correct 
    //termination of clerk loop.
    else{
    
        request_type = random_range(1,3, id);
        if (request_type==1)
            sem_post(customers_num_letter);
        else if (request_type==2)
            sem_post(customers_num_parcel);
        else if (request_type==3)
            sem_post(customers_num_money);
        
        sem_wait(output);
        print_action((*counter_of_actions)++,'Z', id, "entering office for a service", &request_type);
        sem_post(output);
        sem_post(mutex); 
       
        if (request_type==1)
            sem_wait(current_service_letter);
        else if (request_type==2)
            sem_wait(current_service_parcel);
        else if (request_type==3)
            sem_wait(current_service_money);
          
        sem_wait(output);
        print_action((*counter_of_actions)++,'Z', id, "called by office worker", NULL);
        sem_post(output);
    	
        sem_wait(mutex);
        sem_post(service_accepted);
        sem_post(mutex);
        
        usleep(random_range(0, 10, id));
        
        sem_wait(output);
        print_action((*counter_of_actions)++,'Z', id, "going home", NULL);
        sem_post(output);
        
        *counter_of_happy_clients += 1;
        exit(EXIT_SUCCESS);
    }
}

//I have decided to make a separate function for serving a request 
//to make clerk process loop neater. 
void serving_a_request(int request_type, int id){
    if (request_type == 1){
        
        sem_wait(customers_num_letter);     
        
        sem_wait(mutex);
        sem_post(current_service_letter);
        sem_post(mutex);
            
    }
    else if (request_type == 2){
        
        sem_wait(customers_num_parcel);
        
        sem_wait(mutex);
        sem_post(current_service_parcel);
        sem_post(mutex);
        
    }
    else if (request_type == 3){
        
        sem_wait(customers_num_money);
        
        sem_wait(mutex);  
        sem_post(current_service_money);
        sem_post(mutex);
           
    }
    
    sem_wait(service_accepted);
    
    sem_wait(output);
    print_action((*counter_of_actions)++, 'U', id, "serving a service of type", &request_type);
    sem_post(output);
            
    usleep(random_range(0, 10, id));
            
    sem_wait(output);
    print_action((*counter_of_actions)++, 'U', id, "service finished", NULL);
    sem_post(output);

}

//A function for loop termination in clerk process, which checks if all people 
//who entered to the post office are served.
int is_closed_and_served(int NZ){
    if ((*counter_of_happy_clients == (NZ - *counter_of_clients_left)) && *post_office_open == 0){
        return 1;
    }
    else{ 
        return 0;
    }
}
void clerk(args_t *args, int id){

    int request_type;
    int letter=0;
    int parcel=0;
    int money=0;
    
    sem_wait(output);
    print_action((*counter_of_actions)++,'U', id, "started", NULL);
    sem_post(output);
    
    //If statements to check if there are people waiting in three queues and 
    //randomly select a non-empty queue.
       while (is_closed_and_served(args->NZ) == 0){
        sem_getvalue(customers_num_letter, &letter); 
        sem_getvalue(customers_num_parcel, &parcel); 
        sem_getvalue(customers_num_money, &money);
        
        if (letter > 0 && parcel > 0 && money > 0){
            request_type = random_range(1, 3, id);
            serving_a_request(request_type, id);   
        }
        else if (letter > 0 && parcel > 0 && money == 0){
            request_type = random_range(1, 2, id);
            serving_a_request(request_type, id);
        }
        else if (letter > 0 && parcel == 0 && money > 0){
            request_type = random_range(1, 2, id);
            if (request_type == 2){
                request_type = 3;
            }
            serving_a_request(request_type, id);
        }
        else if (letter == 0 && parcel > 0 && money > 0){
            request_type = random_range(2, 3, id);
            serving_a_request(request_type, id);
        }
        else if (letter == 0 && parcel == 0 && money > 0){
            request_type = 3;
            serving_a_request(request_type, id);
        }
        else if (letter == 0 && parcel > 0 && money == 0){
            request_type = 2;
            serving_a_request(request_type, id);
        }
        else if (letter > 0 && parcel == 0 && money == 0){
            request_type = 1;
            serving_a_request(request_type, id);
        }
        //Clerk is taking a break if nobody is in the queues.
        else{
            sem_wait(output);
            print_action((*counter_of_actions)++,'U', id, "taking break", NULL);
            sem_post(output);
            
            usleep(rand() % (args->TU + 1)*1000);
            
            sem_wait(output);
            print_action((*counter_of_actions)++,'U', id, "break finished", NULL);
            sem_post(output);
        }
    }
    sem_wait(output);
    print_action((*counter_of_actions)++,'U', id, "going home", NULL);
    sem_post(output);        
     
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    
    if (argc!=6){
        fprintf(stderr, "Usage: %s NZ NU TZ TU F.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
     
    args_t args;
    int wait_time;
    
    if ((proj2_log=fopen("proj2.out", "w+")) == NULL){
        fprintf(stderr, "Error: Failed to open log file.\n");
        exit(EXIT_FAILURE);
    }
    
    //Arguments parsing and if statement for arguments format validation.
    
    args.NZ = atoi(argv[1]);
    args.NU = atoi(argv[2]);
    args.TZ = atoi(argv[3]);
    args.TU = atoi(argv[4]);
    args.F = atoi(argv[5]);

    if (args.NZ < 0 || args.NU < 0 || args.TZ < 0 || args.TZ > 10000 || args.TU < 0 || args.TU > 100 || args.F < 0 || args.F > 10000) {
        fprintf(stderr, "Error: Invalid input values.\n");
        exit(EXIT_FAILURE);
    }

    //Shared memory and semaphore creation.
    if (shm_init()){
        fprintf(stderr, "Error: Failed to create shared memory.\n");
        clean();
        exit(EXIT_FAILURE);
    }
    if (semaphores_init()){
        fprintf(stderr, "Error: Failed to create a semaphore.\n");
        clean();
        exit(EXIT_FAILURE);
    }

    *post_office_open = 1;
    *counter_of_actions = 1;
    *counter_of_clients_left = 0;
    *counter_of_happy_clients = 0;

    //Memory allocation and fork creation for the processes threads.
    
    pid_t* clerk_pids = malloc(args.NU * sizeof(pid_t));
    if (clerk_pids == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for clerk_pids.\n");
        clean();
        exit(EXIT_FAILURE);
    }

    pid_t* customer_pids = malloc(args.NZ * sizeof(pid_t));
    if (customer_pids == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for customer_pids.\n");
        clean();
        free(clerk_pids);
        exit(EXIT_FAILURE);
    }
     
    for (int i = 0; i < args.NU; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error: Clerk process fork failed.\n");
            clean();
            free(clerk_pids);
            free(customer_pids);
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            clerk(&args, i+1);
            exit(EXIT_SUCCESS);
        }
        clerk_pids[i] = pid;
    }

    for (int i = 0; i < args.NZ; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error: Customer process fork failed.\n");
            clean();
            free(clerk_pids);
            free(customer_pids);
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            customer(&args, i+1);
            exit(EXIT_SUCCESS);
        }
        customer_pids[i] = pid;
    }

    
    //Waiting before the closing.
    wait_time=random_range((args.F)/2, args.F, 0);
    usleep(wait_time*1000);
    
    sem_wait(mutex);
    *post_office_open = 0;
        
    sem_wait(output);
    if ((proj2_log=fopen("proj2.out", "a+")) == NULL){
        fprintf(stderr, "Error: Failed to open log file.\n");
        clean();
        free(clerk_pids);
        free(customer_pids);
        exit(EXIT_FAILURE);
    }
    fprintf(proj2_log,"%d: %s\n", (*counter_of_actions)++, "closing");
    fflush(proj2_log);
    fclose(proj2_log);
    sem_post(output);
    sem_post(mutex);

    // Wait for child processes to finish
    for (int i = 0; i < args.NU; i++) {
        waitpid(clerk_pids[i], NULL, 0);
    }
    for (int i = 0; i < args.NZ; i++) {
        waitpid(customer_pids[i], NULL, 0);
    }
    
    free(clerk_pids);
    free(customer_pids);
    
    //Destuction of semaphores and memory unmapping.
    if (clean()){
        fprintf(stderr, "Error: Failed to destroy a semaphore or unmap shared memory.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
