#include <iostream>    //cout()
#include <string.h>    //strcmp(), strtok()
#include <semaphore.h> //sem_t()
#include <stdlib.h>
#include <fcntl.h> // semaphores
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>      //getline()
#include <random>       //random_device, mt(), dist()
#include <chrono>       //milliseconds, system_clock
#include <stdio.h>      //fprintf()
#include <unistd.h>     //sleep()
#include <semaphore.h>  //sem_t
#include <sys/ipc.h>    //Interprocess Communication
#include <sys/shm.h>    //Shared Memory
#include "shareddata.h" // dependency on the struct of type SharedData

using namespace std;
// These namespaces are necessary for the timer
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

int main(int argc, char *argv[])
{

    //Initialize variables
    int shmid; /* shared memory variables */
    struct SharedData *p;
    int numOfSalads = 0; /* flag variables */
    long chefTime = 0;

    //=====HANDLING MAIN ARGUMENTS====================
    switch (argc)
    {

    // First argument is just the location of the file
    case 1:
        cout << "No additional arguments were passed." << endl;
        break;

    case 0:
        cout << "Error, somehow? This should not happen." << endl;
        break;

    // More than 1 argument
    default:
        // Looping through inline parameters
        for (int i = 0; i < argc; i++)
        {
            try
            {
                //NumOfSalads file flag
                if (strcmp(argv[i], "-n") == 0)
                {
                    if ((i + 1) < argc)
                    {
                        numOfSalads = stoi(argv[i + 1]);
                    }
                }
                //Cheftime flag
                if (strcmp(argv[i], "-m") == 0)
                {
                    if ((i + 1) < argc)
                    {
                        chefTime = stol(argv[i + 1]); /* convert to long */
                    }
                }
            }
            catch (std::invalid_argument &e)
            {
                // if no conversion could be performed from string to integer
                cout << "----ERROR----" << endl
                     << "Non-integer argument supplied to either the -n flag or -m flag." << endl
                     << "Please check your arguments." << endl
                     << endl;
            }
        }
    }

    // Check that all parameters are provided
    if (numOfSalads <= 0)
    {
        cout << "No Number of Salads provided! Add the -n [# salads] flag when running the executable." << endl;
        exit(1);
    }
    if (chefTime <= 0)
    {
        cout << "No chef time value provided! Add the -m [cheftime] flag when running the executable." << endl;
        exit(1);
    }

    //===========================================

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(struct SharedData), IPC_CREAT | 0666);

    // Attach sharedData Struct to shared memory
    p = (struct SharedData *)shmat(shmid, NULL, 0);
    if (p < 0)
    {
        cout << "shmat() failed" << endl;
        exit(1);
    }

    // Write numOfSalads to shared memory
    p->numSaladsToMake = numOfSalads;

    // Output the ID of the shared memory so saladmakers can be spawned
    cout << "Shared Memory ID: " << shmid << endl;
    cout << "Please initialize the saladmakers..." << endl;

    //Add named POSIX semaphores
    sem_t *vegSelection;
    sem_t *wrt;

    // Unlink each semaphore before they are created to wipe out any previously saved value
    sem_unlink("mutex_for_selection");
    sem_unlink("mutex_for_wrt");

    // Create and initialize named POSIX semaphores
    vegSelection = sem_open("mutex_for_selection", O_CREAT, 0666, 1); // initial value of 1, so only 1 process in vegSelection critical section at a time
    wrt = sem_open("mutex_for_wrt", O_CREAT, 0666, 1);

    // check if all saladmakers were initialized
    while (true)
    {
        if (p->vegetable[0] == true && p->vegetable[1] == true && p->vegetable[2] == true)
        {
            cout << "ALL saladmakers initialized. Saladmaking has begun, please stand by..." << endl;
            break;
        }
    }

    string vegArray[3] = {"tomato", "onion", "pepper"};

    // WHILE LOOP -- loop until the number of salads needed is met
    while (p->numSaladsProduced <= p->numSaladsToMake)
    {

        // Generate 2 random vegetables to give to a saladmaker
        int randomVeg1 = rand() % 3;       // random in the range 0 to 2
        int randomVeg2 = rand() % 3;       // random in the range 0 to 2
        int chosenSaladmaker = rand() % 3; // random in the range 0 to 2
        while (randomVeg2 == randomVeg1)
        {
            randomVeg2 = rand() % 3; // ensures the chosen vegetables are different
        }
        while (chosenSaladmaker == randomVeg1 || chosenSaladmaker == randomVeg2)
        {
            chosenSaladmaker = rand() % 3; // picks the leftover number
        }
        chosenSaladmaker = chosenSaladmaker + 1;

        if (p->numSaladsProduced == p->numSaladsToMake)
        {
            break;
        }

        // 'Notify' any waiting saladmakers via a semaphore
        // start writing to critical section
        if (chosenSaladmaker == 1)
        {
            p->saladmaker1 = true; // wakes up SM1
        }
        else if (chosenSaladmaker == 2)
        {
            p->saladmaker2 = true; // wakes up SM2
        }
        else if (chosenSaladmaker == 3)
        {
            p->saladmaker3 = true; // wakes up SM3
        }
        // end of writing to critical section

        // check if numSalads met after giving vegetables
        if (p->numSaladsProduced == p->numSaladsToMake)
        {
            break;
        }

        // wait for saladmakers to pick vegetables
        while (!p->receivedVeg)
        {
            // if true (vegetables received), this loop will break
            // if false, wait
        }

        // Wait for Chef time (max time period the chef waits between serving successive pairs of ingredients)
        // Generate a random decimal within range [0.5 ... 1.0]
        // Code borrowed from https://stackoverflow.com/questions/19665818/generate-random-numbers-using-c11-random-library
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_real_distribution<double> dist(0.5, 1.0);

        // Sleep for [0.5 * chefTime ... chefTime]
        sleep(chefTime * dist(mt));

        // Loop again until all salads are made
    }

    // Display final statistics
    cout << "Finished making salads! Outputting stats:" << endl
         << "================================================" << endl
         << "Number of Salads Produced: " << p->numSaladsProduced << endl
         << "Sum weight of each ingredient used by Saladmaker 1:" << endl
         << "\tTomato: " << p->SM1tomato << "g" << endl
         << "\tOnion: " << p->SM1onion << "g" << endl
         << "\tPepper: " << p->SM1pepper << "g" << endl
         << "Sum weight of each ingredient used by Saladmaker 2:" << endl
         << "\tTomato: " << p->SM2tomato << "g" << endl
         << "\tOnion: " << p->SM2onion << "g" << endl
         << "\tPepper: " << p->SM2pepper << "g" << endl
         << "Sum weight of each ingredient used by Saladmaker 3:" << endl
         << "\tTomato: " << p->SM3tomato << "g" << endl
         << "\tOnion: " << p->SM3onion << "g" << endl
         << "\tPepper: " << p->SM3pepper << "g" << endl
         << "Total time spent working: " << endl
         << "\tSaladmaker 1: " << p->SM1workingTime << " seconds" << endl
         << "\tSaladmaker 2: " << p->SM2workingTime << " seconds" << endl
         << "\tSaladmaker 3: " << p->SM3workingTime << " seconds" << endl
         << "Total time spent waiting: " << endl
         << "\tSaladmaker 1: " << p->SM1waitingTime << " seconds" << endl
         << "\tSaladmaker 2: " << p->SM2waitingTime << " seconds" << endl
         << "\tSaladmaker 3: " << p->SM3waitingTime << " seconds" << endl;

    // List the time periods 2 or more saladmakers were working in parallel
    cout << "List of time periods 2 or more saladmakers were working in parallel: " << endl
         << "================================================" << endl;

    // Gives time for saladmakers to finish and close the parallel.txt file pointers BEFORE reading from file
    sleep(4);
    // Read from parallel execution file
    string line;
    ifstream myfile("parallel.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            cout << line << '\n';
        }
        myfile.close();
    }

    else
        cout << "Unable to open file";

    // Close Semaphores
    sem_close(vegSelection);
    sem_close(wrt);

    // Unlink Semaphores
    sem_unlink("mutex_for_selection");
    sem_unlink("mutex_for_wrt");

    // Detach from shared memory
    shmdt(p);

    // Delete any shared memory segments
    int err = shmctl(shmid, IPC_RMID, NULL); /* Remove segment */
    if (err == -1)
        perror("Removal error.");
    else
        printf("Shared memory removed. No error (%d)\n", (int)(err));
}