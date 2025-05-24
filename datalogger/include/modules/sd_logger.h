#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <SD.h>
#include <SPI.h>
#include "config.h"

class SDLogger {
public:
    SDLogger();
    bool begin();
    bool writeHeader(String header);
    bool writeData(String data);
    void update();

private:
    File dataFile;
    bool sdInitialized;
    unsigned long lastWriteTime;
    String dataHeader;                  // Header del archivo actual
    bool headerIsWritten;               // Header escrito
    String dataBuffer;
    String currentFilename;             // Nombre actual del archivo
    
    void generateUniqueFilename();      // Generar nombres únicos
};

#endif // SD_LOGGER_H