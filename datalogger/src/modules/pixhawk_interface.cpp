#include "modules/pixhawk_interface.h"
#include "logger.h"

// Constructor
PixhawkInterface::PixhawkInterface()  {
    // Posición y navegación básicas (mantener nombres originales)
    latitude = 0.0;
    longitude = 0.0;
    altitude = 0.0;
    heading = 0.0;
    lastUpdateTime = 0;
    
    // Variables adicionales del primer código
    roll = 0.0;
    pitch = 0.0;
    yaw = 0.0;
    altitudeRelative = 0.0;
    
    // Velocidades
    velocidadSuelo = 0.0;
    velocidadAire = 0.0;
    velocidadVertical = 0.0;
    
    // GPS
    tipoFixGPS = 0;
    satelites = 0;
    
    // Variables de batería (mantener nombres originales)
    batteryVoltage = 0.0;
    batteryCurrent = 0.0;
    batteryRemaining = 0;
    batteryTemperature = 0.0;
    
    // Variables adicionales (compatibilidad)
    groundSpeed = 0.0;
    airSpeed = 0.0;
    numSatellites = 0;
    gpsFixType = 0;

    // VARIABLES DE TIEMPO GPS
    gpsTimeUsec = 0;
    gpsYear = 0;
    gpsMonth = 0;
    gpsDay = 0;
    gpsHour = 0;
    gpsMinute = 0;
    gpsSecond = 0;
    gpsTimeValid = false;
    
    // Estado de conexión
    connected = false;
    armed = false;
    flightMode = 0;
    systemStatus = 0;

    paused = false;         
    wasInitialized = false;  
}

void PixhawkInterface::begin() {
    // Configurar UART para comunicación con Pixhawk
    Serial1.begin(PIXHAWK_BAUD_RATE, SERIAL_8N1, PIXHAWK_RX_PIN, PIXHAWK_TX_PIN);
    Serial1.setTimeout(100);
    
    LOG_INFO("PIXHAWK", "Interfaz Pixhawk inicializada");
    LOG_INFO("PIXHAWK", "Puerto: UART1, Baudios: " + String(PIXHAWK_BAUD_RATE));
    LOG_INFO("PIXHAWK", "Pin RX: " + String(PIXHAWK_RX_PIN));
    LOG_INFO("PIXHAWK", "Esperando datos MAVLink...");
}

void PixhawkInterface::update() {
    // No procesar si está pausado
    if (paused) {
        return;
    }

    unsigned long currentTime = millis();

    // Procesar mensajes MAVLink disponibles
    parseMAVLink();

}

void PixhawkInterface::pauseForEmergency() {
    if (!paused && wasInitialized) {
        LOG_INFO("PIXHAWK", "Pausando comunicación - liberando UART1");
        Serial1.end();
        paused = true;
    }
}

void PixhawkInterface::resumeAfterEmergency() {
    if (paused && wasInitialized) {
        LOG_INFO("PIXHAWK", "Reanudando comunicación - reactivando UART1");
        Serial1.begin(PIXHAWK_BAUD_RATE, SERIAL_8N1, PIXHAWK_RX_PIN, PIXHAWK_TX_PIN);
        Serial1.setTimeout(100);
        paused = false;
    }
}

void PixhawkInterface::parseMAVLink() {
    static uint8_t buffer[300];
    static int bufferIndex = 0;
    static bool inMessage = false;
    static bool isMAVLink2 = false;
    static uint8_t expectedLength = 0;
    
    while (Serial1.available()) {
        uint8_t receivedByte = Serial1.read();
        
        // Detectar inicio de mensaje MAVLink
        if (!inMessage && (receivedByte == 0xFD || receivedByte == 0xFE)) {
            inMessage = true;
            isMAVLink2 = (receivedByte == 0xFD);
            bufferIndex = 0;
            buffer[bufferIndex++] = receivedByte;
            continue;
        }
        
        if (inMessage) {
            buffer[bufferIndex++] = receivedByte;
            
            // Obtener longitud del payload después del segundo byte
            if (bufferIndex == 2) {
                uint8_t payloadLength = receivedByte;
                expectedLength = (isMAVLink2 ? 12 : 6) + payloadLength + 2;
            }
            
            // Procesar mensaje completo
            if (bufferIndex >= expectedLength && expectedLength > 0) {
                processMAVLinkMessage(buffer, bufferIndex, isMAVLink2);
                inMessage = false;
                bufferIndex = 0;
                expectedLength = 0;
            }
            
            // Prevenir buffer overflow
            if (bufferIndex >= 300) {
                LOG_WARN("PIXHAWK", "Buffer overflow - reiniciando parser");
                inMessage = false;
                bufferIndex = 0;
            }
        }
    }
}

void PixhawkInterface::processMAVLinkMessage(uint8_t* buffer, uint8_t length, bool isMAVLink2) {
    uint8_t payloadOffset = isMAVLink2 ? 10 : 6;
    uint32_t msgId;
    
    // Extraer message ID según la versión
    if (isMAVLink2) {
        if (length < 12) return;
        msgId = buffer[7] | (buffer[8] << 8) | (buffer[9] << 16);
    } else {
        if (length < 8) return;
        msgId = buffer[5];
    }
    
    uint8_t* payload = &buffer[payloadOffset];
    lastUpdateTime = millis();
    connected = true;
    
    // LOG solo para mensajes importantes
    switch (msgId) {
        case 0:  // HEARTBEAT
            LOG_VERBOSE("PIXHAWK", " Heartbeat recibido");
            parseHeartbeat(payload);
            break;
            
        case 1:  // SYS_STATUS  
            LOG_VERBOSE("PIXHAWK", "⚙️ SYS_STATUS recibido");
            parseSysStatus(payload);
            break;

        case 2:  // SYSTEM_TIME - DATOS DE TIEMPO
            LOG_DEBUG("PIXHAWK", "🕐 SYSTEM_TIME recibido");
            parseSystemTime(payload);
            break;
            
        case 24: // GPS_RAW_INT
            LOG_DEBUG("PIXHAWK", "🛰️ GPS_RAW_INT recibido");
            parseGPSRawInt(payload);
            break;
            
        case 30: // ATTITUDE
            LOG_DEBUG("PIXHAWK", "🧭 ATTITUDE recibido");
            parseAttitude(payload);
            break;
            
        case 33: // GLOBAL_POSITION_INT
            LOG_DEBUG("PIXHAWK", "🌍 GLOBAL_POSITION_INT recibido");
            parseGlobalPosition(payload);
            break;
            
        case 74: // VFR_HUD
            LOG_VERBOSE("PIXHAWK", "📊 VFR_HUD recibido");
            parseVFRHUD(payload);
            break;
            
        case 147: // BATTERY_STATUS
            LOG_DEBUG("PIXHAWK", "🔋 BATTERY_STATUS recibido");
            parseBatteryStatus(payload);
            break;
            
        case 163: // GPS_STATUS
            LOG_VERBOSE("PIXHAWK", "🛰️ GPS_STATUS recibido");
            parseGPSStatus(payload);
            break;
            
        default:
            LOG_VERBOSE("PIXHAWK", "📦 Mensaje ID " + String(msgId) + " recibido");
            break;
    }
}

void PixhawkInterface::parseHeartbeat(uint8_t* payload) {
    flightMode = payload[4];
    systemStatus = payload[5];
    armed = (payload[6] & 0x80) != 0;
    
    LOG_VERBOSE("PIXHAWK", "Heartbeat - Modo: " + String(flightMode) + 
                ", Armado: " + String(armed ? "Sí" : "No"));
}

void PixhawkInterface::parseSysStatus(uint8_t* payload) {
    // Extraer voltaje de batería (posición 12-13, en mV)
    uint16_t voltage = (payload[13] << 8) | payload[12];
    if (voltage != UINT16_MAX) {
        batteryVoltage = voltage / 1000.0;
    }
    
    // Extraer corriente (posición 14-15, en cA)
    int16_t current = (payload[15] << 8) | payload[14];
    if (current != -1) {
        batteryCurrent = current / 100.0;
    }
    
    // Porcentaje de batería (posición 16)
    batteryRemaining = payload[16];
    
    LOG_DEBUG("PIXHAWK", "SysStatus - Bat: " + String(batteryVoltage, 2) + "V, " + 
              String(batteryCurrent, 2) + "A, " + String(batteryRemaining) + "%");
}

// 🕐 NUEVO: Parsear mensaje SYSTEM_TIME
void PixhawkInterface::parseSystemTime(uint8_t* payload) {
    // SYSTEM_TIME contiene:
    // time_unix_usec (uint64_t): tiempo UTC en microsegundos
    // time_boot_ms (uint32_t): tiempo desde boot en ms
    
    uint64_t timeUnixUsec = *((uint64_t*)&payload[0]);
    
    if (timeUnixUsec > 0) {
        gpsTimeUsec = timeUnixUsec;
        convertUnixTimeToDateTime(timeUnixUsec);
        gpsTimeValid = true;
        
        LOG_INFO("PIXHAWK", "🕐 Tiempo del sistema actualizado: " + getGPSTimeString());
    }
}

void PixhawkInterface::parseGPSRawInt(uint8_t* payload) {
    //   EXTRAER TIEMPO GPS (primeros 8 bytes del mensaje)
    uint64_t timeUsec = *((uint64_t*)&payload[0]);
    
    if (timeUsec > 0) {
        gpsTimeUsec = timeUsec;
        convertUnixTimeToDateTime(timeUsec);
        gpsTimeValid = true;
        
        LOG_DEBUG("PIXHAWK", "🕐 Tiempo GPS actualizado: " + getGPSTimeString());
    }

    // Tipo de fix GPS y satélites (posiciones 36, 37)
    gpsFixType = payload[36];
    tipoFixGPS = gpsFixType;  // Compatibilidad
    satelites = payload[37];
    numSatellites = satelites;  // Compatibilidad
    
    // Coordenadas (int32 en grados * 1E7)
    int32_t lat = *((int32_t*)&payload[8]);
    int32_t lon = *((int32_t*)&payload[12]);
    int32_t alt = *((int32_t*)&payload[16]);
    
    if (lat != 0 && lon != 0) {
        latitude = lat / 1e7;
        longitude = lon / 1e7;
        altitude = alt / 1000.0;  // mm a metros
    }
    
    LOG_INFO("PIXHAWK", "🛰️ GPS: Lat=" + String(latitude, 6) + 
             "° Lon=" + String(longitude, 6) + "° Alt=" + String(altitude, 1) + 
             "m Sat=" + String(satelites));
}

void PixhawkInterface::parseAttitude(uint8_t* payload) {
    // Ángulos en radianes (float)
    float rollRad = *((float*)&payload[4]);
    float pitchRad = *((float*)&payload[8]);
    float yawRad = *((float*)&payload[12]);
    
    // Convertir a grados
    roll = rollRad * 180.0 / M_PI;
    pitch = pitchRad * 180.0 / M_PI;
    yaw = yawRad * 180.0 / M_PI;
    
    // Normalizar yaw a 0-360°
    heading = yaw;
    if (heading < 0) heading += 360;
    
    LOG_DEBUG("PIXHAWK", "🧭 Attitude: Roll=" + String(roll, 1) + 
              "° Pitch=" + String(pitch, 1) + "° Yaw=" + String(heading, 1) + "°");
}

void PixhawkInterface::parseGlobalPosition(uint8_t* payload) {
    // Altitud absoluta y relativa (int32 en mm)
    int32_t alt = *((int32_t*)&payload[16]);
    int32_t relativeAlt = *((int32_t*)&payload[20]);
    
    altitude = alt / 1000.0;
    altitudeRelative = relativeAlt / 1000.0;
    
    // Velocidad vertical (int16 en cm/s, coordenadas NED)
    int16_t vz = *((int16_t*)&payload[26]);
    velocidadVertical = -vz / 100.0;  // Negativo porque MAVLink usa NED
    
    LOG_DEBUG("PIXHAWK", "🌍 GlobalPos: Alt=" + String(altitude, 1) + 
              "m AltRel=" + String(altitudeRelative, 1) + "m Vz=" + String(velocidadVertical, 1) + "m/s");
}

void PixhawkInterface::parseVFRHUD(uint8_t* payload) {
    // Velocidades (float)
    airSpeed = *((float*)&payload[0]);
    groundSpeed = *((float*)&payload[4]);
    
    // Compatibilidad con nombres antiguos
    velocidadAire = airSpeed;
    velocidadSuelo = groundSpeed;
    
    LOG_DEBUG("PIXHAWK", "📊 VFR: VelAire=" + String(airSpeed, 1) + 
              "m/s VelSuelo=" + String(groundSpeed, 1) + "m/s");
}

void PixhawkInterface::parseBatteryStatus(uint8_t* payload) {
    // Temperatura (int16 en centígrados * 100)
    int16_t temp = *((int16_t*)&payload[3]);
    if (temp != INT16_MAX) {
        batteryTemperature = temp / 100.0;
    }
    
    // Voltaje de primera celda (uint16 en mV)
    uint16_t cellVoltage = *((uint16_t*)&payload[8]);
    if (cellVoltage != UINT16_MAX) {
        batteryVoltage = cellVoltage / 1000.0;
    }
    
    // Corriente (int16 en cA)
    int16_t current = *((int16_t*)&payload[22]);
    if (current != -1) {
        batteryCurrent = current / 100.0;
    }
    
    // Porcentaje restante
    batteryRemaining = payload[24];
    
    LOG_INFO("PIXHAWK", "🔋 Batería detallada: " + String(batteryVoltage, 2) + "V, " + 
             String(batteryCurrent, 2) + "A, " + String(batteryRemaining) + "%, " + 
             String(batteryTemperature, 1) + "°C");
}

void PixhawkInterface::parseGPSStatus(uint8_t* payload) {
    // Número de satélites visibles
    numSatellites = payload[0];
    satelites = numSatellites;  // Compatibilidad
    
    LOG_DEBUG("PIXHAWK", "🛰️ GPS Status: " + String(numSatellites) + " satélites visibles");
}

// Convertir timestamp UNIX a fecha/hora
void PixhawkInterface::convertUnixTimeToDateTime(uint64_t unixTimeUsec) {
    // Convertir microsegundos a segundos
    uint64_t unixTimeSec = unixTimeUsec / 1000000ULL;
    
    // Cálculos para convertir timestamp UNIX a fecha/hora
    // Segundos por día
    const uint32_t secondsPerDay = 86400;
    const uint32_t secondsPerHour = 3600;
    const uint32_t secondsPerMinute = 60;
    
    // Días desde epoch (1 enero 1970)
    uint32_t daysSinceEpoch = unixTimeSec / secondsPerDay;
    uint32_t secondsInDay = unixTimeSec % secondsPerDay;
    
    // Calcular hora, minuto, segundo
    gpsHour = secondsInDay / secondsPerHour;
    gpsMinute = (secondsInDay % secondsPerHour) / secondsPerMinute;
    gpsSecond = secondsInDay % secondsPerMinute;
    
    // Calcular año, mes, día
    // Empezar desde 1970
    uint16_t year = 1970;
    uint32_t daysRemaining = daysSinceEpoch;
    
    // Encontrar el año
    while (true) {
        uint32_t daysInYear = isLeapYear(year) ? 366 : 365;
        if (daysRemaining >= daysInYear) {
            daysRemaining -= daysInYear;
            year++;
        } else {
            break;
        }
    }
    gpsYear = year;
    
    // Días por mes (año no bisiesto)
    uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Ajustar febrero si es año bisiesto
    if (isLeapYear(year)) {
        daysInMonth[1] = 29;
    }
    
    // Encontrar mes y día
    uint8_t month = 1;
    for (month = 1; month <= 12; month++) {
        if (daysRemaining >= daysInMonth[month - 1]) {
            daysRemaining -= daysInMonth[month - 1];
        } else {
            break;
        }
    }
    gpsMonth = month;
    gpsDay = daysRemaining + 1;  // +1 porque los días empiezan en 1, no 0
}

//  Verificar año bisiesto
bool PixhawkInterface::isLeapYear(uint16_t year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// ====================== GETTERS (mantener interfaz original) ======================

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

float PixhawkInterface::getGroundSpeed() {
    return groundSpeed;
}

float PixhawkInterface::getAirSpeed() {
    return airSpeed;
}

int PixhawkInterface::getNumSatellites() {
    return numSatellites;
}

// 🕐 NUEVOS GETTERS PARA DATOS DE TIEMPO
uint64_t PixhawkInterface::getGPSTimeUsec() {
    return gpsTimeUsec;
}

uint16_t PixhawkInterface::getGPSYear() {
    return gpsYear;
}

uint8_t PixhawkInterface::getGPSMonth() {
    return gpsMonth;
}

uint8_t PixhawkInterface::getGPSDay() {
    return gpsDay;
}

uint8_t PixhawkInterface::getGPSHour() {
    return gpsHour;
}

uint8_t PixhawkInterface::getGPSMinute() {
    return gpsMinute;
}

uint8_t PixhawkInterface::getGPSSecond() {
    return gpsSecond;
}

bool PixhawkInterface::hasValidGPSTime() {
    return gpsTimeValid && gpsYear >= 2020;
}

String PixhawkInterface::getGPSTimeString() {
    if (!hasValidGPSTime()) {
        return "N/A";
    }
    
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
            gpsYear, gpsMonth, gpsDay, gpsHour, gpsMinute, gpsSecond);
    return String(buffer);
}

String PixhawkInterface::getGPSDateString() {
    if (!hasValidGPSTime()) {
        return "N/A";
    }
    
    char buffer[12];
    sprintf(buffer, "%04d-%02d-%02d", gpsYear, gpsMonth, gpsDay);
    return String(buffer);
}

String PixhawkInterface::getGPSTimeOnlyString() {
    if (!hasValidGPSTime()) {
        return "N/A";
    }
    
    char buffer[10];
    sprintf(buffer, "%02d:%02d:%02d", gpsHour, gpsMinute, gpsSecond);
    return String(buffer);
}

// ====================== FUNCIONES CSV Y DISPLAY ======================

String PixhawkInterface::getCSVHeader() {
    return "Latitude,Longitude,Altitude,GPSYear,GPSMonth,GPSDay,GPSHour,GPSMinute,GPSSecond";
}

String PixhawkInterface::save_CSVData() {
    String data = "";
    data += String(latitude, 6) + ",";
    data += String(longitude, 6) + ",";
    data += String(altitude, 2) + ",";
    data += String(gpsYear) + ",";
    data += String(gpsMonth) + ",";
    data += String(gpsDay) + ",";
    data += String(gpsHour) + ",";
    data += String(gpsMinute) + ",";
    data += String(gpsSecond) + ",";
    return data;
}

void PixhawkInterface::show_message() {
    if (!connected) {
        LOG_WARN("PIXHAWK", "❌ PIXHAWK DESCONECTADO");
        return;
    }
    
    LOG_INFO("PIXHAWK", "=================== DATOS PIXHAWK ===================");

    // 🕐 MOSTRAR DATOS DE TIEMPO PRIMERO
    LOG_INFO("PIXHAWK", " TIEMPO GPS:");
    if (hasValidGPSTime()) {
        LOG_INFO("PIXHAWK", "  Fecha: " + getGPSDateString());
        LOG_INFO("PIXHAWK", "  Hora UTC: " + getGPSTimeOnlyString());
        LOG_INFO("PIXHAWK", "  Timestamp: " + String(gpsTimeUsec) + " μs");
    } else {
        LOG_WARN("PIXHAWK", "  Sin datos válidos de tiempo GPS");
    }

    // 📍 Datos de posición
    LOG_INFO("PIXHAWK", "📍 POSICIÓN:");
    LOG_INFO("PIXHAWK", "  Latitud: " + String(latitude, 6) + "°");
    LOG_INFO("PIXHAWK", "  Longitud: " + String(longitude, 6) + "°");
    LOG_INFO("PIXHAWK", "  Altitud: " + String(altitude, 2) + " m");
    LOG_INFO("PIXHAWK", "  Heading: " + String(heading, 1) + "°");
    
    // 🔋 Datos de batería
    LOG_INFO("PIXHAWK", "🔋 BATERÍA:");
    LOG_INFO("PIXHAWK", "  Voltaje: " + String(batteryVoltage, 2) + " V");
    LOG_INFO("PIXHAWK", "  Corriente: " + String(batteryCurrent, 2) + " A");
    LOG_INFO("PIXHAWK", "  Restante: " + String(batteryRemaining) + " %");
    LOG_INFO("PIXHAWK", "  Temperatura: " + String(batteryTemperature, 1) + " °C");
    
    // 📊 Datos adicionales
    LOG_INFO("PIXHAWK", "📊 NAVEGACIÓN:");
    LOG_INFO("PIXHAWK", "  Vel. tierra: " + String(groundSpeed, 1) + " m/s");
    LOG_INFO("PIXHAWK", "  Vel. aire: " + String(airSpeed, 1) + " m/s");
    LOG_INFO("PIXHAWK", "  Satélites: " + String(numSatellites));
    LOG_INFO("PIXHAWK", "  Estado: " + String(armed ? "ARMADO" : "DESARMADO"));
    
    LOG_INFO("PIXHAWK", "====================================================");
}