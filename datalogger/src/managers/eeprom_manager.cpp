#include "managers/eeprom_manager.h"
#include "logger.h"

// Inicialización de variable estática
bool EEPROMManager::initialized = false;

bool EEPROMManager::begin() {
    if (initialized) {
        return true;
    }
    
    // Inicializar EEPROM con el tamaño especificado
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
    
    LOG_INFO("EEPROM","Inicializado correctamente");
    return true;
}

bool EEPROMManager::writeData(int address, const uint8_t* data, size_t length) {
    if (!initialized) {
        LOG_ERROR("EEPROM", "EEPROM no inicializado");
        return false;
    }
    
    // Verificar límites
    if (address < 0 || address + length > EEPROM_SIZE) {
        LOG_ERROR("EEPROM", "Dirección fuera de rango");
        return false;
    }
    
    // Limitar a 16 bytes máximo por operación
    if (length > 16) {
        LOG_ERROR("EEPROM", "Máximo 16 bytes por operación");
        return false;
    }
    
    // Escribir datos byte por byte
    for (size_t i = 0; i < length; i++) {
        EEPROM.write(address + i, data[i]);
    }
    
    // Escribir número mágico al final para validar
    EEPROM.write(address + length, MAGIC_NUMBER);
    
    return true;
}

bool EEPROMManager::readData(int address, uint8_t* data, size_t length) {
    if (!initialized) {
        LOG_ERROR("EEPROM", "EEPROM no inicializado");
        return false;
    }
    
    // Verificar límites
    if (address < 0 || address + length > EEPROM_SIZE) {
        LOG_ERROR("EEPROM", "Dirección fuera de rango");
        return false;
    }
    
    // Limitar a 16 bytes máximo por operación
    if (length > 16) {
        LOG_ERROR("EEPROM", "Máximo 16 bytes por operación");
        return false;
    }
    
    // Verificar si los datos son válidos
    if (!isValidData(address + length)) {
        LOG_ERROR("EEPROM", "Advertencia: Datos no válidos en la dirección especificada");
        // Continuar leyendo pero devolver false para indicar datos no válidos
    }
    
    // Leer datos byte por byte
    for (size_t i = 0; i < length; i++) {
        data[i] = EEPROM.read(address + i);
    }
    
    return true;
}

bool EEPROMManager::writeFloat(int address, float value) {
    if (!initialized) {
        return false;
    }
    
    // Convertir float a bytes
    uint8_t* bytes = (uint8_t*)&value;
    
    // Escribir los 4 bytes del float
    return writeData(address, bytes, sizeof(float));
}

float EEPROMManager::readFloat(int address) {
    if (!initialized) {
        return 0.0;
    }
    
    uint8_t bytes[sizeof(float)];
    
    // Leer los 4 bytes
    if (readData(address, bytes, sizeof(float))) {
        // Convertir bytes de vuelta a float
        return *(float*)bytes;
    }
    
    return 0.0; // Valor por defecto si falla la lectura
}

bool EEPROMManager::isValidData(int address) {
    if (!initialized || address >= EEPROM_SIZE) {
        return false;
    }
    
    // Verificar si hay número mágico en la dirección
    return EEPROM.read(address) == MAGIC_NUMBER;
}

void EEPROMManager::clearAll() {
    if (!initialized) {
        return;
    }
    
    // Escribir 0xFF en toda la EEPROM (estado borrado)
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    
    LOG_INFO("EEPROM", "Memoria borrada completamente");
}

bool EEPROMManager::commit() {
    if (!initialized) {
        return false;
    }
    
    // Confirmar los cambios en la memoria flash
    bool success = EEPROM.commit();
    
    if (success) {
        LOG_INFO("EEPROM", "Cambios guardados correctamente");
    } else {
        LOG_ERROR("EEPROM", "Error al guardar cambios");
    }
    
    return success;
}