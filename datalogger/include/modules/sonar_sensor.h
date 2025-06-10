#ifndef SONAR_SENSOR_H
#define SONAR_SENSOR_H

#include <HardwareSerial.h>
#include "config.h"

class SonarSensor {
public:
    SonarSensor();
    void begin();
    void update();
    
    // Getters para datos del sensor
    float getDepth();
    float getTemperature();
    float getSpeed();
    bool isDataValid();
    
    // Funciones para CSV
    String getCSVHeader();
    String getCSVData();
    
    // Diagnóstico
    unsigned long getLastUpdateTime();
    String getLastRawData();

    bool detectBaudRate();

private:
    HardwareSerial sonarSerial;
    
    // Datos del sensor
    float depth;              // Profundidad en metros
    float temperature;        // Temperatura del agua en °C
    float speed;             // Velocidad del agua (si disponible)
    bool dataValid;          // Indica si los datos son válidos
    
    // Control de tiempo
    unsigned long lastUpdateTime;
    unsigned long lastDataReceived;
    
    // Buffer para datos
    String inputBuffer;
    String lastRawData;      // Para diagnóstico
    
    // Funciones privadas
    void parseData(String data);
    void parseGarminData(String data);
    void parseNMEAData(String data);
    void resetData();
    bool isTimeoutExpired();
    float parseFloatValue(String str);
    
    // Validación de datos
    bool isValidDepth(float depth_val);
    bool isValidTemperature(float temp_val);

    bool testBaudRate(uint32_t baudRate);
    bool isValidSonarData(String data);
};

#endif // SONAR_SENSOR_H