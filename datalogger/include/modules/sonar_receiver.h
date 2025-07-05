#ifndef SONAR_RECEIVER_H
#define SONAR_RECEIVER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class SonarReceiver {
public:
    // Constructor
    SonarReceiver();
    
    // Métodos principales
    bool begin();
    void update();
    
    // Getters para datos del sonar
    double getDepth() const;
    double getOffset() const;
    double getRange() const;
    uint32_t getTotalLog() const;
    uint32_t getTripLog() const;
    
    // Estado y metadatos
    bool hasValidData() const;
    bool isConnected() const;
    unsigned long getLastDataTime() const;
    unsigned long getDataTimestamp() const;
    int getSampleCount() const;
    
    // Estadísticas
    unsigned long getTotalPacketsReceived() const;
    unsigned long getValidPacketsReceived() const;
    unsigned long getErrorPacketsReceived() const;
    
    // Para logging/CSV
    String getCSVHeader() const;
    String getCSVData() const;
    void showStatus() const;

private:
    // Comunicación serial
    HardwareSerial* wroomSerial;
    
    // Datos del sonar
    struct SonarData {
        double depth;
        double offset;
        double range;
        uint32_t totalLog;
        uint32_t tripLog;
        unsigned long timestamp;    // Timestamp del ESP-WROOM
        int sampleCount;           // Número de muestras promediadas
        bool valid;
        unsigned long receivedTime; // Cuando se recibió en datalogger
    } currentData_;
    
    // Estado de conexión
    bool connected_;
    unsigned long lastDataTime_;
    unsigned long connectionTimeout_;
    
    // Estadísticas
    unsigned long totalPacketsReceived_;
    unsigned long validPacketsReceived_;
    unsigned long errorPacketsReceived_;
    
    // Buffer para recepción
    String inputBuffer_;
    
    // Métodos privados
    void processIncomingData();
    bool parsePacket(const String& packet);
    void updateConnectionStatus();
    void logPacketStats();
    double parseDoubleValue(const String& value);
    uint32_t parseUInt32Value(const String& value);
    void resetData();
};

#endif // SONAR_RECEIVER_H