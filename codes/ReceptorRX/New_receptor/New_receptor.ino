/* Heltec Automation Receive communication test example
 *
 * Function:
 * 1. Receive the same frequency band lora signal program
 *  
 * Description:
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:

 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series


 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "TelloESP32.h"
#include "HT_SSD1306Wire.h"
#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


using namespace TelloControl;

typedef struct {
  uint8_t payload[64];
  uint16_t size;
} DronCommand;
QueueHandle_t colaComandos;

// MODO DEPURACION
#define DEBUG 0
#if DEBUG
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#define TEST_WIFI_NO_SEND 0

// Almacenar ID
Preferences prefs;

struct Paquete {
  uint8_t id;
  int16_t rssi;
  int8_t snr;
  int batery_dron;
};
int LV_batery_dron=0;
int tiempo_vuelo=0;
const int MAX_PACKETS = 250;   // Maximo de paquetes
Paquete paquetes[MAX_PACKETS];
int indice = 0;                // Cuántos IDs has guardado

// CONFIGURACION LORA
#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_BANDWIDTH                              1         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30        // Define the payload size here
static uint8_t RcvBuffer[64];
static RadioEvents_t RadioEvents;
int16_t rssi,rxSize;
bool lora_idle = true;

// CONFIGURACION TELLO
//const char *TELLO_SSID = "TELLO-6365A8";   // Replace with your Tello's SSID
const char *TELLO_SSID2 = "TELLO-636ECF";   // Replace with your Tello's SSID
const char *TELLO_PASSWORD = "";          // Tello's WiFi password (usually empty)
boolean connected = false;
TelloESP32 tello;  // Create an instance of the TelloESP32 class

// Pantalla||
SSD1306Wire MyDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// NIVEL DE BATERIA
#define VBAT_PIN 1
#define VBAT_READ_CNTRL_PIN 37
#define ADC_READ_STABILIZE 10
int nVoltage = 0;


// Error handler callback function
void telloErrorHandler(const char* command, const char* errorMessage) {
    Serial.printf("[ERROR] Command '%s': %s\n", command, errorMessage);
}

//***********************************************************************************************************************
//***********************************************************TASK********************************************************
//***********************************************************************************************************************
void taskEjecutarComandos(void *parameter) {
  DronCommand comando;
  while (true) {
    if (xQueueReceive(colaComandos, &comando, portMAX_DELAY) == pdTRUE) {
      uint8_t bit_test  = (comando.payload[0] >> 7) & 0B00000001;
      uint8_t velocidad     = comando.payload[0] & 0B01111111;
      ejecutar_comando(64, velocidad);
      //vTaskDelay(pdMS_TO_TICKS(500));  // Espera 1000 ms de forma amigable para FreeRTOS
      
      ejecutar_comando(16, 0);  // TAKEOFF
      //vTaskDelay(pdMS_TO_TICKS(500));  // Espera 1000 ms de forma amigable para FreeRTOS
      
      for (int i = 1; i < comando.size; i++) {
        uint8_t instruccion = (comando.payload[i] >> 5) & 0B00000111;
        uint8_t d = comando.payload[i] & 0B00011111;
        uint16_t distancia = round(((d / 31.0) * (500 - 20)) + 20);
        ejecutar_comando(instruccion, distancia);
      }

      //vTaskDelay(pdMS_TO_TICKS(500));  // Espera 1000 ms de forma amigable para FreeRTOS
      ejecutar_comando(32, 0);  // LAND
    }
  }
}



//***********************************************************************************************************************
//*******************************************************MENSAJE LORA****************************************************
//***********************************************************************************************************************
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ){
  DEBUG_PRINTLN("Mensaje recibido.");
  if (size < 1) return;

  uint8_t bit_test  = (payload[0] >> 7) & 0B00000001; // 0 o 1
  int velocidad     = payload[0]        & 0B01111111; // 0 a 127
  digitalWrite(35, HIGH);   // Enciende el LED

  if (bit_test == 1) {
    uint8_t id = payload[1];
    //Serial.println(id);
    guardarNuevoID(id,rssi, snr, LV_batery_dron);  // <-- PASAS RSSI y SNR aquí
  } else {
    DronCommand cmd;
    memcpy(cmd.payload, payload, size);
    cmd.size = size;
    if (xQueueSend(colaComandos, &cmd, 100) != pdTRUE) {
      DEBUG_PRINTLN("Cola de comandos llena o error.");
    }
  }

  Radio.Rx(RX_TIMEOUT_VALUE);
  lora_idle = true;
}

void ejecutar_comando(uint8_t instruccion, uint16_t parametro) {
  TelloStatus status;    
  DEBUG_PRINT("Instruccion: ");
  DEBUG_PRINTLN(instruccion);
  DEBUG_PRINT("Distancia: ");
  DEBUG_PRINTLN(parametro);

  switch (instruccion) {
    case 0b000001:  
      DEBUG_PRINT("RIGHT   : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_right(parametro);
      
      break;

    case 0b000010:
      DEBUG_PRINT("LEFT   : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_left(parametro);
      break;
    
    case 0b000011:
      DEBUG_PRINT("UP     : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_up(parametro);
      break;

    case 0b000100:  
      DEBUG_PRINT("DOWN    : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_down(parametro);
      break;

    case 0b000101:
      DEBUG_PRINT("FORWARD: "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_forward(parametro);
      break;

    case 0b000110:
      DEBUG_PRINT("BACK   : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.move_back(parametro);
      break;

    case 0b00010000: 
      DEBUG_PRINT("TAKEOFF: "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.takeoff();
      break;

    case 0b00100000:
      DEBUG_PRINT("LAND   : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.land();
      break;

    case 0b01000000:
      DEBUG_PRINT("SPEED   : "); 
      DEBUG_PRINTLN(parametro); //tiempo_espera(parametro); 
      status = tello.set_speed(parametro);
      break;

    default:        
      DEBUG_PRINTLN("Comando no reconocido");
      break;
  }

  // Error status handling
  if (status != TelloStatus::OK) {
    switch(status) {
      case TelloStatus::Timeout: DEBUG_PRINTLN("Command failed: Timeout"); break;
      case TelloStatus::NotConnected: DEBUG_PRINTLN("Command failed: Not Connected"); break;
      case TelloStatus::NoResponse: DEBUG_PRINTLN("Command failed: No Response"); break;
      case TelloStatus::InvalidParameter: DEBUG_PRINTLN("Command failed: Invalid Parameter"); break;
      default: break; // Don't handle other cases as they're covered by the callback
    }
  }else{
    DEBUG_PRINTLN("Status OK");
  }

  LV_batery_dron=tello.get_battery();
}

//***********************************************************************************************************************
//**********************************************************DISPLAY******************************************************
//***********************************************************************************************************************
void mostrarPantallaWiFi(const String& estado, const String& ssid, const IPAddress& ip, int nivelBateria) {
  MyDisplay.clear();
  MyDisplay.setFont(ArialMT_Plain_10);
  MyDisplay.drawString(8, 0, estado);
  MyDisplay.drawString(10, 20, "SSID: " + ssid);
  MyDisplay.drawString(10, 35, "IP: " + ip.toString());
  MyDisplay.drawString(10, 50, "Bateria: "+ String(nivelBateria));
  MyDisplay.display();
}

void menu_inicio(){
  MyDisplay.clear();
  MyDisplay.setFont(ArialMT_Plain_16);
  MyDisplay.drawString(20, 0, "U. CUENCA");
  MyDisplay.setFont(ArialMT_Plain_10);
  MyDisplay.drawString(0, 20, "J. Cambisaca");
  MyDisplay.drawString(0, 35, "H. Castro");
  MyDisplay.drawString(0, 50, "H. Maldonado");
  MyDisplay.display();
}

//***********************************************************************************************************************
//**********************************************************BATERIA******************************************************
//***********************************************************************************************************************
void inicio_bateria(){
  // turn on vbat read
  pinMode(VBAT_READ_CNTRL_PIN,OUTPUT);
  digitalWrite(VBAT_READ_CNTRL_PIN, LOW);

  // LEER NIVEL BATERIA
  nVoltage = nivel_bateria(); 
}

int nivel_bateria(){
  int analogValue = analogRead(VBAT_PIN);
  float voltage = 0.097752 * analogValue;  // Calcular el voltaje
  return round(voltage);
}


//***********************************************************************************************************************
//**********************************************************TEST ID******************************************************
//***********************************************************************************************************************
void guardarNuevoID(uint8_t nuevo_id, int16_t rssi, int8_t snr, int batery_dron) {
  if (indice >= MAX_PACKETS) {
    DEBUG_PRINTLN("Memoria llena");
    return;
  }

  paquetes[indice].id = nuevo_id;
  paquetes[indice].rssi = rssi;
  paquetes[indice].snr = snr;
  paquetes[indice].batery_dron = batery_dron;
  

  indice++;

  prefs.begin("packetData", false);
  prefs.putBytes("paquetes", paquetes, sizeof(paquetes));
  prefs.putInt("indice", indice);
  prefs.end();

  DEBUG_PRINT("Guardado ID: ");
  DEBUG_PRINT(nuevo_id);
  DEBUG_PRINT(" RSSI: ");
  DEBUG_PRINT(rssi);
  DEBUG_PRINT(" SNR: ");
  DEBUG_PRINTLN(snr);
  DEBUG_PRINT(" Batery dron: ");
  DEBUG_PRINTLN(batery_dron);
}


void tiempo_espera(int distancia){
  int tiempo=distancia*10;
  delay(tiempo);
}


//***********************************************************************************************************************
//*************************************************************INICIO****************************************************
//***********************************************************************************************************************
void setup() {
  Serial.begin(115200);
  // Inicio Display
  pinMode(35, OUTPUT);      // Configura el pin como salida
  digitalWrite(35, LOW);   // Enciende el LED
  delay(500);

  //MyDisplay.init();
  //menu_inicio();
  DEBUG_PRINTLN("Heltec V3 inicializado...");

  colaComandos = xQueueCreate(5, sizeof(DronCommand));
  xTaskCreatePinnedToCore(
    taskEjecutarComandos,   // Task function
    "EjecutarComandos",     // Name
    4096,                   // Stack size
    NULL,                   // Parameters
    1,                      // Priority
    NULL,                   // Task handle
    1                       // Core (ESP32 tiene 2: 0 y 1)
  );

  // Set the error handler callback
  //if(DEBUG==0){
  if (DEBUG ==1) {
    tello.setErrorCallback(telloErrorHandler);
    TelloStatus status = tello.connect(TELLO_SSID2, TELLO_PASSWORD);
    while (status != TelloStatus::OK){
      DEBUG_PRINTLN("Connection failed!");
      status = tello.connect(TELLO_SSID2, TELLO_PASSWORD);
    }
  } 
  //}
  
  
  // Inicio Display
  MyDisplay.init();
  menu_inicio();
  inicio_bateria();
  
  // Inicio LoRa
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
  rssi = 0;
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, 
                    LORA_CODINGRATE,0, LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
                    LORA_FIX_LENGTH_PAYLOAD_ON, 0, true,0, 0, LORA_IQ_INVERSION_ON, true);
}

void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    DEBUG_PRINTLN("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}