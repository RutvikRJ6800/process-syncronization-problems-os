#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <cmath>
#include "logger.h"
using namespace std;

//  Global Value that only reads it ... 
int finalValue;

// Site Status Details
unsigned int num_of_water_molecules, positions;
vector<bool> status;
int currentEnergyConsuption;
pthread_mutex_t status_lock;


// Oxygen Details
int oxygen;
pthread_mutex_t oxygen_mutex;


// Hydrogen Details
int hydrogen;
pthread_mutex_t hydrogen_mutex;

void makeBond(){

    int selectedPos;
    // printf("bond making\n");
    // Logger::Info("bond making..");
    while(true){
        pthread_mutex_lock(&status_lock);
        // printf("here\n");
        if(currentEnergyConsuption < finalValue)
        {
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
                if(status[prevPos] == 0 && status[selectedPos] == 0 && status[nextPos] == 0) break;
            }
        }
        pthread_mutex_unlock(&status_lock);

    }
    

    // modifiy status of the position
    int cec;
    int molNo;
    status[selectedPos] = 1;
    cec = ++currentEnergyConsuption;
    molNo = ++num_of_water_molecules;
    printf("molecule no: %d, created at spot: %d.\nCurrent energy consuption: %d.\n-------------------------\n",num_of_water_molecules, selectedPos, cec);
    Logger::Info("bond completed. \nmolecule no:"+to_string(molNo)+" created at spot: "+to_string(selectedPos)+".\nCurrent energy consuption: "+to_string(cec)+".\n-------------------------\n");
    
    pthread_mutex_unlock(&status_lock);
    
    
    sleep(3);

    //================================//
    //          Bond Making ()       //
    //==============================//

    // modifiy status of the position
    pthread_mutex_lock(&status_lock);
    status[selectedPos] = 0;
    --currentEnergyConsuption;
    pthread_mutex_unlock(&status_lock);
    
}

void *hydrogen_thread_body(void *arg)
{
    pthread_mutex_lock(&oxygen_mutex);
    pthread_mutex_lock(&hydrogen_mutex);
    hydrogen += 1; // apply lock here
    if (hydrogen >= 2 && oxygen >= 1)
    {
        hydrogen -= 2;
        // sem_signal(oxyQueue);
        oxygen -= 1;
        // num_of_water_molecules++;
        // int moLNo = ++num_of_water_molecules;
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
        makeBond(); 
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
    oxygen += 1; // apply lock here
    if (hydrogen >= 2) // here also
    {   
        hydrogen -= 2;
        oxygen -= 1;

        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
        makeBond(); 
    }
    else{
        // bond making not possible so release locks
        pthread_mutex_unlock(&hydrogen_mutex);
        pthread_mutex_unlock(&oxygen_mutex);
    }
    return NULL;
}

int main(int argc, char **argv)
{   
    ofstream log_file("logFile.txt", ios_base::out | ios_base::trunc );

    int hydatoms = atoi(argv[1]);
    int oxyatoms = atoi(argv[2]);
    int thsrd = stoi(argv[4]);

    num_of_water_molecules = 0;
    positions = stoi(argv[3]);
    currentEnergyConsuption = 0;

    finalValue = min(thsrd,(int)positions/2);
    // printf("finalValue: %d\n", finalValue);
    
    pthread_mutex_init(&oxygen_mutex, NULL);
    pthread_mutex_init(&hydrogen_mutex, NULL);
    pthread_mutex_init(&status_lock, NULL);
    
    status.resize(positions, 0);
    srand(time(0));

    // For creating 50 water molecules, we need 50 oxygen atoms and
    // 100 hydrogen atoms
    pthread_t thread[oxyatoms+hydatoms];
    // Create oxygen threads
    for (int i = 0; i < oxyatoms; i++)
    {   
        // printf("thO: %d",i);
        if (pthread_create(thread + i, NULL, oxygen_thread_body, NULL))
        {
            printf("Couldn’t create an oxygen thread.\n");
            exit(1);
        }
    }
    // Create hydrogen threads
    for (int i = oxyatoms; i < oxyatoms+hydatoms; i++)
    {   
        // printf("thH: %d",i);
        if (pthread_create(thread + i, NULL, hydrogen_thread_body, NULL))
        {
            printf("Couldn’t create an hydrogen thread.\n");
            exit(2);
        }
    }
    // Wait for all threads to finish
    for (int i = 0; i < oxyatoms+hydatoms; i++)
    {
        if (pthread_join(thread[i], NULL))
        {
            printf("The thread could not be joined.\n");
            exit(3);
        }
    }

    printf("Total Water Molecules: %d\n", num_of_water_molecules);
    Logger::Info("Total Water Molecules: "+to_string(num_of_water_molecules));

    return 0;
}
