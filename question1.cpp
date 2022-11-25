#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <semaphore.h>
#include "logger.h"
using namespace std;

pthread_barrier_t water_barrier;
pthread_mutex_t oxygen_mutex;
pthread_mutex_t positionMutex;
pthread_mutex_t hydrogen_mutex;
sem_t *hydrogen_sem;
sem_t *thresold;
unsigned int num_of_water_molecules, positions;
vector<bool> status;
int oxygen, hydrogen;

void makeBond(int molNo){

    sem_wait(thresold);
    int selectedPos;
    printf("bond making\n");
    Logger::Info("bond making..");
    while(true){
        // printf("here\n");
        selectedPos = rand()%positions;
        if(selectedPos == 0){
            if(status[0] == 0 && status[1] == 0) break;
        }
        else if(selectedPos == positions - 1){
            if(status[positions-1] == 0 && status[positions-2] == 0) break;
        }
        else {
            int nextPos = selectedPos+1;
            int prevPos = selectedPos-1;
            if(status[prevPos] == 0 && status[nextPos] == 0) break;
        }
    }
    
    // printf("bbbb1\n");

    // modifiy status of the position
    pthread_mutex_lock(&positionMutex);
    status[selectedPos] = 1;
    pthread_mutex_unlock(&positionMutex);
    sleep(1);
    // num_of_water_molecules++;
    
    printf("bond completed.\n");
    printf("molecule no: %d, created at spot: %d. \n-------------------------\n",num_of_water_molecules, selectedPos);
    Logger::Info("bond completed. \nmolecule no:"+to_string(molNo)+" created at spot: "+to_string(selectedPos)+".\n-------------------------\n");

    pthread_mutex_lock(&positionMutex);
    status[selectedPos] = 0;
    pthread_mutex_unlock(&positionMutex);
    sem_post(thresold);
    
}

void *hydrogen_thread_body(void *arg)
{
    
    // sem_wait(hydrogen_sem);
    // pthread_barrier_wait(&water_barrier);
    // sem_post(hydrogen_sem);

    pthread_mutex_lock(&oxygen_mutex);
    pthread_mutex_lock(&hydrogen_mutex);
    // pthread_barrier_wait(&water_barrier);
    hydrogen += 1; // apply lock here
    if (hydrogen >= 2 && oxygen >= 1)
    {
        hydrogen -= 2;
        // sem_signal(oxyQueue);
        oxygen -= 1;
        // num_of_water_molecules++;
        int moLNo = ++num_of_water_molecules;
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
        makeBond(moLNo); 
    }
    else{
        // bond making not possible so release locks
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
    }
    return NULL;
}
void *oxygen_thread_body(void *arg)
{
    pthread_mutex_lock(&oxygen_mutex);
    pthread_mutex_lock(&hydrogen_mutex);
    // pthread_barrier_wait(&water_barrier);
    oxygen += 1; // apply lock here
    if (hydrogen >= 2) // here also
    {   
        // call bond function here
        // sem_signal(hydroQueue);
        // sem_signal(hydroQueue);
        hydrogen -= 2;
        // sem_signal(oxyQueue);
        oxygen -= 1;
        int moLNo = ++num_of_water_molecules;
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
        makeBond(moLNo); 
    }
    else{
        // bond making not possible so release locks
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
    }
    // release lock
    // cout<<"    OQ: "<<oxyQueue->value<<endl;
    // cout<<"LOCKING => OQ"<<endl;
    // sem_wait(oxyQueue);
    // cout<<"DONE => OQ"<<endl;
    // cout<<"    OQ: "<<oxyQueue->value<<endl;
    // printf("Oxygen Bond\n");
    
    // cout<<"-----------"<<endl;
    return NULL;
}
int main(int argc, char **argv)
{   
    int hydatoms = atoi(argv[1]);
    int oxyatoms = atoi(argv[2]);
    num_of_water_molecules = 0;
    positions = atoi(argv[3]);
    int thsrd = atoi(argv[4]);
    pthread_mutex_init(&oxygen_mutex, NULL);
    pthread_mutex_init(&positionMutex, NULL);
    sem_t local_sem1, local_sem2;
    hydrogen_sem = &local_sem1;
    thresold = &local_sem2;
    sem_init(hydrogen_sem, 0, 2);
    sem_init(thresold, 0, thsrd);
    status.resize(positions, 0);
    srand(time(0));
    pthread_barrier_init(&water_barrier, NULL, 3);
    ofstream log_file("logFile.txt", ios_base::out | ios_base::trunc );

    // For creating 50 water molecules, we need 50 oxygen atoms and
    // 100 hydrogen atoms
    pthread_t thread[oxyatoms+hydatoms];
    // Create oxygen threads
    for (int i = 0; i < oxyatoms; i++)
    {   
        printf("thO: %d",i);
        if (pthread_create(thread + i, NULL, oxygen_thread_body, NULL))
        {
            printf("Couldn’t create an oxygen thread.\n");
            exit(1);
        }
    }
    // Create hydrogen threads
    for (int i = oxyatoms; i < oxyatoms+hydatoms; i++)
    {   
        printf("thH: %d",i);
        if (pthread_create(thread + i, NULL, hydrogen_thread_body, NULL))
        {
            printf("Couldn’t create an hydrogen thread.\n");
            exit(2);
        }
    }
    printf("Waiting for hydrogen and oxygen atoms to react …\n");
    Logger::Info("Waiting for hydrogen and oxygen atoms to react ...");
    // Wait for all threads to finish
    for (int i = 0; i < oxyatoms+hydatoms; i++)
    {
        if (pthread_join(thread[i], NULL))
        {
            printf("The thread could not be joined.\n");
            exit(3);
        }
    }
    printf("Number of made water molecules: %d\n", num_of_water_molecules);
    Logger::Info("Waiting for hydrogen and oxygen atoms to react ..."+to_string(num_of_water_molecules));

    sem_destroy(hydrogen_sem);
    return 0;
}