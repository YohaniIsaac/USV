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

    // Métodos para gestión temporal del UART
    void pauseForEmergency();    // Liberar UART1 para GPS
    void resumeAfterEmergency(); // Retomar UART1 para Pixhawk
    bool isPaused() const { return paused; }

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
    
    // DATOS DE TIEMPO GPS
    uint64_t getGPSTimeUsec();     // Tiempo GPS en microsegundos desde epoch UNIX
    uint16_t getGPSYear();         // Año (ej: 2025)
    uint8_t getGPSMonth();         // Mes (1-12)
    uint8_t getGPSDay();           // Día (1-31)
    uint8_t getGPSHour();          // Hora (0-23) UTC
    uint8_t getGPSMinute();        // Minuto (0-59)
    uint8_t getGPSSecond();        // Segundo (0-59)
    bool hasValidGPSTime();        // Si los datos de tiempo son válidos
    String getGPSTimeString();     // Formato "YYYY-MM-DD HH:MM:SS"
    String getGPSDateString();     // Formato "YYYY-MM-DD"
    String getGPSTimeOnlyString(); // Formato "HH:MM:SS"

    // Funciones para CSV
    String save_CSVData();
    String getCSVHeader();

private:
    // Estado de pausa
    bool paused;
    bool wasInitialized;

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

    // DATOS DE TIEMPO GPS
    uint64_t gpsTimeUsec;          // Tiempo GPS en microsegundos desde epoch UNIX
    uint16_t gpsYear;              // Año
    uint8_t gpsMonth;              // Mes (1-12)
    uint8_t gpsDay;                // Día (1-31)
    uint8_t gpsHour;               // Hora (0-23) UTC
    uint8_t gpsMinute;             // Minuto (0-59)
    uint8_t gpsSecond;             // Segundo (0-59)
    bool gpsTimeValid;             // Si los datos de tiempo son válidos

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
    void parseSystemTime(uint8_t* payload);

    // FUNCIONES AUXILIARES PARA TIEMPO
    void convertUnixTimeToDateTime(uint64_t unixTimeUsec);  // Convertir timestamp a fecha/hora
    bool isLeapYear(uint16_t year);   
};

#endif // PIXHAWK_INTERFACE_H