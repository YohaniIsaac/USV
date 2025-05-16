#include "modules/analog_sensors.h"

// Constructor
AnalogSensors::AnalogSensors(uint8_t phPin, uint8_t doPin, uint8_t ecPin) 
    : phPin(phPin), doPin(doPin), ecPin(ecPin) {
    // Inicializar factores de calibración con valores predeterminados
    phOffset = 0.0;
    phSlope = 3.3 / 4095.0 * 3.5;  // ADC de 12 bits (0-4095)
    
    doOffset = 0.0;
    doSlope = 1.0;
    
    ecOffset = 0.0;
    ecSlope = 1.0;
    ecK = 10.0;  // K=10 como se especificó
    
    // Inicializar últimos valores
    lastPH = 7.0;
    lastDO = 0.0;
    lastEC = 0.0;
    
    // Inicializar arrays de lecturas
    readIndex = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
        phReadings[i] = 0;
        doReadings[i] = 0;
        ecReadings[i] = 0;
    }
}

void AnalogSensors::begin() {
    // Configurar pines como entradas
    pinMode(phPin, INPUT);
    pinMode(doPin, INPUT);
    pinMode(ecPin, INPUT);
    
    // Configuración específica para ESP32
    analogReadResolution(12);  // Configurar resolución ADC a 12 bits (0-4095)
    analogSetAttenuation(ADC_11db);  // Configurar atenuación para lecturas de 0-3.3V
}

int AnalogSensors::readRawPH() {
    // Leer valor crudo del sensor de pH
    return analogRead(phPin);
}

int AnalogSensors::readRawDO() {
    // Leer valor crudo del sensor de oxígeno disuelto
    return analogRead(doPin);
}

int AnalogSensors::readRawEC() {
    // Leer valor crudo del sensor de conductividad eléctrica
    return analogRead(ecPin);
}

float AnalogSensors::getPH() {
    // Leer valor crudo y promediar
    int rawValue = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
        rawValue += readRawPH();
        delay(10);
    }
    rawValue /= NUM_READINGS;
    
    // Convertir a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Convertir a pH (relación inversa típica: menor voltaje = mayor pH)
    // pH = 7 + ((2.5 - voltage) / 0.18)
    float pH = 7.0 + ((voltage - 2.5) / phSlope) + phOffset;
    
    lastPH = pH;
    return pH;
}

float AnalogSensors::getDO() {
    // Leer valor crudo y promediar
    int rawValue = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
        rawValue += readRawDO();
        delay(10);
    }
    rawValue /= NUM_READINGS;
    
    // Convertir a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Convertir a concentración de oxígeno disuelto (mg/L)
    // Esta es una fórmula simplificada, generalmente se usa una curva de calibración
    float doValue = voltage * doSlope + doOffset;
    
    lastDO = doValue;
    return doValue;
}

float AnalogSensors::getEC() {
    // Leer valor crudo y promediar
    int rawValue = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
        rawValue += readRawEC();
        delay(10);
    }
    rawValue /= NUM_READINGS;
    
    // Convertir a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Convertir a conductividad eléctrica (μS/cm)
    // EC = Voltaje * K * Factor de calibración
    float ecValue = voltage * ecK * ecSlope + ecOffset;
    
    // Normalmente se aplica compensación de temperatura
    // ecValue = compensateTemperature(ecValue, currentTemperature);
    
    lastEC = ecValue;
    return ecValue;
}

void AnalogSensors::calibratePH(float knownPH, int rawValue) {
    // Convertir el valor crudo a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Calcular offset para este punto
    // Si usamos más puntos, podríamos calcular tanto slope como offset
    phOffset = knownPH - (7.0 + ((voltage - 2.5) / phSlope));
}

void AnalogSensors::calibrateDO(float knownDO, int rawValue) {
    // Convertir el valor crudo a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Calibración simple de un punto (offset)
    doOffset = knownDO - (voltage * doSlope);
}

void AnalogSensors::calibrateEC(float knownEC, int rawValue) {
    // Convertir el valor crudo a voltaje
    float voltage = (rawValue * 3.3) / 4095.0;
    
    // Calibración simple de un punto (offset)
    ecOffset = knownEC - (voltage * ecK * ecSlope);
}

bool AnalogSensors::saveCalibration(ConfigStorage &storage) {
    bool success = true;
    
    // Guardar calibración de pH
    success &= storage.savePHCalibration(phOffset, phSlope);
    
    // Guardar calibración de DO
    success &= storage.saveDOCalibration(doOffset, doSlope);
    
    // Guardar calibración de EC
    success &= storage.saveECCalibration(ecOffset, ecSlope, ecK);
    
    return success;
}

bool AnalogSensors::loadCalibration(ConfigStorage &storage) {
    bool success = true;
    
    // Cargar calibración de pH
    success &= storage.loadPHCalibration(phOffset, phSlope);
    
    // Cargar calibración de DO
    success &= storage.loadDOCalibration(doOffset, doSlope);
    
    // Cargar calibración de EC
    success &= storage.loadECCalibration(ecOffset, ecSlope, ecK);
    
    return success;
}

void AnalogSensors::update() {
    // Leer y almacenar todos los valores de sensores
    phReadings[readIndex] = readRawPH();
    doReadings[readIndex] = readRawDO();
    ecReadings[readIndex] = readRawEC();
    
    // Avanzar índice
    readIndex = (readIndex + 1) % NUM_READINGS;
    
    // Calcular promedios y convertir a valores finales
    int avgPhRaw = getAverageReading(phReadings);
    int avgDoRaw = getAverageReading(doReadings);
    int avgEcRaw = getAverageReading(ecReadings);
    
    // Convertir a voltaje
    float phVoltage = (avgPhRaw * 3.3) / 4095.0;
    float doVoltage = (avgDoRaw * 3.3) / 4095.0;
    float ecVoltage = (avgEcRaw * 3.3) / 4095.0;
    
    // Calcular valores finales
    lastPH = 7.0 + ((phVoltage - 2.5) / phSlope) + phOffset;
    lastDO = doVoltage * doSlope + doOffset;
    lastEC = ecVoltage * ecK * ecSlope + ecOffset;
}

String AnalogSensors::getCSVData() const {
    String data = "";
    data += String(lastPH, 2) + ",";
    data += String(lastDO, 2) + ",";
    data += String(lastEC, 0) + ",";
    
    // Agregar también los valores crudos si están disponibles
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

float AnalogSensors::compensateTemperature(float ec, float temperature) {
    // Compensación estándar de EC por temperatura
    // EC25 = EC / (1 + 0.02 * (T - 25))
    // Puede implementarse si es necesario
    return NULL;
}