#include <LoRaWan-SX126x.h>  //http://librarymanager/All#SX126x
#include <SPI.h>
#include <Tello.h>


// CONFIGURACION TELLO
//const char * networkName = "TELLO-6365A8"; //Replace with your Tello SSID
const char * networkName = "TELLO-636639"; //Replace with your Tello SSID
//const char * networkName = "ismael"; //Replace with your Tello SSID
//const char * networkPswd = "isma1234";
const char * networkPswd = "";
boolean connected = false;
Tello tello;

// CONFIGURACION LORA
hw_config hwConfig;
int PIN_LORA_RESET = 12;  // LORA RESET
int PIN_LORA_BUSY = 13;  // LORA SPI BUSY
int PIN_LORA_DIO_1 = 14; // LORA DIO_1
int PIN_LORA_NSS = 8;   // LORA SPI CS
int PIN_LORA_SCLK = 9;  // LORA SPI CLK
int PIN_LORA_MISO = 11;  // LORA SPI MISO
int PIN_LORA_MOSI = 10;  // LORA SPI MOSI
int RADIO_TXEN = 6;   // LORA ANTENNA TX ENABLE
int RADIO_RXEN = 7;   // LORA ANTENNA RX ENABLE
#define RF_FREQUENCY               915000000  // Hz
#define TX_OUTPUT_POWER            22    // dBm
#define LORA_BANDWIDTH             1    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR      10 // [SF7..SF12]
#define LORA_CODINGRATE            1   // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH       8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT        0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false
#define RX_TIMEOUT_VALUE           3000
#define TX_TIMEOUT_VALUE           3000


static RadioEvents_t RadioEvents;
static uint8_t RcvBuffer[64];
char ReceivedPayload[256];
bool bandera=false;
uint8_t *trama_vector = nullptr; // Puntero global para el vector
int vector_size = 0;       // Tamaño global del vector
bool tamanio_definido = false; // Bandera global para indicar si el tamaño del vector ha sido definido



// FUNCIONES
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);
void connectToWiFi(const char * ssid, const char * pwd);
void WiFiEvent(WiFiEvent_t event);
void init_lora();
void ejecutar_comando(char *mensaje);
void tiempo_espera(int distancia);
void ejecutar_comando(uint8_t instruccion, uint16_t parametro);


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  init_lora();
  connectToWiFi(networkName, networkPswd);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  // Handle Radio events
  Radio.IrqProcess();

  // We are on FreeRTOS, give other tasks a chance to run
  delay(100);
  yield();
}
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/**@brief Function to be executed on Radio Rx Done event*/
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Serial.println("OnRxDone");

  // Copiar el payload recibido al buffer de recepción
  memcpy(RcvBuffer, payload, size);

  // Obtener el número de instrucciones del primer byte del buffer
  uint8_t numInstrucciones = RcvBuffer[0];

  // Imprimir el valor de numInstrucciones
  Serial.print("Cantidad de instrucciones recibidas: ");
  Serial.println(numInstrucciones);

  // Verificar si el primer byte está con todos los bits en 1
  if (numInstrucciones == 0xFF) {
    // Ejecutar solo los comandos de despegue y aterrizaje
    Serial.println("Comando especial recibido: TAKEOFF y LAND");

    ejecutar_comando(16, 0); // Comando de TAKEOFF
    ejecutar_comando(32, 0); // Comando de LAND
  } else {
    // Procesar las instrucciones recibidas
    ejecutar_comando(16, 0); // Comando de TAKEOFF
    for (int i = 1; i < size; i += 2) {
      if (i + 1 < size) {
        // Obtener los bytes alto y bajo
        uint8_t highByte = RcvBuffer[i];
        uint8_t lowByte = RcvBuffer[i + 1];

        // Reconstruir el comando
        uint16_t comando = (highByte << 8) | lowByte;

        // Extraer la instrucción y el parámetro del comando
        uint8_t instruccion = (comando >> 12) & 0xF; // 4 bits más significativos
        uint16_t parametro = comando & 0x1FF;        // 9 bits menos significativos

        // Imprimir la instrucción y el parámetro
        Serial.print("Instrucción recibida: ");
        Serial.print(instruccion);
        Serial.print(", Parámetro recibido: ");
        Serial.println(parametro);

        ejecutar_comando(instruccion, parametro);
      }
    }
    ejecutar_comando(32, 0); // Comando de LAND
  }
}

/**@brief Function to be executed on Radio Rx Timeout event*/
void OnRxTimeout(void){
  Serial.println("OnRxTimeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Error event
 */
void OnRxError(void){
  Serial.println("OnRxError");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void connectToWiFi(const char * ssid, const char * pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  //Initiate connection
  WiFi.begin(ssid, pwd);
  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) 
  {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    //case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initialise Tello after we are connected
      tello.init();
      connected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    //case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      //connectToWiFi(networkName, networkPswd);
      break;
  }
}

void init_lora(){
  // Define the HW configuration between MCU and SX126x
  hwConfig.CHIP_TYPE = SX1262_CHIP;          // Example uses an eByte E22 module with an SX1262
  hwConfig.PIN_LORA_RESET = PIN_LORA_RESET;  // LORA RESET
  hwConfig.PIN_LORA_NSS = PIN_LORA_NSS;      // LORA SPI CS
  hwConfig.PIN_LORA_SCLK = PIN_LORA_SCLK;    // LORA SPI CLK
  hwConfig.PIN_LORA_MISO = PIN_LORA_MISO;    // LORA SPI MISO
  hwConfig.PIN_LORA_DIO_1 = PIN_LORA_DIO_1;  // LORA DIO_1
  hwConfig.PIN_LORA_BUSY = PIN_LORA_BUSY;    // LORA SPI BUSY
  hwConfig.PIN_LORA_MOSI = PIN_LORA_MOSI;    // LORA SPI MOSI
  hwConfig.RADIO_TXEN = RADIO_TXEN;          // LORA ANTENNA TX ENABLE
  hwConfig.RADIO_RXEN = RADIO_RXEN;          // LORA ANTENNA RX ENABLE
  hwConfig.USE_DIO2_ANT_SWITCH = true;       // Example uses an CircuitRocks Alora RFM1262 which uses DIO2 pins as antenna control
  hwConfig.USE_DIO3_TCXO = true;             // Example uses an CircuitRocks Alora RFM1262 which uses DIO3 to control oscillator voltage
  hwConfig.USE_DIO3_ANT_SWITCH = false;      // Only Insight ISP4520 module uses DIO3 as antenna control
  hwConfig.USE_RXEN_ANT_PWR = true;

  // Initialize Serial for debug output
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");

  // Initialize the LoRa chip
  if (!lora_hardware_init(hwConfig))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  // Initialize the Radio callbacks
  RadioEvents.TxDone = NULL;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = NULL;

  // Initialize the Radio
  Radio.Init(&RadioEvents);

  // Set Radio channel
  Radio.SetChannel(RF_FREQUENCY);

  // Set Radio TX configuration
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
            LORA_SPREADING_FACTOR, LORA_CODINGRATE,
            LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
            true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
  
  // Start LoRa
  Serial.println("Starting Radio.Rx");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void ejecutar_comando(uint8_t instruccion, uint16_t parametro) {
    if (!connected) {
        Serial.println("No conectado al WiFi, comando no ejecutado.");
        return;
    }

    switch (instruccion) {
        case 0b000001:  // RIGHT
            tello.right(parametro);
            Serial.print("Enviando comando: RIGHT ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b000010:  // LEFT
            tello.left(parametro);
            Serial.print("Enviando comando: LEFT ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b000011:  // UP
            tello.up(parametro);
            Serial.print("Enviando comando: UP ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b000100:  // DOWN
            tello.down(parametro);
            Serial.print("Enviando comando: DOWN ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b000101:  // FORWARD
            tello.forward(parametro);
            Serial.print("Enviando comando: FORWARD ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b000110:  // BACK
            tello.back(parametro);
            Serial.print("Enviando comando: BACK ");
            Serial.println(parametro);
            tiempo_espera(parametro);
            break;
        case 0b010000:  // TAKEOFF
            tello.takeoff();
            Serial.println("Enviando comando: TAKEOFF");
            delay(10000);
            break;
        case 0b100000:  // LAND
            tello.land();
            Serial.println("Enviando comando: LAND");
            delay(5000);
            break;
        default:
            Serial.println("Comando no reconocido");
            break;
    }

    // Agregar tiempo de espera entre comandos
    delay(5000);  // Delay de 5 segundos para cada comando
}

void tiempo_espera(int distancia){
  int tiempo=distancia*1;
  delay(tiempo);
}

