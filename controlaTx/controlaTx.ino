#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

#define J_IX 4
#define J_IY 5
#define J_DX 6
#define J_DY 7

#define CE_PIN 38
#define CSN_PIN 39

const int IX_CT = 2045; // Valor central de IX
const int IX_DZ = 20;   // Zona muerta de IX

const int IY_CT = 2045; // Valor central de IY
const int IY_DZ = 20;   // Zona muerta de IY

const int DX_CT = 2045; // Valor central de DX
const int DX_DZ = 20;   // Zona muerta de DX

const int DY_CT = 2045; // Valor central de DY
const int DY_DZ = 20;   // Zona muerta de DY

RF24 radio(CE_PIN, CSN_PIN);  // Constructor normal
const byte NRF_Address[6] = "USVRX"; // Dirección para la comunicación

typedef struct DataPacket {
  int CH1;
  int CH2;
  int CH3;
  int CH4;
} DataPacket;

DataPacket dataToSend;

int potPins[4] = {J_IX, J_IY, J_DX, J_DY};
int potValues[4];

unsigned long lastSent = 0;
const unsigned long interval = 20; // ms

void setup() {
  Serial.begin(115200);
  Serial.println("ESTE ES EL CÓDIGO DEL EMISOR V1.3");

  // Configura los pines personalizados para VSPI
  SPI.begin(36, 37, 35, CSN_PIN);  // SCK, MISO, MOSI, SS

  // Este paso es esencial para que la librería use el SPI ya inicializado
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(124);
  radio.openWritingPipe(NRF_Address);
  radio.stopListening();

  pinMode(CSN_PIN, OUTPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(potPins[i], INPUT);
  }

  if (radio.isChipConnected()) {
    Serial.println("NRF24L01 conectado y SPI funcionando");
  } else {
    Serial.println("Error: NRF24L01 no conectado o SPI falló");
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastSent >= interval) {
    unsigned long delta = now - lastSent;
    lastSent = now;

    // Leer los valores de los 4 potenciómetros
    for (int i = 0; i < 4; i++) {
      potValues[i] = analogRead(potPins[i]);
    }

    int IX, IY, DX, DY;

    // Mapeo de IX (canal 1)
    if (potValues[0] < IX_CT - IX_DZ) {
      IX = map(potValues[0], 0, IX_CT - IX_DZ, 1000, 1500);
    }
    else if (potValues[0] > IX_CT + IX_DZ) {
      IX = map(potValues[0], IX_CT + IX_DZ, 4095, 1500, 2000);
    }
    else {
      IX = 1500;
    }

    // Mapeo de IY (canal 2)
    if (potValues[1] < IY_CT - IY_DZ) {
      IY = map(potValues[1], 0, IY_CT - IY_DZ, 1000, 1500);
    }
    else if (potValues[1] > IY_CT + IY_DZ) {
      IY = map(potValues[1], IY_CT + IY_DZ, 4095, 1500, 2000);
    }
    else {
      IY = 1500;
    }

    // Mapeo de DX (canal 3)
    if (potValues[2] < DX_CT - DX_DZ) {
      DX = map(potValues[2], 0, DX_CT - DX_DZ, 1000, 1500);
    }
    else if (potValues[2] > DX_CT + DX_DZ) {
      DX = map(potValues[2], DX_CT + DX_DZ, 4095, 1500, 2000);
    }
    else {
      DX = 1500;
    }

    // Mapeo de DY (canal 4)
    if (potValues[3] < DY_CT - DY_DZ) {
      DY = map(potValues[3], 0, DY_CT - DY_DZ, 1000, 1500);
    }
    else if (potValues[3] > DY_CT + DY_DZ) {
      DY = map(potValues[3], DY_CT + DY_DZ, 4095, 1500, 2000);
    }
    else {
      DY = 1500;
    }

    // Asignación de los valores PWM para cada canal
    int pwm_CH1 = IX;
    int pwm_CH2 = IY;
    int pwm_CH3 = DX;
    int pwm_CH4 = DY;

    // Constrain los valores PWM a un rango de 1000 a 2000
    pwm_CH1 = constrain(pwm_CH1, 1000, 2000);
    pwm_CH2 = constrain(pwm_CH2, 1000, 2000);
    pwm_CH3 = constrain(pwm_CH3, 1000, 2000);
    pwm_CH4 = constrain(pwm_CH4, 1000, 2000);

    // Asignación de los valores al paquete de datos
    dataToSend.CH1 = pwm_CH1;
    dataToSend.CH2 = pwm_CH2;
    dataToSend.CH3 = pwm_CH3;
    dataToSend.CH4 = pwm_CH4;

    // Enviar los datos a través del radio
    bool sent = radio.write(&dataToSend, sizeof(dataToSend));

    // Mostrar los valores leídos y enviados en el monitor serial
    Serial.print("Lecturas: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(potValues[i]);
      if (i < 3) Serial.print(", ");
    }

    Serial.print(" (");
    Serial.print(pwm_CH1);
    Serial.print(", ");
    Serial.print(pwm_CH2);
    Serial.print(", ");
    Serial.print(pwm_CH3);
    Serial.print(", ");
    Serial.print(pwm_CH4);
    Serial.print(")");

    Serial.print(sent ? " - Datos enviados correctamente" : " - Fallo al enviar");
    Serial.print(" - Δt: ");
    Serial.print(delta);
    Serial.println(" ms");
  }
}
