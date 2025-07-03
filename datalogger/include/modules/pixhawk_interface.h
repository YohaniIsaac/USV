#ifndef PIXHAWK_INTERFACE_H
#define PIXHAWK_INTERFACE_H

#include <HardwareSerial.h>
#include "config.h"

class PixhawkInterface {
public:
    PixhawkInterface();
    void begin();
    void update();
    void show_message();
    
    // Getters básicos para los datos
    float getLatitude();
    float getLongitude();
    float getAltitude();
    float getHeading();
    
    // 🔋 Getters para batería
    float getBatteryVoltage();      // Voltaje en V
    float getBatteryCurrent();      // Corriente en A  
    int getBatteryRemaining();      // Carga restante en %
    float getBatteryTemperature();  // Temperatura en °C
    
    // 📊 Getters adicionales
    float getGroundSpeed();         // Velocidad sobre tierra en m/s
    float getAirSpeed();           // Velocidad del aire en m/s
    int getNumSatellites();        // Número de satélites GPS
    
    // Funciones para CSV
    String save_CSVData();
    String getCSVHeader();

private:
    HardwareSerial pixhawkSerial;
    
    // Datos básicos de navegación
    float latitude;
    float longitude;
    float altitude;
    float heading;
    
    // 🔋 Variables de batería
    float batteryVoltage;
    float batteryCurrent;
    int batteryRemaining;
    float batteryTemperature;
    
    // 📊 Variables adicionales
    float groundSpeed;
    float airSpeed;
    int numSatellites;
    int gpsFixType;
    
    unsigned long lastUpdateTime;
    
    // Funciones de procesamiento MAVLink
    void parseMAVLink();
    void processMAVLinkMessage(uint8_t* buffer, uint8_t length);
    void parseGPSRawInt(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseAttitude(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseGlobalPosition(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseBatteryStatus(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseVFRHUD(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseGPSStatus(uint8_t* buffer, uint8_t length, bool isMAVLink2);
};

#endif // PIXHAWK_INTERFACE_H