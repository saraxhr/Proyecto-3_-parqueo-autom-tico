#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>

// Definición de pines para el display de 7 segmentos
const int segmentPins[] = {12, 13, 26, 25, 33, 14, 27}; // Pines A, B, C, D, E, F, G

// Códigos binarios para cada número en el display
const byte digitCodes[] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

const int i2cAddress = 0x08; // Dirección I2C del ESP32 (esclavo)
volatile int availableSpaces = 0; // Espacios disponibles recibidos
const int totalSpaces = 4; // Número total de parqueos
int occupiedSpaces = 0; // Espacios ocupados

// Configuración de red WiFi
const char* ssid = "Sara";
const char* password = "baileybebe123";

// Configuración del servidor web
WebServer server(80);

void setup() {
  // Configuración de los pines del display de 7 segmentos como salidas
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], LOW); // Apaga todos los segmentos inicialmente
  }

  // Inicializa I2C como esclavo
  Wire.begin(i2cAddress);
  Wire.onReceive(receiveData);
  
  Serial.begin(115200);
  Serial.println("ESP32 iniciado y listo para recibir datos por I2C.");

  // Conexión a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexión WiFi establecida");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configura el servidor web
  server.on("/", handleRoot);
  server.begin();

  // Muestra el número inicial de espacios disponibles en el display
  displayDigit(availableSpaces);
}

void loop() {
  // Calcula el número de espacios ocupados
  occupiedSpaces = totalSpaces - availableSpaces;

  // Muestra el número actualizado de espacios disponibles en el display
  displayDigit(availableSpaces);

  // Maneja las solicitudes del servidor web
  server.handleClient();

  delay(1000); // Actualiza cada segundo para visualizar el cambio en el display
}

void displayDigit(int digit) {
  // Asegura que el dígito esté en el rango de 0 a 9
  if (digit < 0 || digit > 9) return;

  // Obtiene el código binario correspondiente al número a mostrar
  byte code = digitCodes[digit];
  
  // Activa o desactiva cada segmento en función del código binario
  for (int i = 0; i < 7; i++) {
    bool segmentState = (code >> i) & 0x01; // Extrae el bit correspondiente al segmento
    digitalWrite(segmentPins[i], segmentState ? HIGH : LOW); // Controla cada segmento
  }
}

// Función de interrupción para recibir datos del NUCLEO
void receiveData(int bytes) {
  if (Wire.available()) {
    availableSpaces = Wire.read(); // Lee el número de espacios disponibles
    Serial.print("Espacios disponibles recibidos: ");
    Serial.println(availableSpaces);
  }
}

// Manejador para la página principal
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Parqueo-matic - Disponibilidad de Parqueos</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f0;
      color: #333;
      display: flex;
      flex-direction: column;
      align-items: center;
      margin: 0;
      padding: 0;
    }
    .header {
      font-size: 2em;
      font-weight: bold;
      color: #4CAF50;
      margin: 20px 0;
    }
    .status-board {
      background-color: #222;
      color: white;
      padding: 15px;
      border-radius: 8px;
      display: flex;
      justify-content: space-around;
      width: 100%;
      max-width: 600px;
    }
    .status-board div {
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .status-board div span {
      font-size: 1.5em;
      font-weight: bold;
    }
    .update-time {
      font-size: 1.2em;
      color: #4CAF50;
      margin-top: 20px;
      animation: pulse 1.5s infinite;
    }
    @keyframes pulse {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.1); }
    }
  </style>
</head>
<body>
  <div class="header">Parqueo-matic - Disponibilidad de Parqueos</div>
  <div class="status-board">
    <div>
      <span id="occupiedSpaces">%OCCUPIED_SPACES%</span>
      Ocupados
    </div>
    <div>
      <span id="availableSpaces">%AVAILABLE_SPACES%</span>
      Disponibles
    </div>
  </div>
  <div class="update-time">Última actualización: <span id="updateTime">10:00 PM</span></div>
</body>
</html>
  )rawliteral";

  // Reemplaza los marcadores de posición con los valores actuales
  html.replace("%OCCUPIED_SPACES%", String(occupiedSpaces));
  html.replace("%AVAILABLE_SPACES%", String(availableSpaces));

  server.send(200, "text/html", html);
}
