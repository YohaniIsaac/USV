#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Definir los pines CE y CSN
#define CE_PIN 2  // GPIO2
#define CSN_PIN 15 // GPIO15

// Crear una instancia de RF24
RF24 radio(CE_PIN, CSN_PIN);

// Dirección de comunicación (debe coincidir con la del transmisor)
const byte address[6] = "00001";

void setup() {
  // Iniciar la comunicación serial
  Serial.begin(115200);

  // Iniciar el módulo NRF24L01
  if (radio.begin()) {
    Serial.println("NRF24L01 inicializado correctamente.");
  } else {
    Serial.println("Error al inicializar el NRF24L01.");
    while (1);
  }

  // Configurar la dirección de comunicación
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);  // Ajusta el nivel de potencia (MIN, LOW, HIGH, MAX)
  radio.startListening();  // Poner el módulo en modo de recepción
}

void loop() {
  // Verificar si hay datos disponibles
  if (radio.available()) {
    int datos[2] = {0};  // Arreglo para almacenar los datos recibidos
    radio.read(&datos, sizeof(datos));  // Leer los datos

    // Mostrar los datos en el monitor serial
    Serial.println("Datos recibidos:");
    Serial.print("Estado D1: ");
    Serial.println(datos[0]);
    Serial.print("Estado D2: ");
    Serial.println(datos[1]);
  }
}