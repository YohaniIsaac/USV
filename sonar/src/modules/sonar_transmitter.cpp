#include "modules/sonar_transmitter.h"
#include "logger.h"

SonarTransmitter::SonarTransmitter() {
    dataloggerSerial = nullptr;
    transmissionInterval_ = 2000;     // 2000ms (2 segundos)
    lastTransmissionTime_ = 0;
    measurementCount_ = 0;
    
    // Inicializar array de mediciones
    resetMeasurements();
}

bool SonarTransmitter::begin() {
    // Configurar UART para comunicación con datalogger
    dataloggerSerial = &Serial2;  // Usar UART2 del ESP-WROOM
    dataloggerSerial->begin(DATALOGGER_BAUD_RATE, SERIAL_8N1, 
                           DATALOGGER_UART_RX_PIN, DATALOGGER_UART_TX_PIN);
    
    dataloggerSerial->setTimeout(100);
    
    LOG_INFO("SONAR_TX", "Transmisor inicializado:");
    LOG_INFO("SONAR_TX", "  Puerto: UART2");
    LOG_INFO("SONAR_TX", "  TX Pin: GPIO" + String(DATALOGGER_UART_TX_PIN));
    LOG_INFO("SONAR_TX", "  RX Pin: GPIO" + String(DATALOGGER_UART_RX_PIN));
    LOG_INFO("SONAR_TX", "  Baudios: " + String(DATALOGGER_BAUD_RATE));
    LOG_INFO("SONAR_TX", "  Intervalo TX: " + String(transmissionInterval_) + "ms");
    LOG_INFO("SONAR_TX", "  Recolección continua: SÍ");
    
    // Enviar mensaje de inicio
    delay(1000);  // Esperar a que datalogger esté listo
    dataloggerSerial->println("SONAR_TX_READY");
    
    return true;
}

void SonarTransmitter::update() {
    unsigned long currentTime = millis();
 
    // Verificar si es momento de transmitir
    if (currentTime - lastTransmissionTime_ >= transmissionInterval_) {
        calculateAndTransmitAverage();
    }
    
    // // Verificar si es momento de iniciar nuevo período de medición
    // if (!collectingData_ && (currentTime - lastTransmissionTime_ >= transmissionInterval_)) {
    //     startMeasurementPeriod();
    // }
    
    // // Verificar si terminó el período de recolección
    // if (collectingData_ && (currentTime - measurementStartTime_ >= averagingPeriod_)) {
    //     calculateAndTransmitAverage();
    // }
}

void SonarTransmitter::addSonarMeasurement(double depth, double offset, double range, 
                                          uint32_t totalLog, uint32_t tripLog, float temperature) {
    // Validar datos primero
    if (!isValidMeasurement(depth, offset, range, temperature)) {
        // LOG_DEBUG("SONAR_TX", "Medición inválida descartada");
        return;
    }
    
    // Verificar si tenemos espacio para más mediciones
    if (measurementCount_ >= MAX_MEASUREMENTS) {
        LOG_WARN("SONAR_TX", "Buffer lleno (" + String(MAX_MEASUREMENTS) + "), forzando transmisión");
        calculateAndTransmitAverage();  // Transmitir inmediatamente si buffer está lleno
    }
    
    // Agregar medición al buffer
    SonarData& measurement = measurements_[measurementCount_];
    measurement.depth = depth;
    measurement.offset = offset;
    measurement.range = range;
    measurement.totalLog = totalLog;
    measurement.tripLog = tripLog;
    measurement.temperature = temperature;
    measurement.valid = true;
    
    measurementCount_++;
    
    LOG_INFO("SONAR_TX", "Datos VÁLIDOS #" + String(measurementCount_) + 
            ": depth=" + String(depth, 2) + "m, offset=" + String(offset, 2) + 
            "m, range=" + String(range, 2) + "m, temp=" + String(temperature, 1) + 
            "°C, totalLog=" + String(totalLog) + ", tripLog=" + String(tripLog));}

void SonarTransmitter::calculateAndTransmitAverage() {    
    LOG_DEBUG("SONAR_TX", "Transmitiendo datos. Mediciones acumuladas: " + 
              String(measurementCount_));
    
    if (measurementCount_ == 0) {
        // Sin mediciones válidas - enviar packet de error
        SonarData errorData;
        errorData.depth = NAN;
        errorData.offset = NAN;
        errorData.range = NAN;
        errorData.totalLog = 0;
        errorData.tripLog = 0;
        errorData.temperature = NAN;
        errorData.valid = false;
        
        transmitData(errorData);
        LOG_WARN("SONAR_TX", "Sin mediciones válidas - enviando datos de error");
    } else {
        // Calcular promedio y transmitir
        SonarData avgData = calculateAverage();
        transmitData(avgData);
        
        LOG_INFO("SONAR_TX", "Promedio calculado y transmitido: depth=" + 
                 String(avgData.depth, 2) + "m, temp_agua=" + String(avgData.temperature, 1) + 
                 "°C (" + String(measurementCount_) + " mediciones)"); 
    }
    
    lastTransmissionTime_ = millis();
}

SonarTransmitter::SonarData SonarTransmitter::calculateAverage() {
    SonarData avgData;
    double sumDepth = 0.0;
    double sumOffset = 0.0;
    double sumRange = 0.0;
    uint64_t sumTotalLog = 0;  // Usar uint64_t para evitar overflow
    uint64_t sumTripLog = 0;
    double sumTemperature = 0.0; 
    int validCount = 0;
    int validTempCount = 0;


    // Sumar todas las mediciones válidas
    for (int i = 0; i < measurementCount_; i++) {
        if (measurements_[i].valid) {
            if (!isnan(measurements_[i].depth)) {
                sumDepth += measurements_[i].depth;
            }
            if (!isnan(measurements_[i].offset)) {
                sumOffset += measurements_[i].offset;
            }
            if (!isnan(measurements_[i].range)) {
                sumRange += measurements_[i].range;
            }
            
            sumTotalLog += measurements_[i].totalLog;
            sumTripLog += measurements_[i].tripLog;
            validCount++;

            if (!isnan(measurements_[i].temperature)) {
                sumTemperature += measurements_[i].temperature;
                validTempCount++;
            }
        }
    }
    
    // Calcular promedios
    if (validCount > 0) {
        avgData.depth = sumDepth / validCount;
        avgData.offset = sumOffset / validCount;
        avgData.range = sumRange / validCount;
        avgData.totalLog = (uint32_t)(sumTotalLog / validCount);
        avgData.tripLog = (uint32_t)(sumTripLog / validCount);
        avgData.valid = true;
    } else {
        avgData.depth = NAN;
        avgData.offset = NAN;
        avgData.range = NAN;
        avgData.totalLog = 0;
        avgData.tripLog = 0;
        avgData.valid = false;
    }

    if (validTempCount > 0) {
        avgData.temperature = sumTemperature / validTempCount;
    } else {
        avgData.temperature = NAN;
    }
    
    return avgData;
}

void SonarTransmitter::transmitData(const SonarData& data) {
    if (!dataloggerSerial) {
        LOG_ERROR("SONAR_TX", "Puerto serial no inicializado");
        return;
    }
    
    String packet = formatDataPacket(data);
    dataloggerSerial->println(packet);
    
    LOG_DEBUG("SONAR_TX", "Packet transmitido: " + packet);
}

String SonarTransmitter::formatDataPacket(const SonarData& data) {
    // Formato: SONAR,timestamp,depth,offset,range,totalLog,tripLog,temperature,valid,samples
    String packet = "SONAR,";
    packet += String(millis()) + ",";
    
    if (data.valid && !isnan(data.depth)) {
        packet += String(data.depth, 3);
    } else {
        packet += "NaN";
    }
    packet += ",";
    
    if (data.valid && !isnan(data.offset)) {
        packet += String(data.offset, 3);
    } else {
        packet += "NaN";
    }
    packet += ",";
    
    if (data.valid && !isnan(data.range)) {
        packet += String(data.range, 3);
    } else {
        packet += "NaN";
    }
    packet += ",";
    
    packet += String(data.totalLog) + ",";
    packet += String(data.tripLog) + ",";

    if (!isnan(data.temperature)) {
        packet += String(data.temperature, 1);
    } else {
        packet += "NaN";
    }
    packet += ",";

    packet += String(data.valid ? 1 : 0) + ",";
    packet += String(measurementCount_);
    
    return packet;
}

void SonarTransmitter::resetMeasurements() {
    measurementCount_ = 0;
    for (int i = 0; i < MAX_MEASUREMENTS; i++) {
        measurements_[i].valid = false;
        measurements_[i].depth = NAN;
        measurements_[i].offset = NAN;
        measurements_[i].range = NAN;
        measurements_[i].totalLog = 0;
        measurements_[i].tripLog = 0;
        measurements_[i].temperature = NAN;
    }
}

bool SonarTransmitter::isValidMeasurement(double depth, double offset, double range, float temperature) {
    // Verificar que al menos la profundidad sea válida
    if (isnan(depth)) {
        return false;
    }
    
    // Verificar rangos razonables (ajustar según tu aplicación)
    if (depth < 0.0 || depth > 1000.0) {  // Profundidad entre 0 y 1000m
        return false;
    }
    
    // Validar temperatura (rango razonable para sensor)
    if (!isnan(temperature)) {
        if (temperature < -40.0 || temperature > 125.0) {  // Rango razonable para sensor ESP32
            LOG_WARN("SONAR_TX", "Temperatura fuera de rango: " + String(temperature, 1) + "°C");
            // No invalidar la medición por temperatura, pero log el warning
        }
    }
    
    // El offset y range pueden ser NaN sin invalidar la medición
    return true;
}

// Métodos de configuración
void SonarTransmitter::setTransmissionInterval(unsigned long intervalMs) {
    transmissionInterval_ = intervalMs;
    LOG_INFO("SONAR_TX", "Intervalo de transmisión actualizado: " + 
             String(transmissionInterval_) + "ms (recolección continua)");
}

// Métodos de estado
bool SonarTransmitter::isConnected() const {
    return (dataloggerSerial != nullptr);
}

unsigned long SonarTransmitter::getLastTransmissionTime() const {
    return lastTransmissionTime_;
}

int SonarTransmitter::getMeasurementCount() const {
    return measurementCount_;
}