#include "modules/pixhawk_interface.h"
#include "logger.h"

// Constructor
PixhawkInterface::PixhawkInterface() : pixhawkSerial(1) {  // Usar UART1
    latitude = 0.0;
    longitude = 0.0;
    altitude = 0.0;
    heading = 0.0;
    lastUpdateTime = 0;
}

void PixhawkInterface::begin() {
    // Configurar UART para comunicación con Pixhawk
    // Usando GPIO1 como RX (solo recepción)
    pixhawkSerial.begin(57600, SERIAL_8N1, PIXHAWK_RX_PIN, PIXHAWK_TX_PIN);
    pixhawkSerial.setTimeout(100);  // Timeout de 100ms
    
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
    static uint8_t msgBuffer[256];
    static uint8_t bufferIndex = 0;
    static bool msgStartFound = false;
    static uint8_t expectedLength = 0;
    static uint8_t msgLength = 0;
    
    while (pixhawkSerial.available() > 0) {
        uint8_t receivedByte = pixhawkSerial.read();
        
        // Buscar inicio de mensaje MAVLink (0xFE para MAVLink 1.0, 0xFD para MAVLink 2.0)
        if (!msgStartFound) {
            if (receivedByte == 0xFE || receivedByte == 0xFD) {
                msgStartFound = true;
                bufferIndex = 0;
                msgBuffer[bufferIndex++] = receivedByte;
                
                if (receivedByte == 0xFE) {
                    expectedLength = 8;  // MAVLink 1.0: header mínimo para obtener length
                } else {
                    expectedLength = 10; // MAVLink 2.0: header mínimo para obtener length
                }
            }
            continue;
        }
        
        // Almacenar byte en buffer
        if (bufferIndex < sizeof(msgBuffer)) {
            msgBuffer[bufferIndex++] = receivedByte;
        }
        
        // Obtener longitud del payload después de leer el header mínimo
        if (bufferIndex == expectedLength && msgLength == 0) {
            if (msgBuffer[0] == 0xFE) {
                // MAVLink 1.0: length está en posición 1
                msgLength = msgBuffer[1];
                expectedLength = 8 + msgLength;  // Header(6) + Payload + Checksum(2)
            } else {
                // MAVLink 2.0: length está en posición 1
                msgLength = msgBuffer[1];
                expectedLength = 10 + msgLength;  // Header(8) + Payload + Checksum(2)
            }
            
            // Verificar longitud razonable
            if (msgLength > 200) {
                LOG_WARN("PIXHAWK", "Longitud de mensaje MAVLink inválida: " + String(msgLength));
                msgStartFound = false;
                msgLength = 0;
                continue;
            }
        }
        
        // Procesar mensaje completo
        if (bufferIndex >= expectedLength && expectedLength > 8) {
            processMAVLinkMessage(msgBuffer, bufferIndex);
            
            // Reiniciar para siguiente mensaje
            msgStartFound = false;
            msgLength = 0;
            bufferIndex = 0;
        }
    }
}

void PixhawkInterface::processMAVLinkMessage(uint8_t* buffer, uint8_t length) {
    if (length < 8) return;
    
    uint8_t msgId;
    bool isMAVLink2 = (buffer[0] == 0xFD);
    
    if (isMAVLink2) {
        if (length < 10) return;
        msgId = buffer[5];  // Message ID en MAVLink 2.0
    } else {
        msgId = buffer[5];  // Message ID en MAVLink 1.0
    }
    
    switch (msgId) {
        case 0:  // HEARTBEAT
            LOG_DEBUG("PIXHAWK", "Heartbeat recibido");
            break;
            
        case 24:  // GPS_RAW_INT
            parseGPSRawInt(buffer, length, isMAVLink2);
            break;
            
        case 30:  // ATTITUDE
            parseAttitude(buffer, length, isMAVLink2);
            break;
            
        case 33:  // GLOBAL_POSITION_INT
            parseGlobalPosition(buffer, length, isMAVLink2);
            break;
            
        default:
            LOG_VERBOSE("PIXHAWK", "Mensaje MAVLink ID " + String(msgId) + " recibido");
            break;
    }
}

void PixhawkInterface::parseGPSRawInt(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 8 : 6;
    
    if (length < payloadStart + 30) return;  // GPS_RAW_INT requiere al menos 30 bytes de payload
    
    // Extraer datos (little-endian)
    int32_t lat = (int32_t)(buffer[payloadStart + 8] |
                           (buffer[payloadStart + 9] << 8) |
                           (buffer[payloadStart + 10] << 16) |
                           (buffer[payloadStart + 11] << 24));
    
    int32_t lon = (int32_t)(buffer[payloadStart + 12] |
                           (buffer[payloadStart + 13] << 8) |
                           (buffer[payloadStart + 14] << 16) |
                           (buffer[payloadStart + 15] << 24));
    
    int32_t alt = (int32_t)(buffer[payloadStart + 16] | 
                           (buffer[payloadStart + 17] << 8) | 
                           (buffer[payloadStart + 18] << 16) | 
                           (buffer[payloadStart + 19] << 24));
    
    // Convertir a decimales
    latitude = lat / 10000000.0;  // De 1E7 a grados decimales
    longitude = lon / 10000000.0;
    altitude = alt / 1000.0;      // De mm a metros
    
    LOG_INFO("PIXHAWK", "GPS: Lat=" + String(latitude, 6) + 
             ", Lon=" + String(longitude, 6) + 
             ", Alt=" + String(altitude, 1) + "m");
}

void PixhawkInterface::parseAttitude(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 8 : 6;
    
    if (length < payloadStart + 28) return;  // ATTITUDE requiere 28 bytes de payload
    
    // Extraer yaw (heading) - está en bytes 16-19 del payload
    union {
        uint8_t bytes[4];
        float value;
    } yawUnion;
    
    yawUnion.bytes[0] = buffer[payloadStart + 16];
    yawUnion.bytes[1] = buffer[payloadStart + 17];
    yawUnion.bytes[2] = buffer[payloadStart + 18];
    yawUnion.bytes[3] = buffer[payloadStart + 19];
    
    // Convertir de radianes a grados
    heading = yawUnion.value * 180.0 / M_PI;
    
    // Normalizar a 0-360 grados
    if (heading < 0) heading += 360;
    
    LOG_DEBUG("PIXHAWK", "Attitude: Heading=" + String(heading, 1) + "°");
}

void PixhawkInterface::parseGlobalPosition(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadStart = isMAVLink2 ? 8 : 6;
    
    if (length < payloadStart + 28) return;
    
    // Extraer datos de posición global filtrada
    int32_t lat = (int32_t)(buffer[payloadStart + 4] | 
                           (buffer[payloadStart + 5] << 8) | 
                           (buffer[payloadStart + 6] << 16) | 
                           (buffer[payloadStart + 7] << 24));
    
    int32_t lon = (int32_t)(buffer[payloadStart + 8] | 
                           (buffer[payloadStart + 9] << 8) | 
                           (buffer[payloadStart + 10] << 16) | 
                           (buffer[payloadStart + 11] << 24));
    
    int32_t alt = (int32_t)(buffer[payloadStart + 12] | 
                           (buffer[payloadStart + 13] << 8) | 
                           (buffer[payloadStart + 14] << 16) | 
                           (buffer[payloadStart + 15] << 24));
    
    // Actualizar con datos filtrados (más precisos que GPS raw)
    latitude = lat / 10000000.0;
    longitude = lon / 10000000.0;
    altitude = alt / 1000.0;
    
    LOG_DEBUG("PIXHAWK", "Global Position: Lat=" + String(latitude, 6) + 
              ", Lon=" + String(longitude, 6) + 
              ", Alt=" + String(altitude, 1) + "m");
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

String PixhawkInterface::getCSVHeader() {
    return "Latitud,Longitud,Altitud(m),Heading(°)";
}

String PixhawkInterface::getCSVData() {
    String data = "";
    data += String(latitude, 6) + ",";
    data += String(longitude, 6) + ",";
    data += String(altitude, 2) + ",";
    data += String(heading, 1);
    
    return data;
}