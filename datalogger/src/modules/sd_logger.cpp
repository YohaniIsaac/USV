#include "logger.h"
#include "modules/sd_logger.h"

SDLogger::SDLogger() {
    sdInitialized = false;
    lastWriteTime = 0;
}

bool SDLogger::begin() {
    // Inicializar SPI para la tarjeta SD
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
    
    // Inicializar la tarjeta SD
    if (!SD.begin(SD_CS_PIN)) {
        LOG_ERROR("SD_LOGGER", "Error al inicializar la tarjeta SD");
        return false;
    }
        // Verificar información de la tarjeta
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    LOG_INFO("SD_LOGGER", "Tarjeta SD detectada. Tamaño: " + String(cardSize) + " MB");

    generateUniqueFilename();
    sdInitialized = true;
    LOG_INFO("SD_LOGGER", "Tarjeta SD inicializada correctamente");
    return true;
}

void SDLogger::generateUniqueFilename() {
    int lastNumber = -1;  
    
    // Buscar el último número usado en la SD
    File root = SD.open("/");
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String filename = file.name();
            
            // Verificar si el archivo sigue el patrón log_XXX.csv
            if (filename.startsWith("log_") && filename.endsWith(".csv") && filename.length() == 11) {
                // Extraer el número: "log_025.csv" -> "025" -> 25
                String numberStr = filename.substring(4, 7);  // Posiciones 4, 5, 6
                int number = numberStr.toInt();
                
                if (number > lastNumber) {
                    lastNumber = number;
                }
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    // Generar el siguiente número
    int nextNumber = lastNumber + 1;
    char buffer[20];
    sprintf(buffer, "/log_%03d.csv", nextNumber);
    currentFilename = String(buffer);
    
    LOG_INFO("SD_LOGGER", "Último archivo encontrado: log_" + String(lastNumber, 3));
    LOG_INFO("SD_LOGGER", "Nuevo nombre generado: " + currentFilename);
}

bool SDLogger::writeHeader(String header) {
    dataHeader += header;
    headerIsWritten = false;

    LOG_INFO("SD_LOGGER", "Header configurado: '" + dataHeader + "'");
    LOG_INFO("SD_LOGGER", "El archivo se creará cuando se escriban los primeros datos");
    return true;
}

bool SDLogger::writeData(String data) {
    if (!sdInitialized) {
        return false;
    }
    
    // Agregar datos al buffer
    unsigned long timestamp = millis();
    dataBuffer += data;
    LOG_INFO("SD_LOGGER", "Buffer configurado a: "+ dataBuffer);
    return true;
}

void SDLogger::update() {
    unsigned long currentTime = millis();
    LOG_INFO("SD_LOGGER", "en update");
    // Escribir en la SD a la frecuencia configurada
    if (dataBuffer.length() > 0) {
        // Abrir archivo en modo append
        dataFile = SD.open(currentFilename, FILE_APPEND);
        ;
        if (!headerIsWritten){
            // Escribir header
            LOG_INFO("SD_LOGGER", "Archivo:  '" + currentFilename + "'  generado automcaticamente");
            dataFile.println(dataHeader);
            headerIsWritten = true;
        }
        if (dataFile) {
            LOG_INFO("SD_LOGGER", "Escribiendo datos");
            // Escribir todos los datos acumulados
            dataFile.println(dataBuffer);
            dataFile.flush();
            dataFile.close();

            LOG_DEBUG("SD_LOGGER", "Datos escritos: " + String(dataBuffer.length()) + " caracteres");
            
            // Limpiar buffer
            dataBuffer = "";
        } else {
            LOG_ERROR("SD_LOGGER", "Error al abrir el archivo para escribir datos");
        }
        
        lastWriteTime = currentTime;
    }
}