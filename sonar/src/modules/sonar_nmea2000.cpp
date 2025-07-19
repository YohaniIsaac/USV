// Definición de pines CAN para ESP32
// #ifndef ESP32_CAN_TX_PIN
// #define ESP32_CAN_TX_PIN GPIO_NUM_2  // CAN TX pin (before GPIO_NUM_2) 
// #endif
// #ifndef ESP32_CAN_RX_PIN  
// #define ESP32_CAN_RX_PIN GPIO_NUM_4   // CAN RX pin (before GPIO_NUM_4)
// #endif

#include "modules/sonar_nmea2000.h"

// INCLUIR LAS LIBRERÍAS NMEA2000 SOLO AQUÍ, NO EN EL HEADER
#include <NMEA2000_CAN.h>
#include <N2kMessages.h>
#include <N2kMsg.h>



// Instancia estática para el callback
SonarNMEA2000* SonarNMEA2000::instance_ = nullptr;

SonarNMEA2000::SonarNMEA2000() {
    // Establecer la instancia estática para el callback
    instance_ = this;
    
    // Inicializar variables
    lastDepth_ = NAN;
    lastOffset_ = NAN;
    lastRange_ = NAN;
    lastTotalLog_ = 0;
    lastTripLog_ = 0;
    lastTemperature_ = NAN; 
    
    depthDataValid_ = false;
    logDataValid_ = false;
    rawMessagesEnabled_ = false;
    initialized_ = false;
    lastDataTime_ = 0;
}

bool SonarNMEA2000::setup() {
    // Inicializar puerto serial
    LOG_INFO("SONAR", "NMEA2000 Sonar Reader iniciado");
    
    // Configurar handler de mensajes NMEA2000
    NMEA2000.SetMsgHandler(staticMessageHandler);
    
    // Abrir puerto NMEA2000
    if (!NMEA2000.Open()) {
        LOG_ERROR("SONAR", "Error: No se pudo abrir el puerto NMEA2000");
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("SONAR", "Esperando datos del sonar...");
    
    return true;
}

void SonarNMEA2000::update() {
    if (!initialized_) {
        return;
    }
    
    // Procesar mensajes NMEA2000
    NMEA2000.ParseMessages();

    // Verificar timeout de datos
    unsigned long currentTime = millis();
    if (lastDataTime_ > 0 && (currentTime - lastDataTime_ > 10000)) {
        LOG_WARN("SONAR", "Timeout - Sin datos por más de 10 segundos");
        depthDataValid_ = false;
        logDataValid_ = false;
    }
}

// Procesar datos de temperatura del sonar
void SonarNMEA2000::processTemperature(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    unsigned char TempInstance;
    tN2kTempSource TempSource;
    double ActualTemperature;
    double SetTemperature;
    
    if (ParseN2kTemperature(N2kMsg, SID, TempInstance, TempSource, ActualTemperature, SetTemperature)) {
        // Solo procesar si es temperatura del agua (Sea Temperature)
        if (TempSource == N2kts_SeaTemperature || TempSource == N2kts_OutsideTemperature) {
            if (ActualTemperature != N2kDoubleNA) {
                lastTemperature_ = KelvinToC(ActualTemperature);  // Convertir de Kelvin a Celsius
                LOG_DEBUG("SONAR", "Temperatura del agua actualizada: " + String(lastTemperature_, 1) + "°C");
            }
        }
    } else {
        LOG_WARN("SONAR", "Error al parsear datos de temperatura");
    }
}

void SonarNMEA2000::show_data() {
    LOG_INFO("SONAR", "=============== DATOS DEL SONAR ===============");
    
    if (hasValidDepthData()) {
        LOG_INFO("SONAR", "📏 PROFUNDIDAD:");
        if (!isnan(lastDepth_)) {
            LOG_INFO("SONAR", "  Profundidad: " + String(lastDepth_, 2) + " m");
        } else {
            LOG_INFO("SONAR", "  Profundidad: N/A");
        }
        
        if (!isnan(lastOffset_)) {
            LOG_INFO("SONAR", "  Offset: " + String(lastOffset_, 2) + " m");
        } else {
            LOG_INFO("SONAR", "  Offset: N/A");
        }
        
        if (!isnan(lastRange_)) {
            LOG_INFO("SONAR", "  Rango: " + String(lastRange_, 2) + " m");
        } else {
            LOG_INFO("SONAR", "  Rango: N/A");
        }
    } else {
        LOG_WARN("SONAR", " Sin datos válidos de profundidad");
    }
    
    if (hasValidLogData()) {
        LOG_INFO("SONAR", " LOG DE DISTANCIA:");
        if (lastTotalLog_ != 0) {
            LOG_INFO("SONAR", "  Log Total: " + String(lastTotalLog_) + " m");
        } else {
            LOG_INFO("SONAR", "  Log Total: N/A");
        }
        
        if (lastTripLog_ != 0) {
            LOG_INFO("SONAR", "  Trip Log: " + String(lastTripLog_) + " m");
        } else {
            LOG_INFO("SONAR", "  Trip Log: N/A");
        }
    } else {
        LOG_WARN("SONAR", " Sin datos válidos de log");
    }

    // Mostrar temperatura del agua
    LOG_INFO("SONAR", " TEMPERATURA DEL AGUA:");
    if (!isnan(lastTemperature_)) {
        LOG_INFO("SONAR", "  Temperatura: " + String(lastTemperature_, 1) + " °C");
    } else {
        LOG_INFO("SONAR", "  Temperatura: N/A");
    }

    LOG_INFO("SONAR", "=============================================");
}

// Getters
double SonarNMEA2000::getDepth() const {
    return lastDepth_;
}

double SonarNMEA2000::getOffset() const {
    return lastOffset_;
}

double SonarNMEA2000::getRange() const {
    return lastRange_;
}

uint32_t SonarNMEA2000::getTotalLog() const {
    return lastTotalLog_;
}

uint32_t SonarNMEA2000::getTripLog() const {
    return lastTripLog_;
}

float SonarNMEA2000::getTemperature() const {
    return lastTemperature_;
}

bool SonarNMEA2000::hasValidDepthData() const {
    return depthDataValid_;
}

bool SonarNMEA2000::hasValidLogData() const {
    return logDataValid_;
}

bool SonarNMEA2000::isInitialized() const {
    return initialized_;
}

void SonarNMEA2000::enableRawMessages(bool enable) {
    rawMessagesEnabled_ = enable;
    LOG_INFO("SONAR", "Mensajes raw " + String(enable ? "habilitados" : "deshabilitados"));
}

String SonarNMEA2000::getCSVHeader() const {
    return "Profundidad,Offset,Rango,LogTotal,TripLog, TemperaturaAgua";
}

String SonarNMEA2000::getCSVData() const {
    String data = "";
    
    // Profundidad
    if (!isnan(lastDepth_)) {
        data += String(lastDepth_, 2);
    } else {
        data += "N/A";
    }
    data += ",";
    
    // Offset
    if (!isnan(lastOffset_)) {
        data += String(lastOffset_, 2);
    } else {
        data += "N/A";
    }
    data += ",";
    
    // Rango
    if (!isnan(lastRange_)) {
        data += String(lastRange_, 2);
    } else {
        data += "N/A";
    }
    data += ",";
    
    // Log Total
    if (lastTotalLog_ != 0) {
        data += String(lastTotalLog_);
    } else {
        data += "N/A";
    }
    data += ",";
    
    // Trip Log
    if (lastTripLog_ != 0) {
        data += String(lastTripLog_);
    } else {
        data += "N/A";
    }

    // Temperatura
    if (!isnan(lastTemperature_)) {
        data += String(lastTemperature_, 1);
    } else {
        data += "N/A";
    }

    return data;
}

// Métodos privados
void SonarNMEA2000::staticMessageHandler(const tN2kMsg &N2kMsg) {
    if (instance_ != nullptr) {
        instance_->handleNMEA2000Message(N2kMsg);
    }
}

void SonarNMEA2000::handleNMEA2000Message(const tN2kMsg &N2kMsg) {
    lastDataTime_ = millis();  // Actualizar timestamp de último dato
    
    switch (N2kMsg.PGN) {
        case 128267L: // Water Depth
            processDepthData(N2kMsg);
            break;
        case 128275L: // Distance Log
            processDistanceLog(N2kMsg);
            break;
        case 130316L: // Temperature Extended Range
        case 130312L: // Temperature
            LOG_INFO("SONAR", " TEMPERATURE recibido!");
            processTemperature(N2kMsg);
            break;
        default:
            // Mostrar otros mensajes si está habilitado
            if (rawMessagesEnabled_) {
                printRawMessage(N2kMsg);
            }
            break;
    }
}

void SonarNMEA2000::processDepthData(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    double depthBelowTransducer;
    double offset;
    double range;
    
    if (ParseN2kWaterDepth(N2kMsg, SID, depthBelowTransducer, offset, range)) {
        // Actualizar valores internos
        lastDepth_ = (depthBelowTransducer != N2kDoubleNA) ? depthBelowTransducer : NAN;
        lastOffset_ = (offset != N2kDoubleNA) ? offset : NAN;
        lastRange_ = (range != N2kDoubleNA) ? range : NAN;
        depthDataValid_ = true;
        
        LOG_DEBUG("SONAR", "Datos de profundidad actualizados - Depth: " + 
                  String(!isnan(lastDepth_) ? lastDepth_ : 0, 2) + "m");
    } else {
        depthDataValid_ = false;
        LOG_WARN("SONAR", "Error al parsear datos de profundidad");
    }
}

void SonarNMEA2000::processDistanceLog(const tN2kMsg &N2kMsg) {
    uint16_t daysSince1970;
    double secondsSinceMidnight;
    uint32_t log;
    uint32_t tripLog;
    
    if (ParseN2kDistanceLog(N2kMsg, daysSince1970, secondsSinceMidnight, log, tripLog)) {
        // Actualizar valores internos
        lastTotalLog_ = (log != N2kUInt32NA) ? log : 0;
        lastTripLog_ = (tripLog != N2kUInt32NA) ? tripLog : 0;
        logDataValid_ = true;
        
        LOG_DEBUG("SONAR", "Datos de log actualizados - Total: " + String(lastTotalLog_) + 
                  "m, Trip: " + String(lastTripLog_) + "m");
    } else {
        logDataValid_ = false;
        LOG_WARN("SONAR", "Error al parsear datos de log");
    }
}

void SonarNMEA2000::printRawMessage(const tN2kMsg &N2kMsg) {
    String message = "PGN: " + String(N2kMsg.PGN) + 
                    " | Source: " + String(N2kMsg.Source) + 
                    " | Data: ";
    
    for (int i = 0; i < N2kMsg.DataLen; i++) {
        if (N2kMsg.Data[i] < 16) message += "0";
        message += String(N2kMsg.Data[i], HEX);
        message += " ";
    }
    
    LOG_VERBOSE("SONAR", message);
}