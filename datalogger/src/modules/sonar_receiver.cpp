#include "modules/sonar_receiver.h"
#include "logger.h"

SonarReceiver::SonarReceiver() {
    wroomSerial = nullptr;
    connected_ = false;
    lastDataTime_ = 0;
    connectionTimeout_ = 5000;  // 5 segundos de timeout
    
    totalPacketsReceived_ = 0;
    validPacketsReceived_ = 0;
    errorPacketsReceived_ = 0;
    
    inputBuffer_ = "";
    
    resetData();
}

bool SonarReceiver::begin() {
    // Configurar UART para recibir datos del ESP-WROOM
    wroomSerial = &Serial1;  // Usar UART1 del ESP32-S3
    wroomSerial->begin(WROOM_BAUD_RATE, SERIAL_8N1, WROOM_UART_RX_PIN, WROOM_UART_TX_PIN);
    wroomSerial->setTimeout(100);
    
    LOG_INFO("SONAR_RX", "Receptor inicializado:");
    LOG_INFO("SONAR_RX", "  Puerto: UART1");
    LOG_INFO("SONAR_RX", "  RX Pin: GPIO" + String(WROOM_UART_RX_PIN));
    LOG_INFO("SONAR_RX", "  Baudios: " + String(WROOM_BAUD_RATE));
    LOG_INFO("SONAR_RX", "  Timeout conexión: " + String(connectionTimeout_) + "ms");
    
    LOG_INFO("SONAR_RX", "Esperando datos del ESP-WROOM...");
    
    return true;
}

void SonarReceiver::update() {
    processIncomingData();
    updateConnectionStatus();
}

void SonarReceiver::processIncomingData() {
    if (!wroomSerial) {
        return;
    }
    
    // Leer datos disponibles
    while (wroomSerial->available()) {
        char c = wroomSerial->read();
        
        if (c == '\n' || c == '\r') {
            // Fin de línea - procesar packet
            if (inputBuffer_.length() > 0) {
                totalPacketsReceived_++;
                
                LOG_VERBOSE("SONAR_RX", "Packet recibido: " + inputBuffer_);
                
                if (parsePacket(inputBuffer_)) {
                    validPacketsReceived_++;
                    lastDataTime_ = millis();
                    connected_ = true;
                    LOG_DEBUG("SONAR_RX", "Packet válido procesado");
                } else {
                    errorPacketsReceived_++;
                    LOG_WARN("SONAR_RX", "Error al parsear packet: " + inputBuffer_);
                }
                
                inputBuffer_ = "";
            }
        } else {
            // Agregar caracter al buffer
            inputBuffer_ += c;
            
            // Prevenir buffer overflow
            if (inputBuffer_.length() > 200) {
                LOG_WARN("SONAR_RX", "Buffer overflow - reseteando");
                inputBuffer_ = "";
                errorPacketsReceived_++;
            }
        }
    }
}

bool SonarReceiver::parsePacket(const String& packet) {
    // Formato esperado: SONAR,timestamp,depth,offset,range,totalLog,tripLog,valid,samples
    
    // Verificar que empiece con "SONAR"
    if (!packet.startsWith("SONAR,")) {
        // Pueden llegar mensajes de control como "SONAR_TX_READY"
        if (packet == "SONAR_TX_READY") {
            LOG_INFO("SONAR_RX", "ESP-WROOM listo para transmisión");
            return true;
        }
        return false;
    }
    
    // Dividir packet en campos
    int fieldCount = 0;
    int startIndex = 6;  // Después de "SONAR,"
    String fields[8];    // Esperamos 8 campos después de "SONAR"
    
    while (fieldCount < 8 && startIndex < packet.length()) {
        int commaIndex = packet.indexOf(',', startIndex);
        
        if (commaIndex == -1) {
            // Último campo
            fields[fieldCount] = packet.substring(startIndex);
            fieldCount++;
            break;
        } else {
            fields[fieldCount] = packet.substring(startIndex, commaIndex);
            fieldCount++;
            startIndex = commaIndex + 1;
        }
    }
    
    // Verificar que tenemos todos los campos
    if (fieldCount != 8) {
        LOG_WARN("SONAR_RX", "Packet incompleto - campos: " + String(fieldCount) + "/8");
        return false;
    }
    
    // Parsear campos
    try {
        currentData_.timestamp = strtoul(fields[0].c_str(), nullptr, 10);
        currentData_.depth = parseDoubleValue(fields[1]);
        currentData_.offset = parseDoubleValue(fields[2]);
        currentData_.range = parseDoubleValue(fields[3]);
        currentData_.totalLog = parseUInt32Value(fields[4]);
        currentData_.tripLog = parseUInt32Value(fields[5]);
        currentData_.valid = (fields[6].toInt() == 1);
        currentData_.sampleCount = fields[7].toInt();
        currentData_.receivedTime = millis();
        
        LOG_INFO("SONAR_RX", "Datos actualizados - Depth: " + 
                 String(isnan(currentData_.depth) ? 0 : currentData_.depth, 2) + 
                 "m, Samples: " + String(currentData_.sampleCount));
        
        return true;
        
    } catch (...) {
        LOG_ERROR("SONAR_RX", "Error al convertir campos numéricos");
        return false;
    }
}

void SonarReceiver::updateConnectionStatus() {
    unsigned long currentTime = millis();
    
    // Verificar timeout de conexión
    if (lastDataTime_ > 0 && (currentTime - lastDataTime_ > connectionTimeout_)) {
        if (connected_) {
            connected_ = false;
            LOG_WARN("SONAR_RX", "Conexión perdida - timeout de " + 
                     String(connectionTimeout_) + "ms");
            resetData();
        }
    }
    
    // Log estadísticas periódicamente
    static unsigned long lastStatsTime = 0;
    if (currentTime - lastStatsTime > 30000) {  // Cada 30 segundos
        logPacketStats();
        lastStatsTime = currentTime;
    }
}

void SonarReceiver::logPacketStats() {
    LOG_INFO("SONAR_RX", "=== ESTADÍSTICAS DE RECEPCIÓN ===");
    LOG_INFO("SONAR_RX", "Total packets: " + String(totalPacketsReceived_));
    LOG_INFO("SONAR_RX", "Válidos: " + String(validPacketsReceived_));
    LOG_INFO("SONAR_RX", "Errores: " + String(errorPacketsReceived_));
    
    if (totalPacketsReceived_ > 0) {
        float successRate = (float)validPacketsReceived_ / totalPacketsReceived_ * 100.0;
        LOG_INFO("SONAR_RX", "Tasa de éxito: " + String(successRate, 1) + "%");
    }
    
    LOG_INFO("SONAR_RX", "Conectado: " + String(connected_ ? "Sí" : "No"));
    LOG_INFO("SONAR_RX", "===============================");
}

double SonarReceiver::parseDoubleValue(const String& value) {
    if (value == "NaN" || value.length() == 0) {
        return NAN;
    }
    return value.toDouble();
}

uint32_t SonarReceiver::parseUInt32Value(const String& value) {
    if (value.length() == 0) {
        return 0;
    }
    return strtoul(value.c_str(), nullptr, 10);
}

void SonarReceiver::resetData() {
    currentData_.depth = NAN;
    currentData_.offset = NAN;
    currentData_.range = NAN;
    currentData_.totalLog = 0;
    currentData_.tripLog = 0;
    currentData_.timestamp = 0;
    currentData_.sampleCount = 0;
    currentData_.valid = false;
    currentData_.receivedTime = 0;
}

// Getters
double SonarReceiver::getDepth() const {
    return currentData_.depth;
}

double SonarReceiver::getOffset() const {
    return currentData_.offset;
}

double SonarReceiver::getRange() const {
    return currentData_.range;
}

uint32_t SonarReceiver::getTotalLog() const {
    return currentData_.totalLog;
}

uint32_t SonarReceiver::getTripLog() const {
    return currentData_.tripLog;
}

bool SonarReceiver::hasValidData() const {
    return connected_ && currentData_.valid;
}

bool SonarReceiver::isConnected() const {
    return connected_;
}

unsigned long SonarReceiver::getLastDataTime() const {
    return lastDataTime_;
}

unsigned long SonarReceiver::getDataTimestamp() const {
    return currentData_.timestamp;
}

int SonarReceiver::getSampleCount() const {
    return currentData_.sampleCount;
}

unsigned long SonarReceiver::getTotalPacketsReceived() const {
    return totalPacketsReceived_;
}

unsigned long SonarReceiver::getValidPacketsReceived() const {
    return validPacketsReceived_;
}

unsigned long SonarReceiver::getErrorPacketsReceived() const {
    return errorPacketsReceived_;
}

String SonarReceiver::getCSVHeader() const {
    return "SonarDepth,SonarValid";
}

String SonarReceiver::getCSVData() const {
    String data = "";
    
    if (hasValidData()) {
        data += String(isnan(currentData_.depth) ? 0 : currentData_.depth, 3) + ",";
        // data += String(isnan(currentData_.offset) ? 0 : currentData_.offset, 3) + ",";
        // data += String(isnan(currentData_.range) ? 0 : currentData_.range, 3) + ",";
        // data += String(currentData_.totalLog) + ",";
        // data += String(currentData_.tripLog) + ",";
        // data += String(currentData_.sampleCount) + ",";
        data += String(currentData_.valid ? 1 : 0);
    } else {
        data += "NaN,0";
    }
    
    return data;
}

void SonarReceiver::showStatus() const {
    LOG_INFO("SONAR_RX", "============ ESTADO DEL SONAR ============");
    LOG_INFO("SONAR_RX", "Conectado: " + String(connected_ ? "Sí" : "No"));
    
    if (hasValidData()) {
        LOG_INFO("SONAR_RX", "📏 DATOS VÁLIDOS:");
        if (!isnan(currentData_.depth)) {
            LOG_INFO("SONAR_RX", "  Profundidad: " + String(currentData_.depth, 2) + " m");
        }
        if (!isnan(currentData_.offset)) {
            LOG_INFO("SONAR_RX", "  Offset: " + String(currentData_.offset, 2) + " m");
        }
        if (!isnan(currentData_.range)) {
            LOG_INFO("SONAR_RX", "  Rango: " + String(currentData_.range, 2) + " m");
        }
        LOG_INFO("SONAR_RX", "  Muestras promediadas: " + String(currentData_.sampleCount));
        
        unsigned long dataAge = millis() - currentData_.receivedTime;
        LOG_INFO("SONAR_RX", "  Edad del dato: " + String(dataAge) + " ms");
    } else {
        LOG_WARN("SONAR_RX", "❌ Sin datos válidos");
    }
    
    LOG_INFO("SONAR_RX", "📊 ESTADÍSTICAS:");
    LOG_INFO("SONAR_RX", "  Packets totales: " + String(totalPacketsReceived_));
    LOG_INFO("SONAR_RX", "  Packets válidos: " + String(validPacketsReceived_));
    LOG_INFO("SONAR_RX", "  Packets con error: " + String(errorPacketsReceived_));
    
    LOG_INFO("SONAR_RX", "==========================================");
}