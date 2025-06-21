// SONAR NMEA2000 Reader - Versión Simplificada
// Solo lee y muestra señales NMEA2000 del sonar en Serial

#define ESP32_CAN_TX_PIN GPIO_NUM_2  // CAN TX pin
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // CAN RX pin

#include <Arduino.h>
#include <NMEA2000_CAN.h>
#include <NMEA2000_esp32.h>
#include <N2kMessages.h>
#include <N2kMsg.h>

// Declaraciones de funciones
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);
void PrintDepthData(const tN2kMsg &N2kMsg);
void PrintDistanceLog(const tN2kMsg &N2kMsg);
void PrintRawMessage(const tN2kMsg &N2kMsg);

void setup() {
  // Inicializar puerto serial
  Serial.begin(115200);
  Serial.println("NMEA2000 Sonar Reader iniciado");
  
  // Configurar handler de mensajes NMEA2000
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
  
  // Abrir puerto NMEA2000
  NMEA2000.Open();
  
  Serial.println("Esperando datos del sonar...");
}

void loop() {
  // Procesar mensajes NMEA2000
  NMEA2000.ParseMessages();
  
  // Pequeña pausa
  delay(100);
}

// Handler principal para mensajes NMEA2000
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  switch (N2kMsg.PGN) {
    case 128267L: // Water Depth
      PrintDepthData(N2kMsg);
      break;
    case 128275L: // Distance Log
      PrintDistanceLog(N2kMsg);
      break;
    default:
      // Mostrar otros mensajes (opcional)
      PrintRawMessage(N2kMsg);
      break;
  }
}

// Función para mostrar datos de profundidad
void PrintDepthData(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  
  if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range)) {
    Serial.print("Profundidad: ");
    if (DepthBelowTransducer != N2kDoubleNA) {
      Serial.print(DepthBelowTransducer);
      Serial.print(" m");
    } else {
      Serial.print("N/A");
    }
    
    Serial.print(" | Offset: ");
    if (Offset != N2kDoubleNA) {
      Serial.print(Offset);
      Serial.print(" m");
    } else {
      Serial.print("N/A");
    }
    
    Serial.print(" | Rango: ");
    if (Range != N2kDoubleNA) {
      Serial.print(Range);
      Serial.print(" m");
    } else {
      Serial.print("N/A");
    }
    
    Serial.println();
  }
}

// Función para mostrar datos del log de distancia
void PrintDistanceLog(const tN2kMsg &N2kMsg) {
  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  uint32_t Log;
  uint32_t TripLog;
  
  if (ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog)) {
    Serial.print("Log Total: ");
    if (Log != N2kUInt32NA) {
      Serial.print(Log);
      Serial.print(" m");
    } else {
      Serial.print("N/A");
    }
    
    Serial.print(" | Trip Log: ");
    if (TripLog != N2kUInt32NA) {
      Serial.print(TripLog);
      Serial.print(" m");
    } else {
      Serial.print("N/A");
    }
    
    Serial.println();
  }
}

// Función para mostrar mensaje raw (opcional, para debug)
void PrintRawMessage(const tN2kMsg &N2kMsg) {
  Serial.print("PGN: ");
  Serial.print(N2kMsg.PGN);
  Serial.print(" | Source: ");
  Serial.print(N2kMsg.Source);
  Serial.print(" | Data: ");
  
  for (int i = 0; i < N2kMsg.DataLen; i++) {
    if (N2kMsg.Data[i] < 16) Serial.print("0");
    Serial.print(N2kMsg.Data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}