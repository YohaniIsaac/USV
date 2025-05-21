#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>
#include "modules/analog_sensors.h"
#include "eeprom_manager.h"
#include "logger.h"

class CommandManager {
public:
    CommandManager(AnalogSensors& sensors);
    
    void begin();
    void update();

private:
    AnalogSensors& sensors;
    
    void processCommand(String command);
    void displaySensorData();
    void displayCalibrationData();
    void displayHelp();
    
};

#endif // COMMAND_MANAGER_H