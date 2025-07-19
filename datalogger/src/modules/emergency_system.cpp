#include "modules/emergency_system.h"
#include "modules/pixhawk_interface.h" 
#include "logger.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


// Constructor
EmergencySystem::EmergencySystem() {
    // Inicializar el array manualmente
    txAddress[0] = 'E';
    txAddress[1] = 'M';
    txAddress[2] = 'R';
    txAddress[3] = 'G';
    txAddress[4] = '1';
    txAddress[5] = '\0';

    emergencyActive = false;
    gpsInitialized = false;
    nrfInitialized = false;
    lastCheckTime = 0;
    lastGPSReadTime = 0;
    lastTransmitTime = 0;

    // Inicializar variables de voltaje
    currentVoltage = 0.0;
    powerControlActive = true;  // Por defecto, alimentación activa
    voltageReadingIndex = 0;
    voltageBufferFull = false;

    // Inicializar variables de seguridad de conmutación
    // stateChangeTime = millis();
    // pendingStateChange = false;
    // targetEmergencyState = false;
    // conditionStartTime = 0;
    // conditionMet = false;

    // Inicializar buffer de voltaje
    for (int i = 0; i < EMERGENCY_VOLTAGE_SAMPLES; i++) {
        voltageReadings[i] = 0.0;
    }

    currentLocation.latitude = 0.0;
    currentLocation.longitude = 0.0;
    currentLocation.altitude = 0.0;
    currentLocation.satellites = 0;
    currentLocation.valid = false;

    radio = nullptr;
    hspi = nullptr;
    pixhawkInterface = nullptr;
}

// Destructor
EmergencySystem::~EmergencySystem() {
    // Limpiar recursos si están activos
    if (gpsInitialized) {
        Serial1.end();
    }
    
    if (radio) {
        radio->powerDown();
        delete radio;
    }
    
    if (hspi) {
        delete hspi;
    }
}

void EmergencySystem::begin() {
    // Configurar pin analógico para lectura de voltaje
    pinMode(EMERGENCY_VOLTAGE_PIN, INPUT);

    // Configurar pin de control de alimentación como salida
    pinMode(EMERGENCY_POWER_CONTROL_PIN, OUTPUT);
    setPowerControl(true);  // Iniciar con alimentación normal (HIGH)

    // Configuración específica para ESP32
    analogReadResolution(12);  // 12 bits (0-4095)
    analogSetAttenuation(ADC_11db);  // 0-3.3V

    LOG_INFO("EMERGENCY", "Sistema de emergencia inicializado");
    LOG_INFO("EMERGENCY", "Pin de voltaje: GPIO" + String(EMERGENCY_VOLTAGE_PIN));
    LOG_INFO("EMERGENCY", "Pin de control alimentación: GPIO" + String(EMERGENCY_POWER_CONTROL_PIN));
    LOG_INFO("EMERGENCY", "Umbral de emergencia: " + String(EMERGENCY_VOLTAGE_THRESHOLD_REAL, 2) + "V");
    LOG_INFO("EMERGENCY", "Histéresis: " + String(EMERGENCY_VOLTAGE_HYSTERESIS_REAL, 2) + "V");
}

void EmergencySystem::update() {
    unsigned long currentTime = millis();
    
    // Verificar estado
    if (currentTime - lastCheckTime >= EMERGENCY_CHECK_RATE) {
        LOG_INFO("EMERGENCY", "Verificando estado del pin");
        checkVoltageLevel();
        lastCheckTime = currentTime;
    }


    // Si está en emergencia, leer GPS y enviar datos
    if (emergencyActive) {
        // Leer GPS continuamente
        readGPSData();
        
        // Enviar datos cada 5 segundos si tenemos ubicación válida
        if (currentTime - lastTransmitTime >= 5000) {
            if (currentLocation.valid) {
                sendNRFPacket();
            } else {
                LOG_WARN("EMERGENCY", "Esperando señal GPS válida...");
            }
            lastTransmitTime = currentTime;
        }
    }
}

void EmergencySystem::checkVoltageLevel() {
    // Leer voltaje actual
    currentVoltage = readAverageVoltage();

    // Actualizar buffer de voltaje
    updateVoltageBuffer(currentVoltage);

    LOG_VERBOSE("EMERGENCY", "Voltaje actual: " + String(currentVoltage, 3) + "V");

     // Lógica de activación/desactivación con histéresis
    if (!emergencyActive) {
        // No estamos en emergencia - verificar si voltaje baja del umbral
        if (currentVoltage < EMERGENCY_VOLTAGE_THRESHOLD_REAL) {
            LOG_WARN("EMERGENCY", "¡VOLTAJE BAJO DETECTADO! " + 
                     String(currentVoltage, 3) + "V < " + String(EMERGENCY_VOLTAGE_THRESHOLD_REAL, 2) + "V");
            activateEmergency();
        }
    } else {
        // Estamos en emergencia - verificar si voltaje se recupera (con histéresis)
        float recoveryThreshold = EMERGENCY_VOLTAGE_THRESHOLD_REAL + EMERGENCY_VOLTAGE_HYSTERESIS_REAL;
        if (currentVoltage > recoveryThreshold) {
            LOG_INFO("EMERGENCY", "Voltaje recuperado: " + 
                     String(currentVoltage, 3) + "V > " + String(recoveryThreshold, 2) + "V");
            deactivateEmergency();
        }
    }
}

float EmergencySystem::readAverageVoltage() {
    // Realizar múltiples lecturas para mayor estabilidad
    int totalReading = 0;
    const int numSamples = 10;
    
    for (int i = 0; i < numSamples; i++) {
        totalReading += analogRead(EMERGENCY_VOLTAGE_PIN);
        delayMicroseconds(100);  // Pequeña pausa entre lecturas
    }
    
    float averageReading = totalReading / (float)numSamples;
    
    // Convertir a voltaje (ESP32: 12 bits, 3.3V referencia)
    float adcVoltage = (averageReading * 3.3) / 4095.0;

    // Convertir a voltaje real del sistema aplicando factor de escala
    float realVoltage = adcVoltage * VOLTAGE_SCALE_FACTOR;

    return realVoltage;
}

void EmergencySystem::updateVoltageBuffer(float newReading) {
    voltageReadings[voltageReadingIndex] = newReading;
    voltageReadingIndex = (voltageReadingIndex + 1) % EMERGENCY_VOLTAGE_SAMPLES;
    
    if (voltageReadingIndex == 0) {
        voltageBufferFull = true;
    }
}
void EmergencySystem::activateEmergency() {
    LOG_ERROR("EMERGENCY", "¡EMERGENCIA ACTIVADA!");
    LOG_ERROR("EMERGENCY", "Voltaje: " + String(currentVoltage, 3) + "V");

    emergencyActive = true;

    // Conmutar control de alimentación
    setPowerControl(false);  // Cambiar a modo emergencia (LOW)

    // Pausar Pixhawk y liberar UART1
    if (pixhawkInterface != nullptr) {
        LOG_INFO("EMERGENCY", "Pausando comunicación con Pixhawk");
        pixhawkInterface->pauseForEmergency();
        delay(100); // Pequeña pausa para asegurar liberación del UART
    }

    /* Inicializar GPS si no está inicializado */
    if (!gpsInitialized) {
        Serial1.begin(9600, SERIAL_8N1, EMERGENCY_GPS_RX_PIN, EMERGENCY_GPS_TX_PIN);
        gpsInitialized = true;
        LOG_DEBUG("EMERGENCY", "GPS de respaldo inicializado");
    }
    
    /* Inicializar NRF */ 
    if (!initializeNRF()) {
        LOG_ERROR("EMERGENCY", "Error al inicializar NRF");
    }
}

void EmergencySystem::deactivateEmergency() {
    LOG_INFO("EMERGENCY", "Emergencia desactivada");
    LOG_INFO("EMERGENCY", "Voltaje actual: " + String(currentVoltage, 3) + "V");

    emergencyActive = false;

    // Restaurar control de alimentación normal
    setPowerControl(true);   // Volver a modo normal (HIGH)

    // Opcional: apagar GPS para ahorrar energía
    if (gpsInitialized) {
        LOG_DEBUG("EMERGENCY", "Apagando gps");
        Serial1.end();
        gpsInitialized = false;
    }
    
    // Apagar NRF
    if (nrfInitialized && radio) {
        LOG_DEBUG("EMERGENCY", "Apagando NRF");
        radio->powerDown();
        delete radio;
        delete hspi;
        radio = nullptr;
        hspi = nullptr;
        nrfInitialized = false;
    }
}

void EmergencySystem::setPowerControl(bool enable) {
    powerControlActive = enable;
    digitalWrite(EMERGENCY_POWER_CONTROL_PIN, enable ? HIGH : LOW);
    
    LOG_INFO("EMERGENCY", "Control de alimentación: " + 
             String(enable ? "NORMAL (HIGH)" : "EMERGENCIA (LOW)"));
}

void EmergencySystem::readGPSData() {
    // Verificar si hay datos disponibles
    int available = Serial1.available();
    LOG_DEBUG("EMERGENCY", "Bytes disponibles en GPS: " + String(available));
    
    if (available > 0) {
        LOG_DEBUG("EMERGENCY", "¡Datos detectados! Procesando " + String(available) + " bytes...");
        
        // Contadores para estadísticas
        int bytesProcessed = 0;
        int sentencesProcessed = 0;
        
        // Procesar TODOS los datos disponibles
        while (Serial1.available() > 0) {
            char c = Serial1.read();
            bytesProcessed++;
            
            // Procesar cada caracter con TinyGPS++
            if (gps.encode(c)) {
                sentencesProcessed++;
                LOG_DEBUG("EMERGENCY", "Sentencia NMEA #" + String(sentencesProcessed) + " procesada");
                
                // Verificar si hay nuevos datos de ubicación
                if (gps.location.isUpdated()) {
                    currentLocation.latitude = gps.location.lat();
                    currentLocation.longitude = gps.location.lng();
                    currentLocation.valid = true;
                    
                    LOG_INFO("EMERGENCY", "GPS UBICACIÓN: Lat=" + String(currentLocation.latitude, 6) + 
                             ", Lon=" + String(currentLocation.longitude, 6));
                }
                
                if (gps.altitude.isUpdated()) {
                    currentLocation.altitude = gps.altitude.meters();
                    LOG_INFO("EMERGENCY", "GPS ALTITUD: " + String(currentLocation.altitude) + " metros");
                }
                
                if (gps.satellites.isUpdated()) {
                    currentLocation.satellites = gps.satellites.value();
                    LOG_INFO("EMERGENCY", "GPS SATÉLITES: " + String(currentLocation.satellites));
                }
                
                if (gps.time.isUpdated()) {
                    LOG_INFO("EMERGENCY", "GPS TIEMPO: " + String(gps.time.hour()) + ":" + 
                             String(gps.time.minute()) + ":" + String(gps.time.second()));
                }
            }
        }
        
        LOG_DEBUG("EMERGENCY", "Procesados " + String(bytesProcessed) + " bytes, " + 
                  String(sentencesProcessed) + " sentencias válidas");
        
        // Verificar estado final del GPS
        if (gps.location.isValid()) {
            currentLocation.valid = true;
            LOG_INFO("EMERGENCY", "GPS: SEÑAL VÁLIDA - Lat=" + String(gps.location.lat(), 6) + 
                     ", Lon=" + String(gps.location.lng(), 6));
        } else {
            currentLocation.valid = false;
            // Mostrar información de diagnóstico
            if (gps.charsProcessed() < 10) {
                LOG_WARN("EMERGENCY", "GPS: Pocos caracteres procesados (" + String(gps.charsProcessed()) + ")");
            } else if (gps.sentencesWithFix() == 0) {
                LOG_WARN("EMERGENCY", "GPS: Sin sentencias con fix válido");
            } else {
                LOG_WARN("EMERGENCY", "GPS: Esperando fix válido... (caracteres: " + 
                         String(gps.charsProcessed()) + ", sentencias: " + String(gps.sentencesWithFix()) + ")");
            }
        }
        
    } else {
        LOG_DEBUG("EMERGENCY", "GPS: Sin datos disponibles en el puerto serie");
    }
}

void EmergencySystem::sendNRFPacket() {
    if (!nrfInitialized || !radio) {
        LOG_ERROR("EMERGENCY", "NRF no inicializado");
        return;
    }

    // Verificar conexión del chip
    if (!radio->isChipConnected()) {
        LOG_ERROR("EMERGENCY", "NRF24L01 no está conectado o no responde");
        // Intentar reinicializar
        LOG_INFO("EMERGENCY", "Intentando reinicializar NRF...");
        nrfInitialized = false;
        delete radio;
        delete hspi;
        radio = nullptr;
        hspi = nullptr;
        
        if (!initializeNRF()) {
            LOG_ERROR("EMERGENCY", "Falló la reinicialización del NRF");
            return;
        }
    }

    LOG_DEBUG("EMERGENCY", "Enviando dato");
    // Preparar paquete de emergencia
    EmergencyPacket packet;
    packet.header = 0xEE;                           // Identificador de emergencia
    packet.latitude = currentLocation.latitude;
    packet.longitude = currentLocation.longitude;
    packet.altitude = currentLocation.altitude;
    packet.satellites = currentLocation.satellites;
    packet.timestamp = millis();
    packet.voltage = currentVoltage;  // Incluir voltaje que causó la emergencia

    // Calcular checksum simple (XOR de todos los bytes)
    uint8_t* data = (uint8_t*)&packet;
    packet.checksum = 0;
    for (size_t i = 0; i < sizeof(EmergencyPacket) - 1; i++) {
        packet.checksum ^= data[i];
    }
    
    LOG_DEBUG("EMERGENCY", "Tamaño del paquete: " + String(sizeof(EmergencyPacket)) + " bytes");

    // Enviar paquete
    bool result = radio->write(&packet, sizeof(EmergencyPacket));
    
    if (result) {
        LOG_INFO("EMERGENCY", "Paquete enviado exitosamente");
        LOG_DEBUG("EMERGENCY", "GPS: " + String(packet.latitude, 6) + "," + 
                  String(packet.longitude, 6) + " Alt:" + String(packet.altitude) + 
                  "m Sat:" + String(packet.satellites));
    } else {
        LOG_ERROR("EMERGENCY", "Error al enviar paquete");
        LOG_ERROR("EMERGENCY", "Detalles del error:");
        LOG_ERROR("EMERGENCY", "- Chip conectado: " + String(radio->isChipConnected() ? "Sí" : "No"));
        LOG_ERROR("EMERGENCY", "- Canal actual: " + String(radio->getChannel()));
        // Verificar si el radio está funcionando
        if (!radio->isChipConnected()) {
            LOG_ERROR("EMERGENCY", "NRF24L01 no responde - reiniciando");
            nrfInitialized = false;
            delete radio;
            delete hspi;
            radio = nullptr;
            hspi = nullptr;
        }
    }
}

bool EmergencySystem::initializeNRF() {
    if (nrfInitialized) {
        return true;
    }
    LOG_DEBUG("EMERGENCY", "Inicializando NRF");
    
    // Crear instancia SPI personalizada para ESP32
    hspi = new SPIClass(HSPI);
    hspi->begin(EMERGENCY_NRF_SCK_PIN, EMERGENCY_NRF_MISO_PIN, EMERGENCY_NRF_MOSI_PIN, EMERGENCY_NRF_CS_PIN);
    
    // Crear objeto RF24
    radio = new RF24(EMERGENCY_NRF_CE_PIN, EMERGENCY_NRF_CS_PIN, 16000000);  // 16MHz SPI
    
    // Inicializar radio
    if (!radio->begin(hspi)) {
        LOG_ERROR("EMERGENCY", "Error al inicializar NRF24L01");
        delete radio;
        delete hspi;
        radio = nullptr;
        hspi = nullptr;
        return false;
    }
    
    // Configurar el radio
    // Niveles de potencia: RF24_PA_MIN=0, RF24_PA_LOW=1, RF24_PA_HIGH=2, RF24_PA_MAX=3
    radio->setPALevel(RF24_PA_MAX);           // Usar valor de config.h
    radio->setChannel(NRF_CHANNEL);            // Canal 76
    // Velocidades: 0=1MBPS, 1=2MBPS, 2=250KBPS
    radio->setDataRate(RF24_1MBPS);          // 250KBPS para mayor alcance

    radio->setRetries(0, 0);                 // 15 reintentos, 15*250us entre reintentos
    radio->setPayloadSize(sizeof(EmergencyPacket)); // Tamaño del paquete
    radio->setAutoAck(false);                   // Habilitar auto-acknowledgment

    // CRC: 0=disabled, 1=8bits, 2=16bits
    radio->setCRCLength(RF24_CRC_16);          // CRC de 16 bits
    
    // Configurar como transmisor
    radio->openWritingPipe(txAddress);
    radio->stopListening();
    
    nrfInitialized = true;
    LOG_INFO("EMERGENCY", "NRF24L01 inicializado correctamente");
    LOG_DEBUG("EMERGENCY", "Canal: " + String(NRF_CHANNEL) + ", DataRate: 250KBPS");
    
    return true;
}