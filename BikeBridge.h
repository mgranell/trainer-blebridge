#ifndef BikeBridge_H_
#define BikeBridge_H_

#include "NetworkService.h"

class BikeBridge 
{
public:
    void setup(NetworkService * pLog);
    void loop();

    short getPower() { return power; }
    uint getCrankCount() { return crankCount; }
    uint getLastCrankEventTime() { return lastCrankEventTime; } 

private:
    NetworkService * pLog;    
    short power;
    uint crankCount;
    uint lastCrankEventTime;

    void checkForRevolution();
    bool checkPwmLevel();
    void updateChangeDirection();
    int calculatePower(int l, int rpm);

};


#endif
