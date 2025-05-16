#ifndef ANALOG_SENSORS_H
#define ANALOG_SENSORS_H

#include <Arduino.h>
#include "config.h"
#include "modules/config_storage.h"

#define NUM_READINGS 10

class AnalogSensors {
public:
    // Constructor
    AnalogSensors(uint8_t phPin, uint8_t doPin, uint8_t ecPin);
    
    // Inicializar los sensores
    void begin();
    
    // Leer valores crudos (raw) de los sensores
    int readRawPH();
    int readRawDO();  // Dissolved Oxygen
    int readRawEC();  // Electrical Conductivity
    
    // Convertir valores raw a unidades reales
    float getPH();
    float getDO();  // mg/L
    float getEC();  // μS/cm
    
    // Calibración
    void calibratePH(float knownPH, int rawValue);
    void calibrateDO(float knownDO, int rawValue);
    void calibrateEC(float knownEC, int rawValue);

    // Guardar y cargar calibraciones desde EEPROM
    bool saveCalibration(ConfigStorage &storage);
    bool loadCalibration(ConfigStorage &storage);

    // Actualización y lectura de todos los sensores
    void update();
    
    // Obtener último valor leído (sin nueva lectura)
    float getLastPH() const { return lastPH; }
    float getLastDO() const { return lastDO; }
    float getLastEC() const { return lastEC; }

    // Obtener datos para CSV
    String getCSVHeader() const { return "pH,dissolvedOxygen,conductivity,phRaw,doRaw,ecRaw"; }
    String getCSVData() const;
    
private:
    // Pines de conexión
    uint8_t phPin;
    uint8_t doPin;
    uint8_t ecPin;
    
    // Factores de calibración
    float phOffset;
    float phSlope;
    
    float doOffset;
    float doSlope;
    
    float ecOffset;
    float ecSlope;
    float ecK;  // Constante K del sensor EC
    
    // Últimos valores calculados
    float lastPH;
    float lastDO;
    float lastEC;
    
    // Variables para promediar lecturas
    int phReadings[NUM_READINGS];
    int doReadings[NUM_READINGS];
    int ecReadings[NUM_READINGS];
    int readIndex;
    
    // Funciones auxiliares
    int getAverageReading(int readings[]);
    float compensateTemperature(float ec, float temperature);
};

#endif // ANALOG_SENSORS_H