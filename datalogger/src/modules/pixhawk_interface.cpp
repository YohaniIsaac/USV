#include "modules/pixhawk_interface.h"
#include "logger.h"

// Constructor
PixhawkInterface::PixhawkInterface() : pixhawkSerial(1) {  // Usar UART1
    latitude = 0.0;
    longitude = 0.0;
    altitude = 0.0;
    heading = 0.0;
    lastUpdateTime = 0;
    
    // Variables de batería
    batteryVoltage = 0.0;
    batteryCurrent = 0.0;
    batteryRemaining = 0;
    batteryTemperature = 0;
    
    // Variables adicionales
    groundSpeed = 0.0;
    airSpeed = 0.0;
    numSatellites = 0;
    gpsFixType = 0;
}

void PixhawkInterface::begin() {
    // Configurar UART para comunicación con Pixhawk
    pixhawkSerial.begin(57600, SERIAL_8N1, PIXHAWK_RX_PIN, PIXHAWK_TX_PIN);
    pixhawkSerial.setTimeout(100);
    
    LOG_INFO("PIXHAWK", "Interfaz Pixhawk inicializada");
    LOG_INFO("PIXHAWK", "Puerto: UART1, Baudios: 57600");
    LOG_INFO("PIXHAWK", "Pin RX: " + String(PIXHAWK_RX_PIN));
    LOG_INFO("PIXHAWK", "Esperando datos MAVLink...");
}

void PixhawkInterface::update() {
    unsigned long currentTime = millis();
    
    // Leer y procesar datos MAVLink disponibles
    if (pixhawkSerial.available() > 0) {
        parseMAVLink();
        lastUpdateTime = currentTime;
    }
    
    // Verificar timeout (sin datos por más de 5 segundos)
    if (currentTime - lastUpdateTime > 5000 && lastUpdateTime > 0) {
        LOG_WARN("PIXHAWK", "Timeout - Sin datos MAVLink por más de 5 segundos");
    }
}

void PixhawkInterface::parseMAVLink() {
    static uint8_t msgBuffer[280];  // Buffer más grande para MAVLink 2.0
    static uint8_t bufferIndex = 0;
    static bool msgStartFound = false;
    static uint8_t expectedLength = 0;
    static uint8_t payloadLength = 0;
    static bool isMAVLink2 = false;
    
    while (pixhawkSerial.available() > 0) {
        uint8_t receivedByte = pixhawkSerial.read();
        
        // Buscar inicio de mensaje MAVLink
        if (!msgStartFound) {
            if (receivedByte == 0xFE || receivedByte == 0xFD) {
                msgStartFound = true;
                bufferIndex = 0;
                msgBuffer[bufferIndex++] = receivedByte;
                isMAVLink2 = (receivedByte == 0xFD);
                expectedLength = isMAVLink2 ? 2 : 2;  // Necesitamos al menos 2 bytes más para obtener length
                payloadLength = 0;
                
                LOG_VERBOSE("PIXHAWK", "Inicio de mensaje " + String(isMAVLink2 ? "MAVLink2" : "MAVLink1"));
            }
            continue;
        }
        
        // Almacenar byte en buffer
        if (bufferIndex < sizeof(msgBuffer)) {
            msgBuffer[bufferIndex++] = receivedByte;
        } else {
            // Buffer overflow - reiniciar
            LOG_WARN("PIXHAWK", "Buffer overflow - reiniciando parser");
            msgStartFound = false;
            continue;
        }
        
        // Obtener longitud del payload después de leer suficientes bytes del header
        if (payloadLength == 0) {
            if (isMAVLink2 && bufferIndex >= 2) {
                // MAVLink 2.0: payload length está en posición 1
                payloadLength = msgBuffer[1];
                expectedLength = 12 + payloadLength;  // Header(10) + Payload + Checksum(2)
            } else if (!isMAVLink2 && bufferIndex >= 2) {
                // MAVLink 1.0: payload length está en posición 1  
                payloadLength = msgBuffer[1];
                expectedLength = 8 + payloadLength;   // Header(6) + Payload + Checksum(2)
            }
            
            // Verificar longitud razonable
            if (payloadLength > 255) {
                LOG_WARN("PIXHAWK", "Longitud de payload inválida: " + String(payloadLength));
                msgStartFound = false;
                payloadLength = 0;
                continue;
            }
            
            if (expectedLength > 0) {
                LOG_VERBOSE("PIXHAWK", "Esperando mensaje de " + String(expectedLength) + " bytes, payload: " + String(payloadLength));
            }
        }
        
        // Procesar mensaje completo
        if (expectedLength > 0 && bufferIndex >= expectedLength) {
            LOG_VERBOSE("PIXHAWK", "Mensaje completo recibido: " + String(bufferIndex) + " bytes");
            processMAVLinkMessage(msgBuffer, bufferIndex);
            
            // Reiniciar para siguiente mensaje
            msgStartFound = false;
            payloadLength = 0;
            expectedLength = 0;
            bufferIndex = 0;
        }
    }
}

void PixhawkInterface::processMAVLinkMessage(uint8_t* buffer, uint8_t length) {
    uint32_t msgId;  // Message ID puede ser de 24 bits en MAVLink 2.0
    bool isMAVLink2 = (buffer[0] == 0xFD);
    
    // Extraer message ID según la versión
    if (isMAVLink2) {
        if (length < 12) return;  // Mensaje muy corto para MAVLink 2.0
        // MAVLink 2.0: Message ID está en posiciones 7, 8, 9 (24 bits, little endian)
        msgId = buffer[7] | (buffer[8] << 8) | (buffer[9] << 16);
    } else {
        if (length < 8) return;   // Mensaje muy corto para MAVLink 1.0  
        // MAVLink 1.0: Message ID está en posición 5 (8 bits)
        msgId = buffer[5];
    }
    
    // 🎯 LOG TODOS LOS MESSAGE IDs PARA DEBUGGING
    LOG_INFO("PIXHAWK", "📡 RX: ID=" + String(msgId) + " len=" + String(length) + 
             " (" + String(isMAVLink2 ? "v2.0" : "v1.0") + ")");
    
    switch (msgId) {
        case 0:  // HEARTBEAT
            LOG_DEBUG("PIXHAWK", "💓 Heartbeat recibido");
            break;
            
        case 24:  // GPS_RAW_INT
            LOG_INFO("PIXHAWK", "🛰️ GPS_RAW_INT recibido!");
            parseGPSRawInt(buffer, length, isMAVLink2);
            break;
            
        case 30:  // ATTITUDE
            LOG_INFO("PIXHAWK", "🧭 ATTITUDE recibido!");
            parseAttitude(buffer, length, isMAVLink2);
            break;
            
        case 33:  // GLOBAL_POSITION_INT
            LOG_INFO("PIXHAWK", "🌍 GLOBAL_POSITION_INT recibido!");
            parseGlobalPosition(buffer, length, isMAVLink2);
            break;
            
        case 74:  // VFR_HUD
            LOG_DEBUG("PIXHAWK", "📊 VFR_HUD recibido");
            parseVFRHUD(buffer, length, isMAVLink2);
            break;
            
        case 147: // BATTERY_STATUS
            LOG_INFO("PIXHAWK", "🔋 BATTERY_STATUS recibido!");
            parseBatteryStatus(buffer, length, isMAVLink2);
            break;
            
        case 163: // GPS_STATUS  
            LOG_DEBUG("PIXHAWK", "🛰️ GPS_STATUS recibido");
            parseGPSStatus(buffer, length, isMAVLink2);
            break;
            
        default:
            LOG_VERBOSE("PIXHAWK", "📦 Mensaje ID " + String(msgId) + " recibido");
            break;
    }
}

void PixhawkInterface::parseGPSRawInt(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;  // Inicio del payload
    uint8_t minLength = payloadStart + 30;       // GPS_RAW_INT requiere 30 bytes de payload
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "GPS_RAW_INT: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // Extraer timestamp (8 bytes, posición 0-7 del payload)
    uint64_t timestamp = 0;
    for (int i = 0; i < 8; i++) {
        timestamp |= ((uint64_t)buffer[payloadStart + i]) << (i * 8);
    }
    
    // Extraer latitud (4 bytes, posición 8-11 del payload) - little endian
    int32_t lat = (int32_t)(buffer[payloadStart + 8] | 
                           (buffer[payloadStart + 9] << 8) | 
                           (buffer[payloadStart + 10] << 16) | 
                           (buffer[payloadStart + 11] << 24));
    
    // Extraer longitud (4 bytes, posición 12-15 del payload)
    int32_t lon = (int32_t)(buffer[payloadStart + 12] | 
                           (buffer[payloadStart + 13] << 8) | 
                           (buffer[payloadStart + 14] << 16) | 
                           (buffer[payloadStart + 15] << 24));
    
    // Extraer altitud (4 bytes, posición 16-19 del payload)
    int32_t alt = (int32_t)(buffer[payloadStart + 16] | 
                           (buffer[payloadStart + 17] << 8) | 
                           (buffer[payloadStart + 18] << 16) | 
                           (buffer[payloadStart + 19] << 24));
    
    // Convertir a valores reales
    latitude = lat / 10000000.0;   // De 1E7 a grados decimales
    longitude = lon / 10000000.0;  // De 1E7 a grados decimales
    altitude = alt / 1000.0;       // De mm a metros
    
    LOG_INFO("PIXHAWK", "🛰️ GPS RAW: Lat=" + String(latitude, 6) + 
             "° Lon=" + String(longitude, 6) + "° Alt=" + String(altitude, 1) + "m");
}

void PixhawkInterface::parseAttitude(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;
    uint8_t minLength = payloadStart + 28;  // ATTITUDE requiere 28 bytes de payload
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "ATTITUDE: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // En ATTITUDE, los ángulos están como float (4 bytes cada uno)
    // Orden: timestamp(4), roll(4), pitch(4), yaw(4), rollspeed(4), pitchspeed(4), yawspeed(4)
    
    // Extraer yaw (heading) - bytes 16-19 del payload (después de timestamp, roll, pitch)
    union {
        uint8_t bytes[4];
        float value;
    } yawUnion;
    
    // Little endian
    yawUnion.bytes[0] = buffer[payloadStart + 16];
    yawUnion.bytes[1] = buffer[payloadStart + 17];
    yawUnion.bytes[2] = buffer[payloadStart + 18];
    yawUnion.bytes[3] = buffer[payloadStart + 19];
    
    // Convertir de radianes a grados
    heading = yawUnion.value * 180.0 / M_PI;
    
    // Normalizar a 0-360 grados
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    
    LOG_INFO("PIXHAWK", "🧭 ATTITUDE: Heading=" + String(heading, 1) + "°");
}

void PixhawkInterface::parseGlobalPosition(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;
    uint8_t minLength = payloadStart + 28;  // GLOBAL_POSITION_INT requiere 28 bytes
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "GLOBAL_POSITION_INT: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // GLOBAL_POSITION_INT estructura:
    // timestamp(4), lat(4), lon(4), alt(4), relative_alt(4), vx(2), vy(2), vz(2), hdg(2)
    
    // Extraer timestamp (4 bytes, posición 0-3)
    uint32_t timestamp = (uint32_t)(buffer[payloadStart + 0] | 
                                   (buffer[payloadStart + 1] << 8) | 
                                   (buffer[payloadStart + 2] << 16) | 
                                   (buffer[payloadStart + 3] << 24));
    
    // Extraer latitud (4 bytes, posición 4-7 del payload)
    int32_t lat = (int32_t)(buffer[payloadStart + 4] | 
                           (buffer[payloadStart + 5] << 8) | 
                           (buffer[payloadStart + 6] << 16) | 
                           (buffer[payloadStart + 7] << 24));
    
    // Extraer longitud (4 bytes, posición 8-11 del payload)
    int32_t lon = (int32_t)(buffer[payloadStart + 8] | 
                           (buffer[payloadStart + 9] << 8) | 
                           (buffer[payloadStart + 10] << 16) | 
                           (buffer[payloadStart + 11] << 24));
    
    // Extraer altitud (4 bytes, posición 12-15 del payload)
    int32_t alt = (int32_t)(buffer[payloadStart + 12] | 
                           (buffer[payloadStart + 13] << 8) | 
                           (buffer[payloadStart + 14] << 16) | 
                           (buffer[payloadStart + 15] << 24));
    
    // Actualizar con datos filtrados (más precisos que GPS raw)
    latitude = lat / 10000000.0;   // De 1E7 a grados decimales
    longitude = lon / 10000000.0;  // De 1E7 a grados decimales  
    altitude = alt / 1000.0;       // De mm a metros
    
    LOG_INFO("PIXHAWK", "🌍 GLOBAL POS: Lat=" + String(latitude, 6) + 
             "° Lon=" + String(longitude, 6) + "° Alt=" + String(altitude, 1) + "m");
}

void PixhawkInterface::parseBatteryStatus(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;
    uint8_t minLength = payloadStart + 41;  // BATTERY_STATUS requiere 41 bytes
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "BATTERY_STATUS: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // BATTERY_STATUS estructura (simplificada):
    // id(1), battery_function(1), type(1), temperature(2), voltages[10](20), current_battery(2), 
    // current_consumed(4), energy_consumed(4), battery_remaining(1), time_remaining(4), charge_state(1)
    
    // Extraer temperatura (2 bytes, posición 3-4, en centígrados*100)
    int16_t temp = (int16_t)(buffer[payloadStart + 3] | (buffer[payloadStart + 4] << 8));
    batteryTemperature = temp / 100.0;  // Convertir de centígrados*100 a grados
    
    // Extraer voltajes de celdas (primeras 3 celdas, posiciones 5-10)
    uint16_t voltage1 = buffer[payloadStart + 5] | (buffer[payloadStart + 6] << 8);
    uint16_t voltage2 = buffer[payloadStart + 7] | (buffer[payloadStart + 8] << 8);
    uint16_t voltage3 = buffer[payloadStart + 9] | (buffer[payloadStart + 10] << 8);
    
    // Calcular voltaje total (en mV, convertir a V)
    if (voltage1 != 65535 && voltage2 != 65535 && voltage3 != 65535) {  // 65535 = sin datos
        batteryVoltage = (voltage1 + voltage2 + voltage3) / 1000.0;  // mV a V
    } else if (voltage1 != 65535) {
        batteryVoltage = voltage1 / 1000.0;  // Solo primera celda
    }
    
    // Extraer corriente (2 bytes, posición 25-26, en centíAmps)
    int16_t current = (int16_t)(buffer[payloadStart + 25] | (buffer[payloadStart + 26] << 8));
    batteryCurrent = current / 100.0;  // De centíAmps a Amps
    
    // Extraer carga restante (1 byte, posición 35, en %)
    batteryRemaining = buffer[payloadStart + 35];
    
    LOG_INFO("PIXHAWK", "🔋 BATERÍA: " + String(batteryVoltage, 2) + "V, " + 
             String(batteryCurrent, 2) + "A, " + String(batteryRemaining) + "%, " + 
             String(batteryTemperature, 1) + "°C");
}

void PixhawkInterface::parseVFRHUD(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;
    uint8_t minLength = payloadStart + 20;  // VFR_HUD requiere 20 bytes
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "VFR_HUD: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // VFR_HUD estructura:
    // airspeed(4), groundspeed(4), heading(2), throttle(2), alt(4), climb(4)
    
    // Extraer airspeed (4 bytes, float, posición 0-3)
    union { uint8_t bytes[4]; float value; } airspeedUnion;
    airspeedUnion.bytes[0] = buffer[payloadStart + 0];
    airspeedUnion.bytes[1] = buffer[payloadStart + 1];
    airspeedUnion.bytes[2] = buffer[payloadStart + 2];
    airspeedUnion.bytes[3] = buffer[payloadStart + 3];
    airSpeed = airspeedUnion.value;
    
    // Extraer groundspeed (4 bytes, float, posición 4-7)
    union { uint8_t bytes[4]; float value; } groundspeedUnion;
    groundspeedUnion.bytes[0] = buffer[payloadStart + 4];
    groundspeedUnion.bytes[1] = buffer[payloadStart + 5];
    groundspeedUnion.bytes[2] = buffer[payloadStart + 6];
    groundspeedUnion.bytes[3] = buffer[payloadStart + 7];
    groundSpeed = groundspeedUnion.value;
    
    LOG_DEBUG("PIXHAWK", "📊 VFR: Velocidad aire=" + String(airSpeed, 1) + 
              "m/s, tierra=" + String(groundSpeed, 1) + "m/s");
}

void PixhawkInterface::parseGPSStatus(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 10 : 6;
    uint8_t minLength = payloadStart + 101;  // GPS_STATUS requiere 101 bytes
    
    if (length < minLength) {
        LOG_WARN("PIXHAWK", "GPS_STATUS: mensaje muy corto (" + String(length) + " < " + String(minLength) + ")");
        return;
    }
    
    // GPS_STATUS estructura:
    // satellites_visible(1), satellite_prn[20](20), satellite_used[20](20), satellite_elevation[20](20), 
    // satellite_azimuth[20](20), satellite_snr[20](20)
    
    // Extraer número de satélites visibles
    numSatellites = buffer[payloadStart + 0];
    
    LOG_DEBUG("PIXHAWK", "🛰️ GPS: " + String(numSatellites) + " satélites visibles");
}

// Getters
float PixhawkInterface::getLatitude() {
    return latitude;
}

float PixhawkInterface::getLongitude() {
    return longitude;
}

float PixhawkInterface::getAltitude() {
    return altitude;
}

float PixhawkInterface::getHeading() {
    return heading;
}

// 🔋 Getters para batería
float PixhawkInterface::getBatteryVoltage() {
    return batteryVoltage;
}

float PixhawkInterface::getBatteryCurrent() {
    return batteryCurrent;
}

int PixhawkInterface::getBatteryRemaining() {
    return batteryRemaining;
}

float PixhawkInterface::getBatteryTemperature() {
    return batteryTemperature;
}

// 📊 Getters adicionales
float PixhawkInterface::getGroundSpeed() {
    return groundSpeed;
}

float PixhawkInterface::getAirSpeed() {
    return airSpeed;
}

int PixhawkInterface::getNumSatellites() {
    return numSatellites;
}

String PixhawkInterface::getCSVHeader() {
    return "git ";
}

String PixhawkInterface::save_CSVData() {
    String data = "";
    data += String(latitude, 6) + ",";
    data += String(longitude, 6) + ",";
    data += String(altitude, 2) + ",";
    // data += String(heading, 1) + ",";
    // data += String(batteryVoltage, 2) + ",";
    // data += String(batteryCurrent, 2) + ",";
    // data += String(batteryRemaining) + ",";
    // data += String(batteryTemperature, 1) + ",";
    // data += String(groundSpeed, 2) + ",";
    // data += String(airSpeed, 2) + ",";
    // data += String(numSatellites);
    
    return data;
}

void PixhawkInterface::show_message() {
    // Mostrar datos cada 2 segundos
    LOG_INFO("MAIN", "=================== DATOS PIXHAWK ===================");
    
    // 📍 Datos de posición
    LOG_INFO("MAIN", "📍 POSICIÓN:");
    LOG_INFO("MAIN", "  Latitud: " + String(latitude, 6) + "°");
    LOG_INFO("MAIN", "  Longitud: " + String(longitude, 6) + "°");
    LOG_INFO("MAIN", "  Altitud: " + String(altitude, 2) + " m");
    LOG_INFO("MAIN", "  Heading: " + String(heading, 1) + "°");
    
    // 🔋 Datos de batería
    LOG_INFO("MAIN", "🔋 BATERÍA:");
    LOG_INFO("MAIN", "  Voltaje: " + String(batteryVoltage, 2) + " V");
    LOG_INFO("MAIN", "  Corriente: " + String(batteryCurrent, 2) + " A");
    LOG_INFO("MAIN", "  Restante: " + String(batteryRemaining) + " %");
    LOG_INFO("MAIN", "  Temperatura: " + String(batteryTemperature, 1) + " °C");
    
    // 📊 Datos adicionales
    LOG_INFO("MAIN", "📊 NAVEGACIÓN:");
    LOG_INFO("MAIN", "  Vel. tierra: " + String(groundSpeed, 1) + " m/s");
    LOG_INFO("MAIN", "  Vel. aire: " + String(airSpeed, 1) + " m/s");
    LOG_INFO("MAIN", "  Satélites: " + String(numSatellites));
    
    LOG_INFO("MAIN", "====================================================");

}