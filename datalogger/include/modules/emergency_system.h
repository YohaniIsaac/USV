#ifndef EMERGENCY_SYSTEM_H
#define EMERGENCY_SYSTEM_H

#include <HardwareSerial.h>
#include "config.h"

class EmergencySystem {
public:
    EmergencySystem();
    void begin();
    void update();
    bool isEmergencyActive();

private:
    HardwareSerial gpsSerial;
    bool emergencyActive;
    unsigned long lastCheckTime;
    void activateEmergencyGPS();
    void sendEmergencySignal();
};

#endif // EMERGENCY_SYSTEM_H