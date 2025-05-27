#include "modules/emergency_system.h"
#include "logger.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Constructor
EmergencySystem::EmergencySystem() : gpsSerial(1) {
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
    
    currentLocation.latitude = 0.0;
    currentLocation.longitude = 0.0;
    currentLocation.altitude = 0.0;
    currentLocation.satellites = 0;
    currentLocation.valid = false;

    radio = nullptr;
    hspi = nullptr;
}

// Destructor
EmergencySystem::~EmergencySystem() {
    // Limpiar recursos si están activos
    if (gpsInitialized) {
        gpsSerial.end();
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
    // Configurar pin de emergencia como entrada con pull-up
    pinMode(EMERGENCY_PIN, INPUT_PULLUP);
    
    LOG_DEBUG("EMERGENCY", "Sistema de emergencia inicializado");
}

void EmergencySystem::update() {
    unsigned long currentTime = millis();
    
    // Verificar estado
    if (currentTime - lastCheckTime >= EMERGENCY_CHECK_RATE) {
        LOG_INFO("EMERGENCY", "Verificando estado del pin");
        // LOG_DEBUG("EMERGENCY", "Leyendo datos desde el GPS");
        // activateEmergency();
        if (emergencyActive){
            readGPSData();
        }
        checkEmergencyPin();
        lastCheckTime = currentTime;
    }
    
    // // Si está en emergencia, leer GPS y enviar datos
    // if (emergencyActive) {
        // Leer GPS continuamente
        // readGPSData();
        
    //     // Enviar datos cada 2 segundos si tenemos ubicación válida
    //     if (currentTime - lastTransmitTime >= 2000) {
    //         if (currentLocation.valid) {
    //             sendNRFPacket();
    //         } else {
    //             LOG_WARN("EMERGENCY", "Esperando señal GPS válida...");
    //         }
    //         lastTransmitTime = currentTime;
    //     }
    // }
}

void EmergencySystem::checkEmergencyPin() {
    bool pinState = digitalRead(EMERGENCY_PIN);
    LOG_DEBUG("EMERGENCY", "Estado del pin " + String(pinState));
    
    // Si el pin está en LOW y no estábamos en emergencia
    if (pinState && !emergencyActive) {
        LOG_DEBUG("EMERGENCY", "Activando emergencia");
        activateEmergency();
    }
    // Si el pin está en HIGH y estábamos en emergencia
    else if (!pinState && emergencyActive) {
        LOG_DEBUG("EMERGENCY", "Desactivando emergenica");
        deactivateEmergency();
    }
}

void EmergencySystem::activateEmergency() {
    LOG_WARN("EMERGENCY", "¡EMERGENCIA ACTIVADA!");
    emergencyActive = true;
    
    /* Inicializar GPS si no está inicializado */
    // if (!gpsInitialized) {
    //     gpsSerial.begin(9600, SERIAL_8N1, EMERGENCY_GPS_RX_PIN, EMERGENCY_GPS_TX_PIN);
    //     gpsInitialized = true;
    //     LOG_DEBUG("EMERGENCY", "GPS de respaldo inicializado");
    // }
    
    /* Inicializar NRF */ 
    if (!initializeNRF()) {
        LOG_ERROR("EMERGENCY", "Error al inicializar NRF");
    }
}

void EmergencySystem::deactivateEmergency() {
    LOG_INFO("EMERGENCY", "Emergencia desactivada");
    emergencyActive = false;
    
    // Opcional: apagar GPS para ahorrar energía
    if (gpsInitialized) {
        LOG_DEBUG("EMERGENCY", "Apagando gps");
        gpsSerial.end();
        gpsInitialized = false;
    }
    
    // // Apagar NRF
    // if (nrfInitialized && radio) {
    //     LOG_DEBUG("EMERGENCY", "Apagando NRF");
    //     radio->powerDown();
    //     delete radio;
    //     delete hspi;
    //     radio = nullptr;
    //     hspi = nullptr;
    //     nrfInitialized = false;
    // }
}

void EmergencySystem::readGPSData() {
    // Verificar si hay datos disponibles
    int available = gpsSerial.available();
    LOG_DEBUG("EMERGENCY", "Bytes disponibles en GPS: " + String(available));
    
    if (available > 0) {
        LOG_DEBUG("EMERGENCY", "¡Datos detectados! Procesando " + String(available) + " bytes...");
        
        // Contadores para estadísticas
        int bytesProcessed = 0;
        int sentencesProcessed = 0;
        
        // Procesar TODOS los datos disponibles
        while (gpsSerial.available() > 0) {
            char c = gpsSerial.read();
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
    LOG_DEBUG("EMERGENCY", "Enviando dato");
    
    // Preparar paquete de emergencia
    EmergencyPacket packet;
    packet.header = 0xEE;  // Identificador de emergencia
    packet.latitude = currentLocation.latitude;
    packet.longitude = currentLocation.longitude;
    packet.altitude = currentLocation.altitude;
    packet.satellites = currentLocation.satellites;
    packet.timestamp = millis();
    
    // Calcular checksum simple (XOR de todos los bytes)
    uint8_t* data = (uint8_t*)&packet;
    packet.checksum = 0;
    for (size_t i = 0; i < sizeof(EmergencyPacket) - 1; i++) {
        packet.checksum ^= data[i];
    }
    
    // Enviar paquete
    bool result = radio->write(&packet, sizeof(EmergencyPacket));
    
    if (result) {
        LOG_INFO("EMERGENCY", "Paquete enviado exitosamente");
        LOG_DEBUG("EMERGENCY", "GPS: " + String(packet.latitude, 6) + "," + 
                  String(packet.longitude, 6) + " Alt:" + String(packet.altitude) + 
                  "m Sat:" + String(packet.satellites));
    } else {
        LOG_ERROR("EMERGENCY", "Error al enviar paquete");
        
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
    radio->setDataRate(RF24_250KBPS);          // 250KBPS para mayor alcance

    radio->setRetries(15, 15);                 // 15 reintentos, 15*250us entre reintentos
    radio->setPayloadSize(sizeof(EmergencyPacket)); // Tamaño del paquete
    radio->setAutoAck(true);                   // Habilitar auto-acknowledgment

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


void EmergencySystem::testGPSBaudRates() {
    uint32_t baudRates[] = {4800, 9600, 19200, 38400, 57600, 115200};
    int numRates = sizeof(baudRates) / sizeof(baudRates[0]);
    
    for (int i = 0; i < numRates; i++) {
        LOG_INFO("EMERGENCY", "Probando velocidad: " + String(baudRates[i]));
        
        // Reiniciar puerto serie con nueva velocidad
        if (gpsInitialized) {
            gpsSerial.end();
        }
        
        gpsSerial.begin(baudRates[i], SERIAL_8N1, EMERGENCY_GPS_RX_PIN, EMERGENCY_GPS_TX_PIN);
        gpsInitialized = true;
        
        // Esperar un poco para que se establezca la conexión
        delay(500);
        
        // Leer datos por 10 segundos
        unsigned long startTime = millis();
        String testData = "";
        bool foundValidData = false;
        
        while (millis() - startTime < 10000) {
            while (gpsSerial.available() > 0) {
                char c = gpsSerial.read();
                testData += c;
                
                // Buscar inicio de sentencia NMEA válida
                if (testData.indexOf("$GP") != -1 || testData.indexOf("$GN") != -1) {
                    foundValidData = true;
                    break;
                }
                
                // Limitar tamaño del buffer de prueba
                if (testData.length() > 200) {
                    testData = testData.substring(100);
                }
            }
            
            if (foundValidData) break;
            delay(10);
        }
        
        if (foundValidData) {
            LOG_INFO("EMERGENCY", "¡Datos válidos encontrados a " + String(baudRates[i]) + " baudios!");
            LOG_DEBUG("EMERGENCY", "Muestra de datos: " + testData.substring(0, 100));
            // return; // Salir cuando encontremos la velocidad correcta
        } else {
            LOG_WARN("EMERGENCY", "Sin datos válidos a " + String(baudRates[i]) + " baudios");
            LOG_DEBUG("EMERGENCY", "Datos recibidos: " + testData.substring(0, 50));
        }
    }
    
    LOG_ERROR("EMERGENCY", "No se encontró velocidad válida para el GPS");
}