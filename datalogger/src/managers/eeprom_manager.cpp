#include "managers/eeprom_manager.h"
#include "logger.h"

// Inicialización de variable estática
bool EEPROMManager::initialized = false;

bool EEPROMManager::begin() {
    if (initialized) {
        return true;
    }
    
    // Inicializar EEPROM con el tamaño necesario
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
    
    LOG_INFO("EEPROM", "Inicializado correctamente");
    return true;
}

bool EEPROMManager::saveCalibrations(float phOffset, float phSlope,
                                    float doOffset, float doSlope,
                                    float ecOffset, float ecSlope, float ecK) {
    if (!initialized) {
        LOG_ERROR("EEPROM", "EEPROM no inicializado");
        return false;
    }
    
    CalibrationData data;
    data.phOffset = phOffset;
    data.phSlope = phSlope;
    data.doOffset = doOffset;
    data.doSlope = doSlope;
    data.ecOffset = ecOffset;
    data.ecSlope = ecSlope;
    data.ecK = ecK;
    data.magic = MAGIC_NUMBER;
    
    // Escribir la estructura completa
    uint8_t* dataPtr = (uint8_t*)&data;
    for (size_t i = 0; i < sizeof(CalibrationData); i++) {
        EEPROM.write(i, dataPtr[i]);
    }
    
    // Confirmar los cambios
    bool success = EEPROM.commit();
    
    if (success) {
        LOG_INFO("EEPROM", "Calibraciones guardadas correctamente");
    } else {
        LOG_ERROR("EEPROM", "Error al guardar calibraciones");
    }
    
    return success;
}

bool EEPROMManager::loadCalibrations(float& phOffset, float& phSlope,
                                    float& doOffset, float& doSlope,
                                    float& ecOffset, float& ecSlope, float& ecK) {
    if (!initialized) {
        LOG_ERROR("EEPROM", "EEPROM no inicializado");
        return false;
    }
    
    if (!hasValidData()) {
        LOG_WARN("EEPROM", "No hay datos válidos de calibración");
        return false;
    }
    
    CalibrationData data;
    uint8_t* dataPtr = (uint8_t*)&data;
    
    // Leer la estructura completa
    for (size_t i = 0; i < sizeof(CalibrationData); i++) {
        dataPtr[i] = EEPROM.read(i);
    }
    
    // Copiar los valores
    phOffset = data.phOffset;
    phSlope = data.phSlope;
    doOffset = data.doOffset;
    doSlope = data.doSlope;
    ecOffset = data.ecOffset;
    ecSlope = data.ecSlope;
    ecK = data.ecK;
    
    LOG_INFO("EEPROM", "Calibraciones cargadas correctamente");
    return true;
}

bool EEPROMManager::hasValidData() {
    if (!initialized) {
        return false;
    }
    
    // Verificar número mágico al final de la estructura
    size_t magicAddress = offsetof(CalibrationData, magic);
    return EEPROM.read(magicAddress) == MAGIC_NUMBER;
}

void EEPROMManager::clearAll() {
    if (!initialized) {
        return;
    }
    
    // Escribir 0xFF en toda la EEPROM
    for (size_t i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    
    EEPROM.commit();
    LOG_INFO("EEPROM", "Calibraciones borradas");
}