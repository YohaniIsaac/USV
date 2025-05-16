#include "pixhawk_interface.h"

PixhawkInterface::PixhawkInterface() : pixhawkSerial(1) {  // Usar UART1 en ESP32
    latitude = 0.0;
    longitude = 0.0;
    altitude = 0.0;
    heading = 0.0;
    lastUpdateTime = 0;
}

void PixhawkInterface::begin() {
    pixhawkSerial.begin(57600, SERIAL_8N1, PIXHAWK_RX_PIN, PIXHAWK_TX_PIN);
}

void PixhawkInterface::update() {
    unsigned long currentTime = millis();
    
    // Actualizar a la frecuencia configurada
    if (currentTime - lastUpdateTime >= PIXHAWK_SAMPLING_RATE) {
        parseMAVLink();
        lastUpdateTime = currentTime;
    }
}

void PixhawkInterface::parseMAVLink() {
    // Esta es una implementación simplificada
    // Para una implementación completa, se debería usar una biblioteca como MAVLink
    
    while (pixhawkSerial.available()) {
        // Código de parseo MAVLink
        // Por ahora, simplemente simulamos valores para la demostración
        // En una implementación real, aquí se analizarían los mensajes MAVLink
        
        // Valores de ejemplo (simulados)
        latitude = 37.7749;
        longitude = -122.4194;
        altitude = 10.5;
        heading = 270.0;
        
        // Consumir todos los bytes disponibles
        pixhawkSerial.read();
    }
}

float PixhawkInterface::getLatitude() {
    return latitude;
}

float PixhawkInterface::getLongitude() {
    return longitude;
}

float PixhawkInterface::getAltitude() {
    return altitude;
}

float PixhawkInterface::getHeading() {
    return heading;
}

String PixhawkInterface::getCSVHeader() {
    return "latitude,longitude,altitude,heading";
}

String PixhawkInterface::getCSVData() {
    return String(latitude, 6) + "," + 
           String(longitude, 6) + "," + 
           String(altitude, 2) + "," + 
           String(heading, 2);
}