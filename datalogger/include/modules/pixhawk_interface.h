#ifndef PIXHAWK_INTERFACE_H
#define PIXHAWK_INTERFACE_H

#include <HardwareSerial.h>
#include "config.h"

class PixhawkInterface {
public:
    PixhawkInterface();
    void begin();
    void update();
    float getLatitude();
    float getLongitude();
    float getAltitude();
    float getHeading();
    String getCSVHeader();
    String getCSVData();

private:
    HardwareSerial pixhawkSerial;
    float latitude;
    float longitude;
    float altitude;
    float heading;
    unsigned long lastUpdateTime;
    void parseMAVLink();
};

#endif // PIXHAWK_INTERFACE_H