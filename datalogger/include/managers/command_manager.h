#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>
#include "modules/analog_sensors.h"
#include "modules/sd_logger.h"
#include "modules/config_storage.h"

class CommandManager {
public:
    CommandManager(AnalogSensors& sensors, SDLogger& sdLogger, ConfigStorage& configStorage);
    
    void begin();
    void update();
    
    // Métodos para habilitar/deshabilitar funcionalidades
    void setAnalogEnabled(bool enabled);
    void setSDLoggingEnabled(bool enabled);
    void setSerialOutputEnabled(bool enabled);
    
    // Getter para el estado de módulos
    bool isAnalogEnabled() const { return enableAnalogSensors; }
    bool isSDLoggingEnabled() const { return enableSDLogging; }
    bool isSerialOutputEnabled() const { return enableSerialOutput; }

private:
    AnalogSensors& sensors;
    SDLogger& sdLogger;
    ConfigStorage& configStorage;
    
    bool enableAnalogSensors;
    bool enableSDLogging;
    bool enableSerialOutput;
    
    void processCommand(String command);
    void displayHelp();
    void displaySensorData();
};

#endif // COMMAND_MANAGER_H