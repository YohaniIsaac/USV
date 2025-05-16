#include "modules/config_storage.h"

ConfigStorage::ConfigStorage() {
    isInitialized = false;
}

bool ConfigStorage::begin(const char* namespaceName) {
    if (isInitialized) {
        return true;
    }
    
    isInitialized = preferences.begin(namespaceName, false); // false = modo lectura/escritura
    return isInitialized;
}

void ConfigStorage::end() {
    if (isInitialized) {
        preferences.end();
        isInitialized = false;
    }
}

// Métodos para calibración de pH
bool ConfigStorage::savePHCalibration(float offset, float slope) {
    if (!isInitialized) return false;
    
    preferences.putFloat("phOffset", offset);
    preferences.putFloat("phSlope", slope);
    return true;
}

bool ConfigStorage::loadPHCalibration(float &offset, float &slope) {
    if (!isInitialized) return false;
    
    offset = preferences.getFloat("phOffset", 0.0);
    slope = preferences.getFloat("phSlope", 3.3 / 4095.0 * 3.5);
    return true;
}

// Métodos para calibración de DO
bool ConfigStorage::saveDOCalibration(float offset, float slope) {
    if (!isInitialized) return false;
    
    preferences.putFloat("doOffset", offset);
    preferences.putFloat("doSlope", slope);
    return true;
}

bool ConfigStorage::loadDOCalibration(float &offset, float &slope) {
    if (!isInitialized) return false;
    
    offset = preferences.getFloat("doOffset", 0.0);
    slope = preferences.getFloat("doSlope", 1.0);
    return true;
}

// Métodos para calibración de EC
bool ConfigStorage::saveECCalibration(float offset, float slope, float kValue) {
    if (!isInitialized) return false;
    
    preferences.putFloat("ecOffset", offset);
    preferences.putFloat("ecSlope", slope);
    preferences.putFloat("ecK", kValue);
    return true;
}

bool ConfigStorage::loadECCalibration(float &offset, float &slope, float &kValue) {
    if (!isInitialized) return false;
    
    offset = preferences.getFloat("ecOffset", 0.0);
    slope = preferences.getFloat("ecSlope", 1.0);
    kValue = preferences.getFloat("ecK", 10.0);
    return true;
}

// Métodos para estado de módulos
bool ConfigStorage::saveModuleState(const char* moduleName, bool enabled) {
    if (!isInitialized) return false;
    
    preferences.putBool(moduleName, enabled);
    return true;
}

bool ConfigStorage::loadModuleState(const char* moduleName, bool defaultState) {
    if (!isInitialized) return false;
    
    return preferences.getBool(moduleName, defaultState);
}

bool ConfigStorage::clearAllSettings() {
    if (!isInitialized) return false;
    
    return preferences.clear();
}