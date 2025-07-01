#ifndef PIXHAWK_INTERFACE_H
#define PIXHAWK_INTERFACE_H

#include <HardwareSerial.h>
#include "config.h"


class PixhawkInterface {
public:
    PixhawkInterface();
    void begin();
    void update();
    
    // Getters para los datos
    float getLatitude();
    float getLongitude();
    float getAltitude();
    float getHeading();
    
    // Funciones para CSV
    String getCSVHeader();
    String getCSVData();

private:
    HardwareSerial pixhawkSerial;
    
    // Datos de navegación
    float latitude;
    float longitude;
    float altitude;
    float heading;
    
    unsigned long lastUpdateTime;
    
    // Funciones de procesamiento MAVLink
    void parseMAVLink();
    void processMAVLinkMessage(uint8_t* buffer, uint8_t length);
    void parseGPSRawInt(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseAttitude(uint8_t* buffer, uint8_t length, bool isMAVLink2);
    void parseGlobalPosition(uint8_t* buffer, uint8_t length, bool isMAVLink2);
};

#endif // PIXHAWK_INTERFACE_H