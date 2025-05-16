#ifndef SONAR_SENSOR_H
#define SONAR_SENSOR_H

#include <HardwareSerial.h>
#include "config.h"

class SonarSensor {
public:
    SonarSensor();
    void begin();
    void update();
    float getDepth();
    String getCSVHeader();
    String getCSVData();

private:
    HardwareSerial sonarSerial;
    float depth;
    unsigned long lastUpdateTime;
    void parseData(String data);
};

#endif // SONAR_SENSOR_H