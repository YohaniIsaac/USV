#include "modules/sonar.h"
#include "logger.h"

// Definición de pines CAN para ESP32
#define ESP32_CAN_TX_PIN GPIO_NUM_2  // CAN TX pin
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // CAN RX pin

// Instancia estática para el callback
NMEA2000SonarReader* NMEA2000SonarReader::instance_ = nullptr;

NMEA2000SonarReader::NMEA2000SonarReader()
    : lastDepth_(NAN)
    , lastOffset_(NAN)
    , lastRange_(NAN)
    , lastTotalLog_(0)
    , lastTripLog_(0)
    , depthDataValid_(false)
    , logDataValid_(false)
    , rawMessagesEnabled_(false)
    , initialized_(false)
{
    // Establecer la instancia estática para el callback
    instance_ = this;
}

NMEA2000SonarReader::~NMEA2000SonarReader() {
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool NMEA2000SonarReader::begin(uint32_t baudRate) {
    if (initialized_) {
        return true;
    }

    // Inicializar puerto serial
    Serial.begin(baudRate);
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

void NMEA2000SonarReader::update() {
    if (!initialized_) {
        return;
    }
    
    // Procesar mensajes NMEA2000
    NMEA2000.ParseMessages();
}

void NMEA2000SonarReader::enableRawMessages(bool enable) {
    rawMessagesEnabled_ = enable;
    LOG_INFO("SONAR", "Mensajes raw ");
    LOG_INFO("SONAR", enable ? "habilitados" : "deshabilitados");
}

double NMEA2000SonarReader::getLastDepth() const {
    return lastDepth_;
}

double NMEA2000SonarReader::getLastOffset() const {
    return lastOffset_;
}

double NMEA2000SonarReader::getLastRange() const {
    return lastRange_;
}

uint32_t NMEA2000SonarReader::getLastTotalLog() const {
    return lastTotalLog_;
}

uint32_t NMEA2000SonarReader::getLastTripLog() const {
    return lastTripLog_;
}

bool NMEA2000SonarReader::hasValidDepthData() const {
    return depthDataValid_;
}

bool NMEA2000SonarReader::hasValidLogData() const {
    return logDataValid_;
}

void NMEA2000SonarReader::staticMessageHandler(const tN2kMsg &N2kMsg) {
    if (instance_ != nullptr) {
        instance_->handleNMEA2000Message(N2kMsg);
    }
}

void NMEA2000SonarReader::handleNMEA2000Message(const tN2kMsg &N2kMsg) {
    switch (N2kMsg.PGN) {
        case 128267L: // Water Depth
            processDepthData(N2kMsg);
            break;
        case 128275L: // Distance Log
            processDistanceLog(N2kMsg);
            break;
        default:
            // Mostrar otros mensajes si está habilitado
            if (rawMessagesEnabled_) {
                printRawMessage(N2kMsg);
            }
            break;
    }
}

void NMEA2000SonarReader::processDepthData(const tN2kMsg &N2kMsg) {
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
        
        // Imprimir datos de profundidad
        LOG_INFO("SONAR", "Profundidad: ");
        if (!isnan(lastDepth_)) {
            LOG_INFO("SONAR", lastDepth_);
            LOG_INFO("SONAR", " m");
        } else {
            LOG_INFO("SONAR", "N/A");
        }
        
        LOG_INFO("SONAR", " | Offset: ");
        if (!isnan(lastOffset_)) {
            LOG_INFO("SONAR", lastOffset_);
            LOG_INFO("SONAR", " m");
        } else {
            LOG_INFO("SONAR", "N/A");
        }
        
        LOG_INFO("SONAR", " | Rango: ");
        if (!isnan(lastRange_)) {
            LOG_INFO("SONAR", lastRange_);
            LOG_INFO("SONAR", " m");
        } else {
            LOG_INFO("SONAR", "N/A");
        }
    } else {
        depthDataValid_ = false;
    }
}

void NMEA2000SonarReader::processDistanceLog(const tN2kMsg &N2kMsg) {
    uint16_t daysSince1970;
    double secondsSinceMidnight;
    uint32_t log;
    uint32_t tripLog;
    
    if (ParseN2kDistanceLog(N2kMsg, daysSince1970, secondsSinceMidnight, log, tripLog)) {
        // Actualizar valores internos
        lastTotalLog_ = (log != N2kUInt32NA) ? log : 0;
        lastTripLog_ = (tripLog != N2kUInt32NA) ? tripLog : 0;
        logDataValid_ = true;
        
        // Imprimir datos del log
        LOG_INFO("SONAR", "Log Total: ");
        if (lastTotalLog_ != 0) {
            LOG_INFO("SONAR", lastTotalLog_);
            LOG_INFO("SONAR", " m");
        } else {
            LOG_INFO("SONAR", "N/A");
        }
        
        LOG_INFO("SONAR", " | Trip Log: ");
        if (lastTripLog_ != 0) {
            LOG_INFO("SONAR", lastTripLog_);
            LOG_INFO("SONAR", " m");
        } else {
            LOG_INFO("SONAR", "N/A");
        }
    } else {
        logDataValid_ = false;
    }
}

void NMEA2000SonarReader::printRawMessage(const tN2kMsg &N2kMsg) {
    LOG_INFO("SONAR", "PGN: ");
    LOG_INFO("SONAR", N2kMsg.PGN);
    LOG_INFO("SONAR", " | Source: ");
    LOG_INFO("SONAR", N2kMsg.Source);
    LOG_INFO("SONAR", "| Data: ");
    
    for (int i = 0; i < N2kMsg.DataLen; i++) {
        if (N2kMsg.Data[i] < 16) Serial.print("0");
        LOG_INFO("SONAR", N2kMsg.Data[i], HEX);
    }
}