#include <iostream> //cout, cin
#include <string.h> //strcmp(), stoi(), stol()
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h> //fprintf()
#include <stdlib.h>
#include <random>       //random device
#include <chrono>       //milliseconds, system_clock
#include <unistd.h>     //sleep()
#include <sys/ipc.h>    //Interprocess Communication
#include <sys/shm.h>    //Shared Memory
#include <semaphore.h>  //sem_t
#include "shareddata.h" // dependency on the struct of type SharedData

using namespace std;
// These namespaces are necessary for the timer
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

int main(int argc, char *argv[])
{

    // Shared memory variables
    int shmid = 0;
    struct SharedData *p;
    long saladmakertime = 0; /* flag variables */

    // Vegetable weight variables
    double tomato = 0;
    double onion = 0;
    double pepper = 0;

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
                //saladmkrtime file flag
                if (strcmp(argv[i], "-m") == 0)
                {
                    if ((i + 1) < argc)
                    {
                        saladmakertime = stol(argv[i + 1]); /* convert to long */
                        // ** Uncomment to watch it run in real time!
                        // cout << "Saladmaker time: " << saladmakertime << " seconds" << endl; // for testing purposes
                    }
                }
                //Shmid flag
                if (strcmp(argv[i], "-s") == 0)
                {
                    if ((i + 1) < argc)
                    {
                        shmid = stoi(argv[i + 1]);
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
    if (shmid <= 0)
    {
        cout << "No Shared Memory ID provided! Add the -s [shmid] flag when running the executable." << endl;
        exit(1);
    }
    if (saladmakertime <= 0)
    {
        cout << "No saladmaker time value provided! Add the -m [salmkrtime] flag when running the executable." << endl;
        exit(1);
    }

    //===========================================
    // Initialization
    // Attach to shared memory
    p = (struct SharedData *)shmat(shmid, NULL, 0);
    // HOW IS P NOT LESS THAN 0??? --> To-do: error message for shmat for testing purposes
    if (p < 0)
    {
        cout << "shmat() failed" << endl;
        exit(1);
    }
    else
    {
        cout << "shmat() successful" << endl;
    }

    //Add and initialize named POSIX semaphores
    sem_t *vegSelection;
    sem_t *wrt;

    // Unlink each semaphore before they are created to wipe out any previously saved value
    sem_unlink("mutex_for_selection");
    sem_unlink("mutex_for_wrt");

    // Create and initialize named POSIX semaphores
    vegSelection = sem_open("mutex_for_selection", O_CREAT, 0666, 1); // initial value of 1, so only 1 process in vegSelection critical section at a time
    wrt = sem_open("mutex_for_wrt", O_CREAT, 0666, 1);

    // Start of vegetable selection critical section =============
    sem_wait(vegSelection); // ensures only 1 process selects their vegetable at a time
    bool picking = true;
    string chosenVeg;
    string vegArray[3] = {"tomato", "onion", "pepper"};
    int saladmakerNum; // this keeps track of which saladmaker this process is (1, 2, or 3)

    while (picking)
    {
        // generate random number, 0 to 2
        int random = rand() % 3; // random in the range 0 to 2

        // check if the vegetable at the index of that number has been chosen by another saladmaker (from shared memory)
        if (p->vegetable[random] == false)
        {
            // if no, choose this vegetable
            chosenVeg = vegArray[random];
            p->vegetable[random] = true;

            // Randomly chosen vegetable also determines which saladmaker this is
            // Tomato --> SM1, Onion --> SM2, Pepper --> SM3
            saladmakerNum = random + 1;
            picking = false;
        }
    }

    // End of critical section
    sem_post(vegSelection);
    //=======================================================

    // Create and open the temporal log file for writing
    FILE *fptr;
    FILE *fptrparallel; // file to record parallel times

    if (saladmakerNum == 1)
    {
        fptr = fopen("saladmaker1Log.txt", "w");
        fprintf(fptr, "SALADMAKER 1 LOG\n");
        fprintf(fptr, "===================================\n");
    }
    else if (saladmakerNum == 2)
    {
        fptr = fopen("saladmaker2Log.txt", "w");
        fprintf(fptr, "SALADMAKER 2 LOG\n");
        fprintf(fptr, "===================================\n");
    }
    else if (saladmakerNum == 3)
    {
        fptr = fopen("saladmaker3Log.txt", "w");
        fprintf(fptr, "SALADMAKER 3 LOG\n");
        fprintf(fptr, "===================================\n");
    }

    // If file fails to open
    if (fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }

    // Open the file for writing parallel execution times
    fptrparallel = fopen("parallel.txt", "a");
    // If file fails to open
    if (fptrparallel == NULL)
    {
        printf("Error!");
        exit(1);
    }

    // Display chosen vegetable
    cout << "Chosen vegetable: " << chosenVeg << endl;
    cout << "I am saladmaker " << saladmakerNum << endl;

    // loop until the number of salads needed is met
    while (p->numSaladsProduced <= p->numSaladsToMake)
    {
        // Start waiting timer
        // time code borrowed from https://www.delftstack.com/howto/cpp/how-to-get-time-in-milliseconds-cpp/
        auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // get time in ms for timestamp (time since epoch, 1970)
        fprintf(fptr, "%ld: Waiting for vegetables\n", ms);

        // Wait for vegetable from Chef
        if (saladmakerNum == 1)
        {
            // ** Uncomment to watch it run in real time!
            // cout << "Waiting on semaphore 1..." << endl; // for testing purposes
            sleep(1); // for testing
            while (true)
            {
                if (p->saladmaker1 == true)
                {
                    break;
                }
            }
        }
        else if (saladmakerNum == 2)
        {
            // ** Uncomment to watch it run in real time!
            // cout << "Waiting on semaphore 2..." << endl; // for testing purposes
            sleep(1);
            while (true)
            {
                if (p->saladmaker2 == true)
                {
                    break;
                }
            }
        }
        else if (saladmakerNum == 3)
        {
            // ** Uncomment to watch it run in real time!
            // cout << "Waiting on semaphore 3..." << endl; // for testing purposes
            sleep(1); // for testing purposes
            while (true)
            {
                if (p->saladmaker3 == true)
                {
                    break;
                }
            }
        }

        // end waiting timer, write "received vegetables" in log file
        auto ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // get time in ms for timestamp (time since epoch, 1970)
        fprintf(fptr, "%ld: Received vegetables\n", ms1);
        // ** Uncomment to watch it run in real time!
        // cout << "woke up and received vegetables. Posted to receivedVeg" << endl; // for testing purposes

        // Calculate time spent waiting
        double timewaiting = (ms1 - ms) / 1000; // time the saladmaker spent waiting for vegetables in seconds (/1000 gives secs from msecs)

        // Wake up the Chef ================================
        sem_wait(wrt);
        p->receivedVeg = true;

        // Write time spent waiting for vegetables to file
        switch (saladmakerNum)
        {
        case 1:
            // Add time "waiting" to shared memory
            p->SM1waitingTime = p->SM1waitingTime + timewaiting;
            break;

        case 2:
            // Add time "waiting" to shared memory
            p->SM2waitingTime = p->SM2waitingTime + timewaiting;
            break;

        case 3:
            // Add time "waiting" to shared memory
            p->SM3waitingTime = p->SM3waitingTime + timewaiting;
            break;

        default:
            cout << "Error generating vegetable weights." << endl;
        }
        sem_post(wrt);
        //==================================

        sleep(1); // give time for chef to realize boolean has been changed

        // Reset the received Veg ==========
        sem_wait(wrt);
        p->receivedVeg = false;
        sem_post(wrt);
        //==================================

        // Generate a weight within vegetable weight bounds
        // Code borrowed from https://stackoverflow.com/questions/19665818/generate-random-numbers-using-c11-random-library
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_real_distribution<double> dist(0.8, 1.2);

        // Weigh vegetable and save sum total of current vegetable weights
        switch (saladmakerNum)
        {
        case 1:
            // SM1 receives onion and pepper
            onion = onion + (dist(mt) * 30);
            pepper = pepper + (dist(mt) * 50);
            tomato = tomato + 80; // SM1 always has tomatoes
            break;

        case 2:
            // SM2 needs tomato and pepper
            tomato = tomato + (dist(mt) * 80);
            pepper = pepper + (dist(mt) * 50);
            onion = onion + 30; // SM2 always has onions
            break;

        case 3:
            // SM3 needs tomato and onion
            tomato = tomato + (dist(mt) * 80);
            onion = onion + (dist(mt) * 30);
            pepper = pepper + 50; //SM3 always has peppers
            break;

        default:
            cout << "Error generating vegetable weights." << endl;
        }

        // ** Uncomment to watch the saladmaker run in real time!
        // cout << "Tomato weight: " << tomato << endl
        //      << "Onion weight: " << onion << endl
        //      << "Pepper weight: " << pepper << endl;

        // Check if vegetable weights are enough for salad
        if (tomato >= 80 && onion >= 30 && pepper >= 50)
        {
            // enough, so make salad
            // "started making salad" + timestamp in file
            auto startTimer = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // Get time for log timestamp
            fprintf(fptr, "%ld: Started making salad\n", startTimer);

            // Set timerOn to true, critical section ====================
            sem_wait(wrt);
            p->timerOn = true;
            p->numSaladmakersRunning = p->numSaladmakersRunning + 1;

            // If more than one process running, record the timer start val
            if (p->numSaladmakersRunning == 2)
            {
                p->timerStartVal = startTimer;
            }
            sem_post(wrt);
            //===========================================================

            // Generate a salmkrtime to 'make salad' (in reality the process is sleeping to simulate being busy)
            std::random_device rd1;
            std::mt19937 mt1(rd1());
            std::uniform_real_distribution<double> dist1(0.8, 1.0);

            double timeworking; /* [0.8 - 1.0] * saladmakertime is time spent working */
            timeworking = saladmakertime * dist1(mt1);

            // sleep for salmkrtime
            // Limitation: with sleep(), a process only sleeps for an INT amount of time, not double
            // However, the ideal double value is recorded to shared memory.
            sleep(timeworking);

            // "finished making salad" + timestamp in file
            auto endTimer = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // Get time for log timestamp
            fprintf(fptr, "%ld: Finished making salad\n\n", endTimer);

            // Check if multiple processes running in parallel ================
            sem_wait(wrt);
            p->numSaladmakersRunning = p->numSaladmakersRunning - 1;
            // cout<<"Num saladmakers running: "<<p->numSaladmakersRunning <<endl;     // for testing purposes
            // If there is only 1 process running, the parallel period just ended!
            if (p->numSaladmakersRunning == 1)
            {
                p->timerEndVal = endTimer;

                // Record the parallel period to file
                fprintf(fptrparallel, "\t - %ld to %ld\n", p->timerStartVal, p->timerEndVal);
                // cout<<"Parallel period: "<<p->timerStartVal << " to " << p->timerEndVal <<endl;  // for testing purposes
            }
            // If no processes running anymore, turn the timerOn to off
            if (p->numSaladmakersRunning == 0)
            {
                p->timerOn = false;
            }
            sem_post(wrt);
            //================================================================

            //Start to writing stats critical section ========================
            sem_wait(wrt);
            // check if numSalads met before incrementing
            if (p->numSaladsProduced == p->numSaladsToMake)
            {
                break;
            }
            // Increment numSaladsProduced
            p->numSaladsProduced = p->numSaladsProduced + 1;
            cout << "Made salad #" << p->numSaladsProduced << endl;
            // ** Uncomment to watch it run in real time!
            // cout << "Num Salads Produced: " << p->numSaladsProduced << endl; // for testing purposes

            // Add sum weights of vegetables to stats file
            switch (saladmakerNum)
            {
            case 1:
                // update SM1 vegetable variables
                p->SM1tomato = p->SM1tomato + 80;
                p->SM1onion = p->SM1onion + 30;
                p->SM1pepper = p->SM1pepper + 50;
                // Add time "working" (sleeping, in reality) and "waiting" to shared memory
                p->SM1workingTime = p->SM1workingTime + timeworking;
                break;

            case 2:
                // update SM2 vegetable variables
                p->SM2tomato = p->SM2tomato + 80;
                p->SM2onion = p->SM2onion + 30;
                p->SM2pepper = p->SM2pepper + 50;
                // Add time "working" (sleeping, in reality) and "waiting" to shared memory
                p->SM2workingTime = p->SM2workingTime + timeworking;
                break;

            case 3:
                // update SM3 vegetable variables
                p->SM3tomato = p->SM3tomato + 80;
                p->SM3onion = p->SM3onion + 30;
                p->SM3pepper = p->SM3pepper + 50;
                // Add time "working" (sleeping, in reality) and "waiting" to shared memory
                p->SM3workingTime = p->SM3workingTime + timeworking;
                break;

            default:
                cout << "Error generating vegetable weights." << endl;
            }

            // Decrement local variables (vegetables used in salad)
            tomato = tomato - 80;
            onion = onion - 30;
            pepper = pepper - 50;

            // check if numSalads met after incrementing
            if (p->numSaladsProduced == p->numSaladsToMake)
            {
                break;
            }

            // End of writing stats critical section
            sem_post(wrt);
            //===========================================================
        }
        else
        {
            // ** Uncomment to watch it run in real time!
            // cout << "Not enough vegetables" << endl; // for testing purposes

            // Writing to temporal log
            ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // Get time for log timestamp
            fprintf(fptr, "%ld: Not enough vegetables for salad\n", ms);
            // repeats loop, causing the sem_wait to trigger
        }
    }

    cout << "Finished!" << endl;

    // Close the temporal log file
    fclose(fptr);
    fclose(fptrparallel);

    // Close Semaphores
    sem_close(vegSelection);
    sem_close(wrt);

    // Detach from shared memory
    shmdt(p);
}