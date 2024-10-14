#include <WiFi.h>

//------------------Servidor Web en puerto 80---------------------
WiFiServer server(80);

//---------------------Credenciales de WiFi-----------------------
const char* ssid     = "Tu wifi";
const char* password = "Tu contraseña";
int dom;

//---------------------VARIABLES GLOBALES-------------------------
int contconexion = 0;

String header;  // Variable para guardar el HTTP request

String estadoSalida = "off";
const int motor = 4;
const int salida = 2;

// Sensores de temperatura y humedad
int temp = 35;   // Pin del sensor de temperatura
int humedad = 34; // Pin del sensor de humedad

int readtemp;       // Lectura del sensor de temperatura
int readhum;        // Lectura del sensor de humedad
float voltage;      // Voltaje del sensor de temperatura
float temperature;  // Temperatura calculada
float humidity;     // Humedad calculada (inversa)

//------------------------CODIGO HTML------------------------------
String pagina = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset='utf-8' />"
                "<meta http-equiv='refresh' content='5'>"  // Recargar la página cada 5 segundos
                "<title>Web Server</title>"
                "</head>"
                "<body>"
                "<center>"
                "<h1>Web Server</h1>"
                "<p><a href='/on'><button style='height:50px;width:100px'>ON</button></a></p>"
                "<p><a href='/off'><button style='height:50px;width:100px'>OFF</button></a></p>"
                "<h2>Data of the sensors</h2>"
                "<p>Temperature: %TEMP% °C</p>"
                "<p>Humidity: %HUM% %</p>"
                "</center>"
                "</body>"
                "</html>";

//---------------------------SETUP--------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("");

  pinMode(salida, OUTPUT);
  digitalWrite(salida, LOW);

  pinMode(motor, OUTPUT);
  digitalWrite(motor, LOW);

  // Configurar los pines de los sensores
  pinMode(temp, INPUT);
  pinMode(humedad, INPUT);

  // Conexión WIFI
  WiFi.begin(ssid, password);
  // Cuenta hasta 50 si no se puede conectar lo cancela
  while (WiFi.status() != WL_CONNECTED and contconexion < 50) {
    ++contconexion;
    delay(500);
    Serial.print(".");
  }
  if (contconexion < 50) {
    //para usar con ip fija
    IPAddress ip(192, 168, 0, 13);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(ip, gateway, subnet);

    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println(WiFi.localIP());
    server.begin();  // Iniciamos el servidor
  } else {
    Serial.println("");
    Serial.println("Error de conexión");
  }
}

//----------------------------LOOP----------------------------------
void loop() {
  WiFiClient client = server.available();   // Escucha a los clientes entrantes

  if (client) {                             // Si se conecta un nuevo cliente
    Serial.println("New Client.");          //
    String currentLine = "";                //
    while (client.connected()) {            // loop mientras el cliente está conectado
      if (client.available()) {             // si hay bytes para leer desde el cliente
        char c = client.read();             // lee un byte
        Serial.write(c);                    // imprime ese byte en el monitor serial
        header += c;
        if (c == '\n') {                    // si el byte es un caracter de salto de línea
          // si la nueva línea está en blanco significa que es el fin del
          // HTTP request del cliente, entonces respondemos:
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Enciende y apaga el GPIO
            if (header.indexOf("GET /on") >= 0) {
              Serial.println("GPIO on");
              estadoSalida = "on";
              digitalWrite(salida, LOW);
            } else if (header.indexOf("GET /off") >= 0) {
              Serial.println("GPIO off");
              estadoSalida = "off";
              digitalWrite(salida, HIGH);
            }

            // Leer los sensores
            readSensorData();
            if (estadoSalida == "on") {
              readH();
              dom = 1;
            }
            else {
              dom = 0;
            }

            // Actualizar la página web con los datos de los sensores
            String response = pagina;
            response.replace("%TEMP%", String(temperature, 1));  // Reemplaza el marcador de temperatura
            response.replace("%HUM%", String(humidity));       // Reemplaza el marcador de humedad

            // Mostrar la página web con los datos actualizados
            client.println(response);

            // La respuesta HTTP termina con una línea en blanco
            client.println();
            break;
          } else {  // si tenemos una nueva línea limpiamos currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // si C es distinto al caracter de retorno de carro
          currentLine += c;      // lo agrega al final de currentLine
        }
      }
    }
    // Limpiamos la variable header
    header = "";
    // Cerramos la conexión
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  if (dom == 1) {
    readH();
  }
  delay(500);
}

//-------------------------FUNCIONES DE LECTURA DE SENSORES-------------------
void readSensorData() {
  // Lectura del sensor de temperatura (LM35)
  readtemp = analogRead(temp);  // Leer el valor analógico
  voltage = (readtemp / 4095.0) * 5;  // Convertir el valor a voltaje
  temperature = voltage * 100;  // Convertir el voltaje a temperatura (LM35 genera 10mV/°C)

  // Lectura del sensor de humedad
  readhum = analogRead(humedad);  // Leer el valor analógico

  // Normalizamos el valor de humedad (invertido)
  humidity = 100 - ((readhum / 4095.0) * 100);  // Invertimos el valor de humedad

  // Mostrar los valores en el monitor serial
  Serial.print("Temperatura: ");
  Serial.print(temperature, 1);
  Serial.print(" °C, Humedad: ");
  Serial.print(humidity);
  Serial.println(" %");
}
void readH() {
  readhum = analogRead(humedad);  // Lee el valor analógico del sensor.
  int humedadPorcentaje = map(readhum, 0, 4095, 100, 0);
  if (humedadPorcentaje == 0) {
    digitalWrite(motor, 1);
    delay(600);
    digitalWrite(motor, 0);
  }
}
