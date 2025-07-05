#ifndef SONAR_SENSOR_H
#define SONAR_SENSOR_H

#include "config.h"

class SonarSensor {
public:
    bool begin();
    void update();

private:
    void processData(String data);
    
};

#endif // SONAR_SENSOR_H