#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <Preferences.h>
#include <Arduino.h>

class ConfigStorage {
public:
    ConfigStorage();
    
    // Inicialización
    bool begin(const char* namespaceName = "datalogger");
    void end();
    
    // Métodos para pH
    bool savePHCalibration(float offset, float slope);
    bool loadPHCalibration(float &offset, float &slope);
    
    // Métodos para DO (oxígeno disuelto)
    bool saveDOCalibration(float offset, float slope);
    bool loadDOCalibration(float &offset, float &slope);
    
    // Métodos para EC (conductividad eléctrica)
    bool saveECCalibration(float offset, float slope, float kValue);
    bool loadECCalibration(float &offset, float &slope, float &kValue);
    
    // Métodos para configuración general
    bool saveModuleState(const char* moduleName, bool enabled);
    bool loadModuleState(const char* moduleName, bool defaultState = true);
    
    // Método para limpiar todas las configuraciones (reset)
    bool clearAllSettings();

private:
    Preferences preferences;
    bool isInitialized;
};

#endif // CONFIG_STORAGE_H