#include "sd_logger.h"

SDLogger::SDLogger() {
    sdInitialized = false;
    lastWriteTime = 0;
}

bool SDLogger::begin() {
    // Inicializar SPI para la tarjeta SD
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
    
    // Inicializar la tarjeta SD
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Error al inicializar la tarjeta SD");
        return false;
    }
    
    sdInitialized = true;
    Serial.println("Tarjeta SD inicializada correctamente");
    return true;
}

bool SDLogger::writeHeader(String header) {
    if (!sdInitialized) {
        return false;
    }
    
    // Abrir archivo en modo escritura
    dataFile = SD.open(LOG_FILENAME, FILE_WRITE);
    
    if (!dataFile) {
        Serial.println("Error al abrir el archivo de registro");
        return false;
    }
    
    // Escribir encabezado
    dataFile.println("timestamp," + header);
    dataFile.close();
    
    return true;
}

bool SDLogger::writeData(String data) {
    if (!sdInitialized) {
        return false;
    }
    
    // Agregar datos al buffer
    unsigned long timestamp = millis();
    dataBuffer += String(timestamp) + "," + data + "\n";
    
    return true;
}

void SDLogger::update() {
    unsigned long currentTime = millis();
    
    // Escribir en la SD a la frecuencia configurada
    if (currentTime - lastWriteTime >= SD_WRITE_RATE && dataBuffer.length() > 0) {
        // Abrir archivo en modo append
        dataFile = SD.open(LOG_FILENAME, FILE_APPEND);
        
        if (dataFile) {
            // Escribir todos los datos acumulados
            dataFile.print(dataBuffer);
            dataFile.close();
            
            // Limpiar buffer
            dataBuffer = "";
        } else {
            Serial.println("Error al abrir el archivo para escribir datos");
        }
        
        lastWriteTime = currentTime;
    }
}