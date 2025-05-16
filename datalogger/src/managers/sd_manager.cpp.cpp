#include "managers/sd_manager.h"
#include "logger.h"
#include "config.h"

// Inicialización de variables estáticas
SDLogger* SDManager::logger = nullptr;
bool SDManager::enabled = false;
unsigned long SDManager::lastWriteTime = 0;

bool SDManager::init(ConfigStorage &storage) {
    LOG_INFO("SD", "Inicializando módulo SD");
    
    // Crear instancia de logger
    logger = new SDLogger();
    
    // Inicializar SD
    if (!logger->begin()) {
        LOG_ERROR("SD", "Error al inicializar la tarjeta SD");
        enabled = false;
        return false;
    }
    
    enabled = true;
    LOG_INFO("SD", "Tarjeta SD inicializada correctamente");
    return true;
}

void SDManager::update() {
    if (!enabled || logger == nullptr) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Actualizar a la frecuencia configurada
    if (currentTime - lastWriteTime >= WRITE_INTERVAL) {
        logger->update();
        lastWriteTime = currentTime;
    }
}

bool SDManager::writeHeader(const String &header) {
    if (!enabled || logger == nullptr) {
        return false;
    }
    
    return logger->writeHeader(header);
}

bool SDManager::writeData(const String &data) {
    if (!enabled || logger == nullptr) {
        return false;
    }
    
    bool success = logger->writeData(data);
    if (success) {
        LOG_DEBUG("SD", "Datos registrados en SD");
    } else {
        LOG_ERROR("SD", "Error al registrar datos en SD");
    }
    
    return success;
}

void SDManager::enable() {
    // Solo habilitar si ya está inicializado
    if (logger != nullptr) {
        enabled = true;
        LOG_INFO("SD", "Logging SD habilitado");
    } else {
        LOG_ERROR("SD", "No se puede habilitar SD: no está inicializado");
    }
}

void SDManager::disable() {
    enabled = false;
    LOG_INFO("SD", "Logging SD deshabilitado");
}

bool SDManager::isEnabled() {
    return enabled;
}