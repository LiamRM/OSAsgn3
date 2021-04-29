// create a struct to store shared data
struct SharedData
{
    // Stats variables
    int numSaladsProduced;
    double SM1tomato, SM1pepper, SM1onion;
    double SM2tomato, SM2pepper, SM2onion;
    double SM3tomato, SM3pepper, SM3onion;
    double SM1waitingTime, SM1workingTime;
    double SM2waitingTime, SM2workingTime;
    double SM3waitingTime, SM3workingTime;

    // Array to randomly determine vegetable for each saladmaker
    bool vegetable[3] = {false, false, false};

    // Shared variables
    int numSaladsToMake = 0;

    // SIGNALS for each process to wake up
    // true --> process is running
    // false --> process is blocked
    bool saladmaker1;       
    bool saladmaker2;
    bool saladmaker3;
    bool receivedVeg;

    // Listings of time periods that 2 or more saladmakers were busy at the same time
    bool timerOn = false;
    long timerStartVal;
    long timerEndVal;
    int numSaladmakersRunning = 0;
};