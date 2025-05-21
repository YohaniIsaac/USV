#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Tamaño total de EEPROM a usar
#define EEPROM_SIZE 64

// Direcciones predefinidas para diferentes tipos de calibración
#define EEPROM_PH_ADDR 0      // pH: offset(4) + slope(4) = 8 bytes
#define EEPROM_DO_ADDR 16     // DO: offset(4) + slope(4) = 8 bytes  
#define EEPROM_EC_ADDR 32     // EC: offset(4) + slope(4) + k(4) = 12 bytes
#define EEPROM_CONFIG_ADDR 48 // Configuración general: 16 bytes

class EEPROMManager {
public:
    // Inicializar EEPROM
    static bool begin();
    
    // Escribir datos en EEPROM (máximo 16 bytes por operación)
    static bool writeData(int address, const uint8_t* data, size_t length);
    
    // Leer datos desde EEPROM (máximo 16 bytes por operación)
    static bool readData(int address, uint8_t* data, size_t length);
    
    // Escribir un float en una dirección específica
    static bool writeFloat(int address, float value);
    
    // Leer un float desde una dirección específica
    static float readFloat(int address);
    
    // Verificar si hay datos válidos en una dirección (check magic number)
    static bool isValidData(int address);
    
    // Borrar todos los datos de EEPROM
    static void clearAll();
    
    // Confirmar cambios (commit)
    static bool commit();

private:
    static bool initialized;
    static const uint8_t MAGIC_NUMBER = 0xAA; // Número mágico para validar datos
};

#endif // EEPROM_MANAGER_H