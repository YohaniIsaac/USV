#ifndef ANALOG_SENSORS_H
#define ANALOG_SENSORS_H

#include <Arduino.h>
#include "config.h"

#define NUM_READINGS 10

class AnalogSensors {
public:
    // Constructor
    AnalogSensors(uint8_t phPin, uint8_t doPin, uint8_t ecPin);

    // Factores de calibración
    float phOffset;
    float phSlope;
    float doOffset;
    float doSlope;
    float ecOffset;
    float ecSlope;
    float ecK;

    // Últimos valores calculados
    float lastPH;
    float lastDO;
    float lastEC;

    // Últimos valores crudos promediados
    int lastRawPH;
    int lastRawDO;
    int lastRawEC;
    
    // Inicializar los sensores
    void begin();
    
    // Actualización de sensores
    void update();
    
    // Calibración por sensor (asume que el sensor debería medir el valor dado AHORA)
    int calibrateCurrentPH(float shouldBePH);
    int calibrateCurrentDO(float shouldBeDO);
    int calibrateCurrentEC(float shouldBeEC);

    //  calibración por command (texto)
    void setPhCalibration(float offset, float slope);
    void setDoCalibration(float offset, float slope);
    void setEcCalibration(float offset, float slope, float k);
    
    // Inicializar con valores predeterminados
    void initDefaultCalibration();
    
    // Obtener datos para CSV
    String getCSVData() const;
    
private:
    // Pines de conexión
    uint8_t phPin;
    uint8_t doPin;
    uint8_t ecPin;
    
    // Variables para promediar lecturas
    int phReadings[NUM_READINGS];
    int doReadings[NUM_READINGS];
    int ecReadings[NUM_READINGS];
    int readIndex;

    // Funciones auxiliares
    void performReadings();
    int getAverageReading(int readings[]);
    void calculateValues();
};

#endif // ANALOG_SENSORS_H