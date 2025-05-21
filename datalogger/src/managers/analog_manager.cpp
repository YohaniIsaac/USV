#include "managers/analog_manager.h"
#include "logger.h"
#include "config.h"

// Inicialización de variables estáticas
AnalogSensors* AnalogManager::sensors = nullptr;
bool AnalogManager::enabled = false;
unsigned long AnalogManager::lastUpdateTime = 0;

bool AnalogManager::init() {
    LOG_INFO("ANALOG", "Inicializando sensores analógicos");
    
    // Crear instancia de sensores
    sensors = new AnalogSensors(ANALOG_SENSOR1_PIN, ANALOG_SENSOR2_PIN, ANALOG_SENSOR3_PIN);
    sensors->begin();
    
    // Cargar calibraciones guardadas
    if (sensors->loadCalibration()) {
        LOG_INFO("ANALOG", "Calibraciones cargadas correctamente");
    } else {
        LOG_INFO("ANALOG", "Usando valores de calibración predeterminados");
    }
    
    enabled = true;
    LOG_INFO("ANALOG", "Sensores analógicos inicializados correctamente");
    return true;
}

void AnalogManager::update() {
    if (!enabled || sensors == nullptr) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Actualizar a la frecuencia configurada
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        sensors->update();
        lastUpdateTime = currentTime;
    }
}

String AnalogManager::getCSVData() {
    if (!enabled || sensors == nullptr) {
        return "";
    }
    
    return sensors->getCSVData();
}

String AnalogManager::getCSVHeader() {
    if (!enabled || sensors == nullptr) {
        return "";
    }
    
    return sensors->getCSVHeader();
}

void AnalogManager::calibratePH(float knownPH) {
    if (!enabled || sensors == nullptr) {
        return;
    }
    
    int rawValue = sensors->readRawPH();
    sensors->calibratePH(knownPH, rawValue);
    
    LOG_INFO("ANALOG", "pH calibrado a " + String(knownPH) + 
             " (valor crudo: " + String(rawValue) + ")");
}

void AnalogManager::calibrateDO(float knownDO) {
    if (!enabled || sensors == nullptr) {
        return;
    }
    
    int rawValue = sensors->readRawDO();
    sensors->calibrateDO(knownDO, rawValue);
    
    LOG_INFO("ANALOG", "Oxígeno disuelto calibrado a " + String(knownDO) + 
             " mg/L (valor crudo: " + String(rawValue) + ")");
}

void AnalogManager::calibrateEC(float knownEC) {
    if (!enabled || sensors == nullptr) {
        return;
    }
    
    int rawValue = sensors->readRawEC();
    sensors->calibrateEC(knownEC, rawValue);
    
    LOG_INFO("ANALOG", "Conductividad calibrada a " + String(knownEC) + 
             " μS/cm (valor crudo: " + String(rawValue) + ")");
}

bool AnalogManager::saveCalibration(ConfigStorage &storage) {
    if (!enabled || sensors == nullptr) {
        return false;
    }
    
    bool success = sensors->saveCalibration(storage);
    if (success) {
        LOG_INFO("ANALOG", "Calibración guardada en memoria");
    } else {
        LOG_ERROR("ANALOG", "Error al guardar calibración");
    }
    
    return success;
}

void AnalogManager::displayData() {
    if (!enabled || sensors == nullptr) {
        LOG_INFO("ANALOG", "Los sensores analógicos están deshabilitados");
        return;
    }
    
    // Leer valores crudos
    int phRaw = sensors->readRawPH();
    int doRaw = sensors->readRawDO();
    int ecRaw = sensors->readRawEC();
    
    // Leer valores convertidos
    float ph = sensors->getLastPH();
    float dissolvedOxygen = sensors->getLastDO();
    float conductivity = sensors->getLastEC();
    
    // Construir mensaje para mostrar
    String message = "\n----------------------------------------\n";
    message += "       LECTURAS DE SENSORES\n";
    message += "----------------------------------------\n";
    message += "pH: " + String(ph, 2) + " (raw: " + String(phRaw) + ")\n";
    message += "Oxígeno Disuelto: " + String(dissolvedOxygen, 2) + 
               " mg/L (raw: " + String(doRaw) + ")\n";
    message += "Conductividad: " + String(conductivity, 0) + 
               " μS/cm (raw: " + String(ecRaw) + ")\n";
    message += "----------------------------------------";
    
    LOG_INFO("ANALOG", message);
}

float AnalogManager::getPH() {
    if (!enabled || sensors == nullptr) {
        return 0.0;
    }
    return sensors->getLastPH();
}

float AnalogManager::getDO() {
    if (!enabled || sensors == nullptr) {
        return 0.0;
    }
    return sensors->getLastDO();
}

float AnalogManager::getEC() {
    if (!enabled || sensors == nullptr) {
        return 0.0;
    }
    return sensors->getLastEC();
}

void AnalogManager::enable() {
    if (sensors == nullptr) {
        return;
    }
    enabled = true;
    LOG_INFO("ANALOG", "Sensores analógicos habilitados");
}

void AnalogManager::disable() {
    enabled = false;
    LOG_INFO("ANALOG", "Sensores analógicos deshabilitados");
}

bool AnalogManager::isEnabled() {
    return enabled;
}
