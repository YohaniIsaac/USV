#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "modules/sd_logger.h"
#include "modules/config_storage.h"

class SDManager {
public:
    // Inicialización en setup()
    static bool init(ConfigStorage &storage);
    
    // Actualización en loop()
    static void update();
    
    // Métodos para registro de datos
    static bool writeHeader(const String &header);
    static bool writeData(const String &data);
    
    // Métodos para control de habilitación
    static void enable();
    static void disable();
    static bool isEnabled();

private:
    static SDLogger* logger;
    static bool enabled;
    static unsigned long lastWriteTime;
    
    // Constante para tasa de escritura
    static const unsigned long WRITE_INTERVAL = SD_WRITE_RATE;
};

#endif // SD_MANAGER_H