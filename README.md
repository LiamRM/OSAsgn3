# OSAsgn3
NYUAD Operating Systems - Programming Assignment 3
Submitted: April 29th, 2021

## 🚀 WHAT I LEARNED
- Synchronization with POSIX Semaphores, that allow for saladmaker processes to coordinate their work via appropriate P() and V() primitives
- Creating, allocating and de-allocating Shared Memory between processes, so that each process may conveniently access and change main-memory resident variables

## ⚙️ HOW TO RUN THE PROGRAM
- Navigate on a UNIX-based terminal (Terminal on Mac, Ubuntu subsystem on Windows) to the folder containing the .cpp files
- type `make` to compile and create an executable file
- first, type: 
	`./chef -m [cheftime] -n [numSaladsToMake]`
- The Chef executable will output a shared memory ID. We can use this to initialize the saladmaker executables.
- Now, in 3 separate ttys type:
	 `./saladmaker -m [salmkrtime] -s [shared memory id]`
- The saladmakers will output text to indicate they are running. Wait until all salads are made to view the final runtime statistics output in the Chef executable!
**Note**: This program will only run on UNIX-based systems, meaning Mac or Linux. Windows users will have to install a [Ubuntu subsystem](https://ubuntu.com/wsl) or can follow [this guide](https://itsfoss.com/install-linux-in-virtualbox/) to install a Linux Virtual Machine.
![Runtime Screenshot](https://github.com/LiamRM/OSAsgn3/blob/main/Screenshot%20from%202021-04-29%2021-50-05.png)

## 🎨 GENERAL DESIGN DECISIONS
**Salad making**
For my saladmaking scheme, I decided every salad has EXACTLY 80g tomatoes, 50g of peppers, and 30g of onions. The average ingredient also weighs this exact amount, with a variance of (0.8 * weight) - (1.2 * weight).
Each saladmaker also stockpiles ingredients. Meaning, when a saladmaker makes a salad, it subtracts 80g tomatoes, 50g peppers, and 30g onions from its available ingredients and saves the rest for the next salad.

## 🎨 SHARED MEMORY DESIGN DECISIONS

- To ensure each saladmaker is distributed a random vegetable that they will always have throughout runtime, I implemented a Boolean array for each chosen vegetable, which would be randomly chosen once a saladmaker is initialized. The array is initialized as follows:

	EX: `bool vegetable[3] = {false, false, false}` --> [Tomato, Onion, Pepper]

As seen above, each Boolean in array vegetable corresponds to a vegetable. Also, the randomly chosen vegetable determines which saladmaker it is, and ultimately which semaphore the process will post to and wait on. It is ALWAYS in this order, and is consistent whenever vegetables are referenced throughout the entire program.

To summarize, in this program: 
	vegetable[0] --> Tomato --> Saladmaker 1
	vegetable[1] --> Onion  --> Saladmaker 2
	vegetable[2] --> Pepper --> Saladmaker 3

**Note:** However, just because a saladmaker is initialized first does not mean it is automatically Saladmaker 1. The vegetables are picked randomly with the following algorithm:
	- Generate a random number from 0 to 2
	- if vegetable[] boolean with this index is "true", it has already been chosen. Generate another number.
	- else if vegetable[] boolean with this index is "false", it has not been chosen yet. Choose this vegetable and set this boolean to "true".

## 🎨 SEMAPHORE DESIGN DECISIONS
- There are 2 semaphores in shared memory: 
	vegSelection, which is used to ensure only 1 process at a time may select a vegetable.
	wrt, which is used to ensure only 1 process is writing to and reading from shared memory at any given point. 
In both cases, if any process tries to choose a vegetable or write to shared memory while another process is doing so, it is automatically blocked until the process(es) before it finish with this critical section. 

I originally planned for each saladmaker to have their own semaphore for the Chef to 'wake them up' when giving them the appropriate vegetable combination -- however, I ultimately decided a method of signalling was more appropriate, because with the way the salad making process is designed, each saladmaker is 'woken up' ONLY when their specific combination of vegetables is made available and is given both vegetables. 
For example, if Saladmaker 1 (always has tomatoes available) is asleep waiting for an onion, the Chef will wake it up only when it has both an onion and pepper in hand and give BOTH vegetables. If, on the other hand, Saladmaker 1 were waiting on ONLY an onion (given it has enough peppers and tomatoes), it would make sense to block this Saladmaker on a tomato semaphore, because multiple processes could be waiting for a tomato simulataneously. This is not the case here. Each saladmaker is waiting for their own independent combination of vegetables.
So, I chose to implement 3 booleans in shared memory as follows:
	bool saladmaker1;       
    	bool saladmaker2;
    	bool saladmaker3;
If a bool is "true", it means it has received both the vegetables it requires from the Chef process. If it is "false", the saladmaker is blocked in a while loop.

## 🎨 PARALLEL TIMING DESIGN DECISIONS
To calculate when saladmakers are working in parallel, I:
- increment a shared memory int variable numSaladmakersRunning by 1 every time a saladmaker starts making salad 
- start a timer when a saladmaker starts making salad, saving the value in a shared memory long variable timerStartVal if there are more than 1 saladmakers running. 
- whenever a saladmaker finishes making salad, decrement numSaladmakersRunning by 1, and if there is only 1 saladmaker running, record a timer value, saving the value in a shared memory long variable timerEndVal.
- Save the range from timerStartVal to timerEndVal to a file called "parallel.txt". This indicates the range in which 2 or more processes were running in parallel.
- Read from this file in the Chef process when outputting the final statistics. Voila.

## ⚡ LIMITATIONS/INACCURACIES
- I used the sleep() function to simulate the saladmaker "working" on a salad. However, with sleep() a process only sleeps for an INT amount of time, not double. I record the correct time "working" on salads to shared memory via the variable 'timeworking' of type double. When you compare the timestamps in each process' temporal log on the other hand, the time between starting a salad and finishing a salad is a difference of WHOLE seconds, not fractions of seconds. As a result, the time calculated for when a saladmaker is working **may not 100% accurately represent the reality** (fractions of seconds are ignored in reality). 
- My solution also is subject to **busy waiting**, meaning the CPU expends cycles stuck in a while loop, doing no useful work. If I were to improve this solution, I would implement semaphores for each saladmaker so that the saladmaker is **suspended** while waiting and no longer wastes CPU cycles.
