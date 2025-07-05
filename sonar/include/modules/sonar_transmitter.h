#ifndef SONAR_TRANSMITTER_H
#define SONAR_TRANSMITTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class SonarTransmitter {
public:
    // Constructor
    SonarTransmitter();
    
    // Métodos principales
    bool begin();
    void update();
    void addSonarMeasurement(double depth, double offset, double range, uint32_t totalLog, uint32_t tripLog);
    
    // Configuración
    void setTransmissionInterval(unsigned long intervalMs);
    void setAveragingPeriod(unsigned long periodMs);
    
    // Estado
    bool isConnected() const;
    unsigned long getLastTransmissionTime() const;
    int getMeasurementCount() const;

private:
    // Comunicación serial
    HardwareSerial* dataloggerSerial;
    
    // Control de tiempo
    unsigned long transmissionInterval_;      // Intervalo de transmisión (500ms por defecto)
    unsigned long averagingPeriod_;          // Período de promediado (450ms por defecto)
    unsigned long lastTransmissionTime_;
    unsigned long measurementStartTime_;
    
    // Datos para promedio
    struct SonarData {
        double depth;
        double offset;
        double range;
        uint32_t totalLog;
        uint32_t tripLog;
        bool valid;
    };
    
    static const int MAX_MEASUREMENTS = 50;  // Máximo de mediciones a promediar
    SonarData measurements_[MAX_MEASUREMENTS];
    int measurementCount_;
    bool collectingData_;
    
    // Métodos privados
    void startMeasurementPeriod();
    void calculateAndTransmitAverage();
    SonarData calculateAverage();
    void transmitData(const SonarData& data);
    String formatDataPacket(const SonarData& data);
    void resetMeasurements();
    
    // Validación
    bool isValidMeasurement(double depth, double offset, double range);
};

#endif // SONAR_TRANSMITTER_H