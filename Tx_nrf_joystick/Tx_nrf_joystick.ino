#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Lectura de los pines analógicos
#define PIN_D1 35 
#define PIN_D2 34  

// Definir los pines CE y CSN
#define CE_PIN 4
#define CSN_PIN 5

// Crear una instancia de RF24
RF24 radio(CE_PIN, CSN_PIN);

// Dirección de comunicación
const byte address[6] = "00001";

void setup() {
  // Iniciar la comunicación serial
  Serial.begin(115200);

  // Configuración de los pines
  pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT);

  // Iniciar el módulo NRF24L01
  if (radio.begin()) {
    Serial.println("NRF24L01 inicializado correctamente.");
  } else {
    Serial.println("Error al inicializar el NRF24L01.");
    while (1);
  }

  // Configurar la dirección de comunicación
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);  // Puedes ajustar el nivel de potencia (MIN, LOW, HIGH, MAX)
  radio.stopListening();  // Poner el módulo en modo de transmisión
}

void loop() {
  // Lectura de los sensores analógicos
  int estadoD1 = analogRead(PIN_D1);
  int estadoD2 = analogRead(PIN_D2);

  // Crear un arreglo para enviar los datos
  int datos[2] = {estadoD1, estadoD2};

  // Enviar los datos
  bool enviado = radio.write(&datos, sizeof(datos));

  // Verificar si los datos fueron enviados correctamente
  if (!enviado) {
    Serial.println("Datos enviados correctamente:");
    Serial.print("Estado D1: ");
    Serial.println(estadoD1);
    Serial.print("Estado D2: ");
    Serial.println(estadoD2);
  } else {
    Serial.println("Error al enviar los datos.");
  }

  delay(1000);  // Esperar 1 segundo antes de la siguiente lectura
}
