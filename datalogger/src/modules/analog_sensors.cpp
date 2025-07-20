#include "logger.h"
#include "modules/analog_sensors.h"
#include "managers/eeprom_manager.h"


AnalogSensors::AnalogSensors() {
    
    // Inicializar con valores predeterminados
    initDefaultCalibration();
    
    // Inicializar últimos valores
    lastPH = 7.0;
    lastDO = 0.0;
    lastEC = 0.0;
    lastRawPH = 0;
    lastRawDO = 0;
    lastRawEC = 0;
    
    // Inicializar arrays de lecturas
    for (int i = 0; i < NUM_READINGS; i++) {
        phReadings[i] = 0;
        doReadings[i] = 0;
        ecReadings[i] = 0;
    }
}

void AnalogSensors::begin() {
    // Intentar cargar calibraciones desde EEPROM
    if (!loadCalibrationFromEEPROM()) {
        LOG_WARN("ANALOG", "Usando calibraciones por defecto");
    }

    // Configurar pines como entradas
    pinMode(ANALOG_SENSOR_PH, INPUT);
    pinMode(ANALOG_SENSOR_DO, INPUT);
    pinMode(ANALOG_SENSOR_EC, INPUT);
    
    // Configuración específica|| para ESP32
    analogReadResolution(12);  // 12 bits (0-4095)
    analogSetAttenuation(ADC_11db);  // 0-3.3V

    LOG_INFO("ANALOG", "Sensores analógicos inicializados");
}

// ====================== CALCULAR VALORES Y ACTUALIZAR ======================
void AnalogSensors::performReadings() {
    // Realizar NUM_READINGS lecturas
    // Secuencia: pH -> DO -> EC -> pH -> DO -> EC ...
    for (int i = 0; i < NUM_READINGS; i++) {
        phReadings[i] = analogRead(ANALOG_SENSOR_PH);
        delayMicroseconds(100);  // Pequeña pausa entre lecturas
        
        doReadings[i] = analogRead(ANALOG_SENSOR_DO);
        delayMicroseconds(100);
        
        ecReadings[i] = analogRead(ANALOG_SENSOR_EC);
        delayMicroseconds(100);
        
        // Pausa entre ciclos de lectura
        if (i < NUM_READINGS - 1) {
            delay(10);  // 1ms entre ciclos
        }
    }
}

void AnalogSensors::update() {
    // Realizar lecturas de todos los sensores
    performReadings();

    // Calcular promedios
    lastRawPH = getAverageReading(phReadings);
    lastRawDO = getAverageReading(doReadings);
    lastRawEC = getAverageReading(ecReadings);
    LOG_INFO("ANALOG", "Valores crudos promediados:");
    LOG_INFO("ANALOG", "  pH raw: " + String(lastRawPH));
    LOG_INFO("ANALOG", "  DO raw: " + String(lastRawDO)); 
    LOG_INFO("ANALOG", "  EC raw: " + String(lastRawEC));
    // Convertir a voltaje
    float phVoltage = (lastRawPH  * 3.3) / 4095.0;
    float doVoltage = (lastRawDO * 3.3) / 4095.0;
    float ecVoltage = (lastRawEC * 3.3) / 4095.0;
    
    // Aplicar calibraciones
    lastPH = 7.0 + ((phVoltage - 2.5) / phSlope) + phOffset;

    // pH = pH_neutro + (ΔVoltaje / Pendiente) + Offset
    // phSlope = (Voltaje_pH4 - Voltaje_pH7) / (4 - 7) = ΔV / ΔpH
    lastDO = doVoltage * doSlope + doOffset;
    lastEC = ecVoltage * ecK * ecSlope + ecOffset;
}

// ====================== CALIBRACIÓN POR SENSOR  ======================
//      (asume que el sensor debería medir el valor dado AHORA)
int AnalogSensors::calibrateCurrentPH(float shouldBePH) {
    // Tomar lectura actual
    performReadings();
    int currentRaw = getAverageReading(phReadings);

    // Convertir valor raw a voltaje
    float voltage = (currentRaw * 3.3) / 4095.0;
    
    // Calcular qué valor daría con la calibración actual
    float currentCalculated = 7.0 + ((voltage - 2.5) / phSlope);
    
    // Ajustar offset para que la lectura actual sea el valor conocido
    phOffset = shouldBePH - currentCalculated;

    // Guardar en EEPROM
    saveCalibrationToEEPROM();
    
    return currentRaw;
}

int AnalogSensors::calibrateCurrentDO(float shouldBeDO) {
    // Tomar lectura actual
    performReadings();
    int currentRaw = getAverageReading(doReadings);
    
    // Convertir valor raw a voltaje
    float voltage = (currentRaw * 3.3) / 4095.0;
    
    // Calcular qué valor daría con la calibración actual
    float currentCalculated = voltage * doSlope;
    
    // Ajustar offset para que la lectura actual sea el valor conocido
    doOffset = shouldBeDO - currentCalculated;

    // Guardar en EEPROM
    saveCalibrationToEEPROM();

    return currentRaw;
}

int AnalogSensors::calibrateCurrentEC(float shouldBeEC) {
    // Tomar lectura actual
    performReadings();
    int currentRaw = getAverageReading(ecReadings);
    
    // Convertir valor raw a voltaje
    float voltage = (currentRaw * 3.3) / 4095.0;
    
    // Calcular qué valor daría con la calibración actual
    float currentCalculated = voltage * ecK * ecSlope;
    
    // Ajustar offset para que la lectura actual sea el valor conocido
    ecOffset = shouldBeEC - currentCalculated;

    // Guardar en EEPROM
    saveCalibrationToEEPROM();

    return currentRaw;
}

// ====================== CALIBRACIONES POR COMMAND ======================
void AnalogSensors::setPhCalibration(float offset, float slope) {
    phOffset = offset;
    phSlope = slope;

    // Guardar en EEPROM
    saveCalibrationToEEPROM();
}

void AnalogSensors::setDoCalibration(float offset, float slope) {
    doOffset = offset;
    doSlope = slope;

    // Guardar automáticamente en EEPROM
    saveCalibrationToEEPROM();
}

void AnalogSensors::setEcCalibration(float offset, float slope, float k) {
    ecOffset = offset;
    ecSlope = slope;
    ecK = k;

    // Guardar automáticamente en EEPROM
    saveCalibrationToEEPROM();
}

// ====================== GESTIÓN DE EEPROM ======================

bool AnalogSensors::loadCalibrationFromEEPROM() {
    if (!EEPROMManager::hasValidData()) {
        LOG_INFO("ANALOG", "No hay calibraciones válidas en EEPROM");
        return false;
    }
    
    bool success = EEPROMManager::loadCalibrations(
        phOffset, phSlope,
        doOffset, doSlope,
        ecOffset, ecSlope, ecK
    );
    
    if (success) {
        LOG_INFO("ANALOG", "Calibraciones cargadas desde EEPROM");
    } else {
        LOG_ERROR("ANALOG", "Error al cargar calibraciones de EEPROM");
    }
    
    return success;
}

bool AnalogSensors::saveCalibrationToEEPROM() {
    bool success = EEPROMManager::saveCalibrations(
        phOffset, phSlope,
        doOffset, doSlope,
        ecOffset, ecSlope, ecK
    );
    
    if (success) {
        LOG_INFO("ANALOG", "Calibraciones guardadas en EEPROM");
    } else {
        LOG_ERROR("ANALOG", "Error al guardar calibraciones en EEPROM");
    }
    
    return success;
}

void AnalogSensors::resetCalibrationToDefaults() {
    initDefaultCalibration();
    saveCalibrationToEEPROM();
    LOG_INFO("ANALOG", "Calibraciones reiniciadas a valores por defecto");
}

// ====================== Funciones extras ======================

void AnalogSensors::initDefaultCalibration() {
    // Valores predeterminados
    phOffset = 0.0;
    phSlope = 3.3 / 4095.0 * 3.5;   // típico 180mV
    
    doOffset = 0.0;
    doSlope = 1.0;                  // Ejemplo: 4mg/L por volt
    
    ecOffset = 0.0;
    ecSlope = 1.0;
    ecK = 10.0;                     // Constante de celda (se debe determinar experimentalmente)
}

String AnalogSensors::getCSVData() const {
    String data = "";
    data += String(lastPH, 2) + ",";
    data += String(lastDO, 2) + ",";
    data += String(lastEC, 0) + ",";
    data += String(phReadings[readIndex]) + ",";
    data += String(doReadings[readIndex]) + ",";
    data += String(ecReadings[readIndex]);
    
    return data;
}

int AnalogSensors::getAverageReading(int readings[]) {
    long sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
        sum += readings[i];
    }
    return sum / NUM_READINGS;
}