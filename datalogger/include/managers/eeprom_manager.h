#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Estructura para almacenar todas las calibraciones
struct CalibrationData {
    float phOffset;
    float phSlope;
    float doOffset;
    float doSlope;
    float ecOffset;
    float ecSlope;
    float ecK;
    uint8_t magic;  // Para validar datos
};

#define EEPROM_SIZE sizeof(CalibrationData)
#define MAGIC_NUMBER 0xAB

class EEPROMManager {
public:
    // Inicializar EEPROM
    static bool begin();
    
    // Guardar todas las calibraciones
    static bool saveCalibrations(float phOffset, float phSlope, 
                                float doOffset, float doSlope,
                                float ecOffset, float ecSlope, float ecK);
    
    // Cargar todas las calibraciones
    static bool loadCalibrations(float& phOffset, float& phSlope,
                                float& doOffset, float& doSlope,
                                float& ecOffset, float& ecSlope, float& ecK);
    
    // Verificar si hay datos válidos
    static bool hasValidData();
    
    // Borrar todos los datos
    static void clearAll();

private:
    static bool initialized;
};

#endif // EEPROM_MANAGER_H