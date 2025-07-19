#include "modules/sonar_transmitter.h"
#include "logger.h"

SonarTransmitter::SonarTransmitter() {
    dataloggerSerial = nullptr;
    transmissionInterval_ = 2000;     // 2000ms (2 segundos)
    lastTransmissionTime_ = 0;
    
    // Inicializar buffer circular
    writeIndex_ = 0;
    validDataCount_ = 0;
    bufferFull_ = false;
    
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
    LOG_INFO("SONAR_TX", "  Buffer circular: " + String(MAX_MEASUREMENTS) + " elementos");
    
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
}

bool SonarTransmitter::isValidMeasurement(double depth, double offset, double range, float temperature) {
    // CLAVE: 0.0 es un valor VÁLIDO para profundidad
    bool hasValidDepth = !isnan(depth) && depth >= 0.0 && depth <= 1000.0;  // 0.0 ES VÁLIDO!
    bool hasValidTemp = !isnan(temperature) && temperature >= -40.0 && temperature <= 125.0;
    bool hasValidOffset = !isnan(offset) && offset >= -100.0 && offset <= 100.0;
    bool hasValidRange = !isnan(range) && range >= 0.0 && range <= 1000.0;  // 0.0 ES VÁLIDO!
    
    // Aceptar si al menos UN dato es válido
    bool isValid = hasValidDepth || hasValidTemp || hasValidOffset || hasValidRange;
    
    if (!isValid) {
        LOG_DEBUG("SONAR_TX", "Medición completamente inválida descartada");
    }
    
    return isValid;
}

void SonarTransmitter::addSonarMeasurement(double depth, double offset, double range, 
                                          uint32_t totalLog, uint32_t tripLog, float temperature) {
    
    // Validar que al menos un dato sea válido
    if (!isValidMeasurement(depth, offset, range, temperature)) {
        return;
    }
    
    // Preparar estructura de datos
    SonarData newData;
    
    // IMPORTANTE: Solo marcar como válido si el valor NO es NaN
    // 0.0 es un valor perfectamente válido!
    bool hasValidDepth = !isnan(depth) && depth >= 0.0 && depth <= 1000.0;
    bool hasValidTemp = !isnan(temperature) && temperature >= -40.0 && temperature <= 125.0;
    bool hasValidOffset = !isnan(offset) && offset >= -100.0 && offset <= 100.0;
    bool hasValidRange = !isnan(range) && range >= 0.0 && range <= 1000.0;
    
    // Almacenar solo valores válidos, el resto como NaN
    newData.depth = hasValidDepth ? depth : NAN;
    newData.offset = hasValidOffset ? offset : NAN;
    newData.range = hasValidRange ? range : NAN;
    newData.totalLog = totalLog;
    newData.tripLog = tripLog;
    newData.temperature = hasValidTemp ? temperature : NAN;
    newData.valid = true;  // Al menos un dato es válido
    
    // Agregar al buffer circular
    addToCircularBuffer(newData);
    
    // Log detallado de lo que se almacenó
    String logMsg = "Datos almacenados #" + String(validDataCount_) + 
                   " (pos=" + String((writeIndex_ - 1 + MAX_MEASUREMENTS) % MAX_MEASUREMENTS) + "): ";
    
    if (hasValidDepth) {
        logMsg += "depth=" + String(depth, 2) + "m ";
    }
    if (hasValidOffset) {
        logMsg += "offset=" + String(offset, 2) + "m ";
    }
    if (hasValidRange) {
        logMsg += "range=" + String(range, 2) + "m ";
    }
    if (hasValidTemp) {
        logMsg += "temp=" + String(temperature, 1) + "°C ";
    }
    logMsg += "totalLog=" + String(totalLog) + " tripLog=" + String(tripLog);
    
    LOG_INFO("SONAR_TX", logMsg);
}

void SonarTransmitter::addToCircularBuffer(const SonarData& data) {
    // Si la posición actual tenía datos válidos y la vamos a sobrescribir,
    // decrementar el contador (solo si el buffer está lleno)
    if (bufferFull_ && measurements_[writeIndex_].valid) {
        validDataCount_--;
    }
    
    // Almacenar el nuevo dato
    measurements_[writeIndex_] = data;
    
    // Incrementar contador de datos válidos
    validDataCount_++;
    
    // Avanzar índice circular
    writeIndex_++;
    if (writeIndex_ >= MAX_MEASUREMENTS) {
        writeIndex_ = 0;
        bufferFull_ = true;  // El buffer ha dado una vuelta completa
    }
    
    LOG_VERBOSE("SONAR_TX", "Buffer: writeIndex=" + String(writeIndex_) + 
                ", validData=" + String(validDataCount_) + 
                ", bufferFull=" + String(bufferFull_ ? "SÍ" : "NO"));
}

SonarTransmitter::SonarData SonarTransmitter::calculateAverage() {
    SonarData avgData;
    
    // Variables para acumular sumas
    double sumDepth = 0.0;
    double sumOffset = 0.0;
    double sumRange = 0.0;
    uint64_t sumTotalLog = 0;
    uint64_t sumTripLog = 0;
    double sumTemperature = 0.0;
    
    // Contadores independientes para cada variable
    int validDepthCount = 0;
    int validOffsetCount = 0;
    int validRangeCount = 0;
    int validTotalLogCount = 0;
    int validTripLogCount = 0;
    int validTempCount = 0;

    // Determinar cuántos datos procesar
    int dataToProcess = bufferFull_ ? MAX_MEASUREMENTS : writeIndex_;
    
    // Recorrer TODO el buffer y sumar solo valores válidos
    for (int i = 0; i < dataToProcess; i++) {
        if (measurements_[i].valid) {
            // CLAVE: Verificar específicamente que no sea NaN
            // 0.0 es un valor VÁLIDO y debe ser incluido!
            if (!isnan(measurements_[i].depth)) {
                sumDepth += measurements_[i].depth;
                validDepthCount++;
                LOG_VERBOSE("SONAR_TX", "Sumando depth[" + String(i) + "]=" + String(measurements_[i].depth, 2));
            }
            if (!isnan(measurements_[i].offset)) {
                sumOffset += measurements_[i].offset;
                validOffsetCount++;
            }
            if (!isnan(measurements_[i].range)) {
                sumRange += measurements_[i].range;
                validRangeCount++;
            }
            if (measurements_[i].totalLog > 0) {
                sumTotalLog += measurements_[i].totalLog;
                validTotalLogCount++;
            }
            if (measurements_[i].tripLog > 0) {
                sumTripLog += measurements_[i].tripLog;
                validTripLogCount++;
            }
            if (!isnan(measurements_[i].temperature)) {
                sumTemperature += measurements_[i].temperature;
                validTempCount++;
            }
        }
    }
    
    // Calcular promedios SOLO con datos válidos
    // IMPORTANTE: Si validDepthCount > 0, calcular promedio (incluso si es 0.0)
    avgData.depth = (validDepthCount > 0) ? (sumDepth / validDepthCount) : NAN;
    avgData.offset = (validOffsetCount > 0) ? (sumOffset / validOffsetCount) : NAN;
    avgData.range = (validRangeCount > 0) ? (sumRange / validRangeCount) : NAN;
    avgData.totalLog = (validTotalLogCount > 0) ? (uint32_t)(sumTotalLog / validTotalLogCount) : 0;
    avgData.tripLog = (validTripLogCount > 0) ? (uint32_t)(sumTripLog / validTripLogCount) : 0;
    avgData.temperature = (validTempCount > 0) ? (sumTemperature / validTempCount) : NAN;
    
    // Marcar como válido si al menos una variable tiene datos
    avgData.valid = (validDepthCount > 0 || validTempCount > 0 || 
                    validOffsetCount > 0 || validRangeCount > 0);
    
    // Log detallado del cálculo
    LOG_DEBUG("SONAR_TX", "Promedio calculado de " + String(dataToProcess) + " posiciones:");
    LOG_DEBUG("SONAR_TX", "  depth(" + String(validDepthCount) + ")=" + String(avgData.depth, 3));
    LOG_DEBUG("SONAR_TX", "  temp(" + String(validTempCount) + ")=" + String(avgData.temperature, 1));
    LOG_DEBUG("SONAR_TX", "  offset(" + String(validOffsetCount) + ")=" + String(avgData.offset, 3));
    LOG_DEBUG("SONAR_TX", "  range(" + String(validRangeCount) + ")=" + String(avgData.range, 3));
    
    return avgData;
}

void SonarTransmitter::calculateAndTransmitAverage() {    
    int totalData = bufferFull_ ? MAX_MEASUREMENTS : writeIndex_;
    
    LOG_DEBUG("SONAR_TX", "Transmitiendo datos. Buffer: " + String(totalData) + 
              " posiciones, " + String(validDataCount_) + " datos válidos");
    
    if (validDataCount_ == 0) {
        LOG_WARN("SONAR_TX", "Sin datos válidos para transmitir");
        lastTransmissionTime_ = millis();
        return;
    }
    
    // Calcular promedio y transmitir
    SonarData avgData = calculateAverage();
    
    if (avgData.valid) {
        transmitData(avgData);
        
        String logMsg = "Promedio transmitido: ";
        if (!isnan(avgData.depth)) {
            logMsg += "depth=" + String(avgData.depth, 2) + "m ";
        }
        if (!isnan(avgData.temperature)) {
            logMsg += "temp_agua=" + String(avgData.temperature, 1) + "°C ";
        }
        if (!isnan(avgData.offset)) {
            logMsg += "offset=" + String(avgData.offset, 2) + "m ";
        }
        if (!isnan(avgData.range)) {
            logMsg += "range=" + String(avgData.range, 2) + "m ";
        }
        logMsg += "(" + String(validDataCount_) + " datos válidos)";
        
        LOG_INFO("SONAR_TX", logMsg); 
    } else {
        LOG_WARN("SONAR_TX", "Datos calculados no válidos");
    }
    
    // NO resetear el buffer, mantener los datos para la próxima transmisión
    lastTransmissionTime_ = millis();
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
    
    // IMPORTANTE: Si depth no es NaN, enviarlo (incluso si es 0.0)
    if (!isnan(data.depth)) {
        packet += String(data.depth, 3);
    } else {
        packet += "NaN";
    }
    packet += ",";
    
    if (!isnan(data.offset)) {
        packet += String(data.offset, 3);
    } else {
        packet += "NaN";
    }
    packet += ",";
    
    if (!isnan(data.range)) {
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
    packet += String(validDataCount_);  // Número de datos válidos en buffer
    
    return packet;
}

void SonarTransmitter::resetMeasurements() {
    writeIndex_ = 0;
    validDataCount_ = 0;
    bufferFull_ = false;
    
    // Limpiar todo el buffer
    for (int i = 0; i < MAX_MEASUREMENTS; i++) {
        measurements_[i].valid = false;
        measurements_[i].depth = NAN;
        measurements_[i].offset = NAN;
        measurements_[i].range = NAN;
        measurements_[i].totalLog = 0;
        measurements_[i].tripLog = 0;
        measurements_[i].temperature = NAN;
    }
    
    LOG_DEBUG("SONAR_TX", "Buffer circular reiniciado");
}

// Métodos de configuración
void SonarTransmitter::setTransmissionInterval(unsigned long intervalMs) {
    transmissionInterval_ = intervalMs;
    LOG_INFO("SONAR_TX", "Intervalo de transmisión actualizado: " + 
             String(transmissionInterval_) + "ms");
}

// Métodos de estado
bool SonarTransmitter::isConnected() const {
    return (dataloggerSerial != nullptr);
}

unsigned long SonarTransmitter::getLastTransmissionTime() const {
    return lastTransmissionTime_;
}

int SonarTransmitter::getMeasurementCount() const {
    return validDataCount_;
}