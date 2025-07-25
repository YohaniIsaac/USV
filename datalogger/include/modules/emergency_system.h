#ifndef EMERGENCY_SYSTEM_H
#define EMERGENCY_SYSTEM_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "config.h"

class PixhawkInterface;

class EmergencySystem {
public:
    EmergencySystem();      //Constructor
    ~EmergencySystem();     // Destructor
    void begin();
    void update();
    bool isEmergencyActive() { return emergencyActive; }

    // Método para inyectar referencia al Pixhawk
    void setPixhawkInterface(PixhawkInterface* pixhawk) { pixhawkInterface = pixhawk; }

    // Métodos para monitoreo de voltaje
    float getCurrentVoltage() { return currentVoltage; }
    float getVoltageThreshold() { return EMERGENCY_VOLTAGE_THRESHOLD_REAL; }
    bool isPowerControlActive() { return powerControlActive; }

    // Métodos para monitoreo de la seguridad de conmutación
    unsigned long getTimeInCurrentState() { return millis() - stateChangeTime; }
    bool isStatePending() { return pendingStateChange; }

private:
    TinyGPSPlus gps;
    RF24* radio;  // Puntero para el objeto RF24
    SPIClass* hspi;  // SPI personalizado para ESP32
    
    // Referencia al Pixhawk para coordinación
    PixhawkInterface* pixhawkInterface;

    bool emergencyActive;
    bool gpsInitialized;
    bool nrfInitialized;
    unsigned long lastCheckTime;
    unsigned long lastGPSReadTime;
    unsigned long lastTransmitTime;

    // Variables para control de voltaje y alimentación
    float currentVoltage;
    bool powerControlActive;          // Estado del control de alimentación
    float voltageReadings[EMERGENCY_VOLTAGE_SAMPLES];  // Buffer para promedio
    int voltageReadingIndex;
    bool voltageBufferFull;

    // Variables para seguridad de conmutación
    static const unsigned long RELAY_SAFETY_TIME = 5000;  // 5 segundos mínimo
    unsigned long stateChangeTime;        // Tiempo del último cambio de estado
    bool pendingStateChange;              // Si hay un cambio pendiente
    bool targetEmergencyState;            // Estado objetivo (para cambio pendiente)
    unsigned long conditionStartTime;     // Cuando comenzó la condición actual
    bool conditionMet;                    // Si la condición se está cumpliendo

    // Direcciones para NRF24L01
    uint8_t txAddress[6];  // Dirección de transmisión
    
    // Estructura para datos de emergencia
    struct EmergencyData {
        float latitude;
        float longitude;
        float altitude;
        uint8_t satellites;
        bool valid;
    } currentLocation;

    // Estructura del paquete a transmitir (32 bytes máximo)
    struct __attribute__((packed)) EmergencyPacket {
        uint8_t header;      // 0xEE para identificar paquetes de emergencia
        float latitude;      // 4 bytes
        float longitude;     // 4 bytes
        float altitude;      // 4 bytes
        uint8_t satellites;  // 1 byte
        uint32_t timestamp;  // 4 bytes (millis)
        float voltage;       // 4 bytes (voltaje que causó la emergencia)
        uint8_t checksum;    // 1 byte
    };

    // Métodos para control de voltaje
    void checkVoltageLevel();
    float readAverageVoltage();
    void updateVoltageBuffer(float newReading);

    // Métodos para control seguro de conmutación
    void requestEmergencyStateChange(bool newEmergencyState);
    void processPendingStateChange();
    
    void activateEmergency();
    void deactivateEmergency();
    void setPowerControl(bool enable);  // Control del pin de alimentación
    
    void readGPSData();
    bool initializeNRF();
    void sendNRFPacket();
};

#endif // EMERGENCY_SYSTEM_H