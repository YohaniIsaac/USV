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
    String dataBuffer;
};

#endif // SD_LOGGER_H