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
    
    // Getters básicos para los datos (mantener interfaz original)
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
    
    // Datos básicos de navegación (mantener nombres originales)
    float latitude;
    float longitude;
    float altitude;
    float heading;
    
    // Variables adicionales de orientación
    float roll;
    float pitch;
    float yaw;
    float altitudeRelative;
    
    // Velocidades (nombres españoles para compatibilidad)
    float velocidadSuelo;
    float velocidadAire;
    float velocidadVertical;
    
    // GPS (nombres españoles para compatibilidad)
    uint8_t tipoFixGPS;
    uint8_t satelites;
    
    // 🔋 Variables de batería (mantener nombres originales)
    float batteryVoltage;
    float batteryCurrent;
    int batteryRemaining;
    float batteryTemperature;
    
    // 📊 Variables adicionales (nombres ingleses para compatibilidad)
    float groundSpeed;
    float airSpeed;
    int numSatellites;
    int gpsFixType;
    
    // Estado de conexión y sistema
    bool connected;
    bool armed;
    uint8_t flightMode;
    uint8_t systemStatus;
    
    unsigned long lastUpdateTime;
    
    // Funciones de procesamiento MAVLink (refactorizadas)
    void parseMAVLink();
    void processMAVLinkMessage(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    
    // Métodos de parseo simplificados (basados en el código funcional)
    void parseHeartbeat(uint8_t* payload);
    void parseSysStatus(uint8_t* payload);
    void parseGPSRawInt(uint8_t* payload);
    void parseAttitude(uint8_t* payload);
    void parseGlobalPosition(uint8_t* payload);
    void parseVFRHUD(uint8_t* payload);
    void parseBatteryStatus(uint8_t* payload);
    void parseGPSStatus(uint8_t* payload);
};

#endif // PIXHAWK_INTERFACE_H