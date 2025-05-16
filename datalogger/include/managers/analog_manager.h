#ifndef ANALOG_MANAGER_H
#define ANALOG_MANAGER_H

#include "modules/analog_sensors.h"
#include "modules/config_storage.h"

class AnalogManager {
public:
    // Inicialización en setup()
    static bool init(ConfigStorage &storage);
    
    // Actualización en loop()
    static void update();
    
    // Métodos para acceder a datos
    static String getCSVData();
    static String getCSVHeader();
    
    // Métodos de calibración y configuración
    static void calibratePH(float knownPH);
    static void calibrateDO(float knownDO);
    static void calibrateEC(float knownEC);
    static bool saveCalibration(ConfigStorage &storage);
    
    // Métodos para diagnóstico
    static void displayData();
    
    // Getters para datos individuales
    static float getPH();
    static float getDO();
    static float getEC();
    
    // Métodos para control de habilitación
    static void enable();
    static void disable();
    static bool isEnabled();

private:
    static AnalogSensors* sensors;
    static bool enabled;
    static unsigned long lastUpdateTime;
    
    // Constante para tasa de actualización
    static const unsigned long UPDATE_INTERVAL = ANALOG_SAMPLING_RATE;
};

#endif // ANALOG_MANAGER_H