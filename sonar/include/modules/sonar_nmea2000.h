#ifndef SONAR_NMEA2000_H
#define SONAR_NMEA2000_H

#include <Arduino.h>
#include "logger.h"

// Forward declarations - NO incluir NMEA2000_CAN.h aquí
class tN2kMsg;

class SonarNMEA2000 {
public:
    // Constructor
    SonarNMEA2000();
    
    // Métodos principales
    bool setup();
    void update();
    void show_data();
    
    // Getters para obtener los datos
    double getDepth() const;
    double getOffset() const;
    double getRange() const;
    uint32_t getTotalLog() const;
    uint32_t getTripLog() const;
    float getTemperature() const;
    
    // Métodos de estado
    bool hasValidDepthData() const;
    bool hasValidLogData() const;
    bool isInitialized() const;
    
    // Configuración
    void enableRawMessages(bool enable);
    
    // Para CSV/logging
    String getCSVHeader() const;
    String getCSVData() const;

private:
    // Variables para almacenar datos
    double lastDepth_;
    double lastOffset_;
    double lastRange_;
    uint32_t lastTotalLog_;
    uint32_t lastTripLog_;
    float lastTemperature_;
    
    // Estado del sistema
    bool depthDataValid_;
    bool logDataValid_;
    bool rawMessagesEnabled_;
    bool initialized_;
    unsigned long lastDataTime_;
    unsigned long lastTempReadTime_;
    
    // Métodos privados para procesamiento de mensajes
    void handleNMEA2000Message(const tN2kMsg &N2kMsg);
    void processDepthData(const tN2kMsg &N2kMsg);
    void processDistanceLog(const tN2kMsg &N2kMsg);
    void processTemperature(const tN2kMsg &N2kMsg);
    void printRawMessage(const tN2kMsg &N2kMsg);

    // Función estática para el callback
    static void staticMessageHandler(const tN2kMsg &N2kMsg);
    
    // Instancia estática para el callback
    static SonarNMEA2000* instance_;
};

#endif // SONAR_NMEA2000_H