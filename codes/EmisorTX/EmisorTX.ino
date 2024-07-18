#include "img.h"
#include "HT_SSD1306Wire.h"
#include <Keypad.h>
#include "arduino.h"
#include <LoRaWan-SX126x.h>  //http://librarymanager/All#SX126x
#include <SPI.h>

void impTexto(int posX, int posY, String cadenaTexto);
void menu0();
void menu3();
void menu3a();
void menu2a();
void seleccion2a();
void seleccion3a();
String seleccion3();
int ingresarValor(int posX, int posY, String cadenaTexto);
void impRetroalimentacion16(int posX, int posY, String cadenaTexto);
String verificarYConcatenar(int arriba, int abajo, int izq, int der, int fret, int atras);
void imprimirRutaEnColumnas(String cadenaTexto);
void impRuta(String cadenaTexto, int posX, int posY);
int readBatLevel();
String eliminarTakeoffYLand(String cadena);

int contarInstrucciones(String str);
void parsearInstrucciones(String str, uint8_t *trama);
uint8_t mapearInstruccion(String instruccion);

// Lora functions
void OnTxDone(void);
void OnTxTimeout(void);

SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

const byte filas = 4; 
const byte columnas = 3;
//byte pinesFilas[]  = {45,38,39,41};
//byte pinesColumnas[] = {42,46,40};
byte pinesFilas[] = {41,7,5,1};
byte pinesColumnas[] =	{39,45,3};
char teclas[4][3] = {{'1','2','3'},
                     {'4','5','6'},
                     {'7','8','9'},
                     {'*','0','#'}};
Keypad teclado1 = Keypad( makeKeymap(teclas), pinesFilas, pinesColumnas, filas, columnas);

//char key;
//int dato;

#define VBAT_PIN 1
#define VBAT_READ_CNTRL_PIN 37 // Heltec GPIO to toggle VBatt read connection …
// Also, take care NOT to have ADC read connection
// in OPEN DRAIN when GPIO goes HIGH
#define ADC_READ_STABILIZE 10 // in ms (delay from GPIO control and ADC connections times)

int commdArriba = 0;

/// LORA
hw_config hwConfig;

// Microcontroller - SX126x pin configuration
// Microcontroller - SX126x pin configuration: Heltec Wifi V3
int PIN_LORA_RESET = 12;  // LORA RESET
int PIN_LORA_BUSY = 13;  // LORA SPI BUSY
int PIN_LORA_DIO_1 = 14; // LORA DIO_1
int PIN_LORA_NSS = 8;   // LORA SPI CS
int PIN_LORA_SCLK = 9;  // LORA SPI CLK
int PIN_LORA_MISO = 11;  // LORA SPI MISO
int PIN_LORA_MOSI = 10;  // LORA SPI MOSI
int RADIO_TXEN = 6;   // LORA ANTENNA TX ENABLE
int RADIO_RXEN = 7;   // LORA ANTENNA RX ENABLE
#define RF_FREQUENCY 915000000  // Hz
#define TX_OUTPUT_POWER 22    // dBm
#define LORA_BANDWIDTH 1    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 10 // [SF7..SF12]
#define LORA_CODINGRATE 1   // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000
static RadioEvents_t RadioEvents;
static uint8_t TxdBuffer[64];

int nVoltage = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n\n");

  // Define the HW configuration between MCU and SX126x
  hwConfig.CHIP_TYPE = SX1262_CHIP;     // Example uses an eByte E22 module with an SX1262
  hwConfig.PIN_LORA_RESET = PIN_LORA_RESET; // LORA RESET
  hwConfig.PIN_LORA_NSS = PIN_LORA_NSS;  // LORA SPI CS
  hwConfig.PIN_LORA_SCLK = PIN_LORA_SCLK;   // LORA SPI CLK
  hwConfig.PIN_LORA_MISO = PIN_LORA_MISO;   // LORA SPI MISO
  hwConfig.PIN_LORA_DIO_1 = PIN_LORA_DIO_1; // LORA DIO_1
  hwConfig.PIN_LORA_BUSY = PIN_LORA_BUSY;   // LORA SPI BUSY
  hwConfig.PIN_LORA_MOSI = PIN_LORA_MOSI;   // LORA SPI MOSI
  hwConfig.RADIO_TXEN = RADIO_TXEN;     // LORA ANTENNA TX ENABLE
  hwConfig.RADIO_RXEN = RADIO_RXEN;     // LORA ANTENNA RX ENABLE
  hwConfig.USE_DIO2_ANT_SWITCH = true;    // Example uses an CircuitRocks Alora RFM1262 which uses DIO2 pins as antenna control
  hwConfig.USE_DIO3_TCXO = true;        // Example uses an CircuitRocks Alora RFM1262 which uses DIO3 to control oscillator voltage
  hwConfig.USE_DIO3_ANT_SWITCH = false;  // Only Insight ISP4520 module uses DIO3 as antenna control
  hwConfig.USE_RXEN_ANT_PWR = true;

  // Initialize Serial for debug output
  Serial.println("LoRa Sender");

  // Initialize the LoRa chip
    if (!lora_hardware_init(hwConfig))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  // Initialize the Radio callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = NULL;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = NULL;
  RadioEvents.RxError = NULL;
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

  // turn on vbat read
  pinMode(VBAT_READ_CNTRL_PIN,OUTPUT);
  digitalWrite(VBAT_READ_CNTRL_PIN, LOW);

  display.init();
  // Limpiar la pantalla
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  // Dibujar el texto en la pantalla
  display.drawString(20, 0, "U. CUENCA");
  // Mostrar el contenido en la pantalla
  display.setFont(ArialMT_Plain_10);
  // Dibujar el texto en la pantalla
  display.drawString(0, 20, "J. Cambisaca");
  display.drawString(0, 35, "H. Castro");
  display.drawString(0, 50, "H. Maldonado");
  display.drawXbm(80, 30, 40, 40, logos[0]);
  display.display();

  nVoltage = readBatLevel();

  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Handle Radio events
  Radio.IrqProcess();
  delay(100);// We are on FreeRTOS, give other tasks a chance to run
  yield();

  menu0();
  //Verifica si alguna tecla fue presionada
  char tecla_presionada = teclado1.getKey();
  //seleccion(tecla_presionada);
  if (tecla_presionada)
    {
      nVoltage = readBatLevel(); //Actualiza el nivel de bateria
      switch (tecla_presionada) {
        case '1':
          Serial.println("Caso - Ejemplo:");
          seleccion2a();
          break;
        case '2':
          Serial.println("Caso - Path Planning:");
          seleccion3a();
          break;
        default:
          // statements
          Serial.println("Escoja una opcion entre 1-2: ");
          String cadTexto = "Escoja una opcion: [1-2]";
          impTexto(6, 10, cadTexto);
          //Serial.println(tecla_presionada);
      }
    }
}


void menu0(){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  // Dibujar el texto en la pantalla
  display.drawString(25, 0, "T. de vuelo");
  display.drawString(0, 22, "1. Ejemplo");
  display.drawString(0, 44, "2. Path Planning");
  display.setFont(ArialMT_Plain_10);
  //int v = readBatLevel();
  display.drawString(114, 0, String(nVoltage));
  display.display();
}

void menu3a(){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  // Dibujar el texto en la pantalla
  display.drawString(0, 0, "1. Instrucciones");
  display.drawString(0, 20, "2. Revisar ruta");
  display.drawString(0, 40, "3. Volver");
  display.drawString(90, 40, "5. TX");
  display.setFont(ArialMT_Plain_10);
  display.drawString(115, 0, String(nVoltage));
  display.display();
}

void seleccion3a(){
  String instruccion = "TAKEOFF LAND";
  String instruccion2 = "";
  boolean band = 0;
  while (band == 0){
    menu3a();
    char op2 = teclado1.getKey();
    if (op2){
      if (op2 == '1'){
        Serial.println("seleccion3---Intrucciones");
        instruccion = seleccion3(); //Funcion usada para leer las intrucciones
        instruccion2 = eliminarTakeoffYLand(instruccion);
        Serial.println(instruccion);
      }else if (op2 == '2'){
        Serial.println("seleccion3---ver instrucciones");
        Serial.println(instruccion);
        imprimirRutaEnColumnas(instruccion);
      }else if (op2 == '3'){
        Serial.println("Saliendo");
        band = 1;
      }else if (op2 == '5'){
        // ############ OPCION PARA TRANSMITIR ############ //
        // ############     RUTA DINAMICA      ############ //

        // Aqui incluir funcion de envio
        //send("TAKEOFF RIGHT,150 FORWARD,150 LEFT,150 BACK,150 LAND");
        if ((instruccion == "TAKEOFF LAND") || (instruccion == "")){
          send(instruccion.c_str()); //---INTRUCCION PARA ENVIAR
        }else{
          send(instruccion2.c_str()); //---INTRUCCION PARA ENVIAR
        }
        Serial.println(instruccion);
        Serial.println(instruccion2);
        // Impresion por pantalla del envio
        Serial.println("TX");
        impTexto(6, 10, "Transmitiendo...");
        delay(1000);
        //instruccion = "TAKEOFF LAND"; //inicializa la ruta
      }else{
        Serial.println(op2);
        impTexto(6, 10, "Escoja una opcion: [1-5]");
      }
    }
  }
}

void menu3(){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_10);
  // Dibujar el texto en la pantalla
  display.drawString(14, 0, "Instrucciones");
  display.drawString(90, 0, "7. Salir");
  // Dibujar el texto en la pantalla
  display.drawString(0, 20, "1. Arriba");
  display.drawString(0, 35, "2. Abajo");
  display.drawString(0, 50, "3. Izquierda");
  display.drawString(64, 20, "4. Derecha");
  display.drawString(64, 35, "5. Frente");
  display.drawString(64, 50, "6. Atras");
  display.display();
}

String seleccion3(){
  int band = 0;
  int aux = 0;
  String auxText = "";
  String instruccion = "TAKEOFF ";
  while (band == 0){
    aux = 0;
    auxText = "";
    menu3();
    char op2 = teclado1.getKey();
    if (op2){
      if (op2 == '1'){
        aux = ingresarValor(40, 0, "Arriba");
        auxText = "UP," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '2'){
        aux = ingresarValor(40, 0, "Abajo");
        auxText = "DOWN," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '3'){
        aux = ingresarValor(40, 0, "Izqda");
        auxText += "LEFT," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '4'){
        aux = ingresarValor(40, 0, "Derecha");
        auxText += "RIGHT," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '5'){
        aux = ingresarValor(40, 0, "Frente");
        auxText += "FORWARD," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '6'){
        aux = ingresarValor(40, 0, "Atras");
        auxText += "BACK," + String(aux) + " ";
        instruccion += auxText;
        Serial.println(instruccion);
      }else if (op2 == '7'){
        Serial.println("Saliendo");
        band = 1;
      }else{
        Serial.println(op2);
        //String cadTexto = "Escoja una opcion: [1-3]";
        impTexto(6, 10, "Escoja una opcion: [1-7]");
      }
    }
  }
  instruccion += "LAND";
  return instruccion;
}

void menu2a(){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  // Dibujar el texto en la pantalla
  display.drawString(0, 0, "1. Revisar ruta");
  display.drawString(0, 20, "2. TX");
  display.drawString(0, 40, "3. Volver");
  display.setFont(ArialMT_Plain_10);
  display.drawString(114, 0, String(nVoltage));
  display.display();
}

void seleccion2a(){
  int band = 0;
    // Ejemplo de ruta:
    /*TAKEOFF FORWARD,80 RIGHT,50 LEFT,40 UP,90 DOWN,90 BACK,100 LAND*/
    String instruccion = "TAKEOFF LAND";
    String instruccion2 = ""; //Ruta programada
  while (band == 0){
    menu2a();
    char op2 = teclado1.getKey();
    //seleccion(tecla_presionada);
    if (op2){
      if (op2 == '1'){
        imprimirRutaEnColumnas(instruccion);
      }else if (op2 == '2'){
        // ############ OPCION PARA TRANSMITIR ############ //
        // ############     RUTA Estatica      ############ //
        // Aqui incluir funcion de envio
        if ((instruccion == "TAKEOFF LAND") || (instruccion == "")){
          send(instruccion.c_str()); //---INTRUCCION PARA ENVIAR
        }else{
          instruccion2 = eliminarTakeoffYLand(instruccion);
          send(instruccion2.c_str()); //---INTRUCCION PARA ENVIAR
        }
        Serial.println(instruccion);
        Serial.println(instruccion2);

        // Impresion por pantalla del envio
        Serial.println("TX");
        impTexto(6, 10, "Transmitiendo...");
        delay(1000);
      }else if (op2 == '3'){
        Serial.println("Saliendo");
        band = 1;
      }else{
        Serial.println(op2);
        //String cadTexto = "Escoja una opcion: [1-3]";
        impTexto(6, 10, "Escoja una opcion: [1-3]");
      }
    }
  }
}


// Funciones para imprimir texto por pantalla

void impTexto(int posX, int posY, String cadenaTexto){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_10);
  // Dibujar el texto en la pantalla
  display.drawString(posX, posY, cadenaTexto);
  // Mostrar el contenido en la pantalla
  display.display();
  delay(800);
}

void impRetroalimentacion16(int posX, int posY, String cadenaTexto){
  display.clear();
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  display.drawString(10, 0, "V. Ingresado");
  // Dibujar el texto en la pantalla
  display.drawString(posX, posY, cadenaTexto);
  // Mostrar el contenido en la pantalla
  display.display();
  delay(2000);
}

int ingresarValor(int posX, int posY, String cadenaTexto){
  boolean sal1 = 0;
  boolean band = 0;
  int valor = 0;
  String valTexto = "";
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 50, "*: Corregir");
  display.drawString(64, 50, "#: Enter");
  // Configurar el tamanio de la fuente
  display.setFont(ArialMT_Plain_16);
  // Dibujar el texto en la pantalla
  display.drawString(posX, posY, cadenaTexto);
  display.drawString(0, 25, "Dist. [cm]: ");
  // Mostrar el contenido en la pantalla
  display.display();

  while (sal1 == 0){
    char tecla1 = teclado1.getKey();
    if ((valor > 400) || (band == 1)) {
      valTexto = "";
      valor = 0;
      tecla1 = '*'; // Simula la pulsación de la tecla '*' cuando el valor es mayor a 400
    }
    if (tecla1){
      band = 0;
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 50, "*: Corregir");
      display.drawString(64, 50, "#: Enter");
      // Configurar el tamanio de la fuente
      display.setFont(ArialMT_Plain_16);
      // Dibujar el texto en la pantalla
      display.drawString(posX, posY, cadenaTexto);
      display.drawString(0, 25, "Dist. [cm]: ");
      // Mostrar el contenido en la pantalla
      //display.display();
      if (tecla1 == '#'){
        if (valor < 20){
          delay(600);
          display.clear();
          // Configurar el tamanio de la fuente
          display.setFont(ArialMT_Plain_16);
          // Dibujar el texto en la pantalla
          display.drawString(0, 0, "El valor ingresado");
          display.drawString(0, 20, "debe ser mayor a");
          display.drawString(0, 40, "20 [cm]");
          // Mostrar el contenido en la pantalla
          display.display();
          delay(1000);
          valTexto = "";
          band = 1;
          //valor = 1000; // Resetear el valor
        }else{
          sal1 = 1;
          String retro = cadenaTexto + ": " + valor + " cm";
          impRetroalimentacion16(10, 24, retro);
        }
      }else if (tecla1 == '*'){
        valTexto = "";
        valor = 0; // Resetear el valor
      }else{
        valTexto.concat(tecla1);
        valor = valTexto.toInt();
      }
      display.drawString(80, 25, valTexto);
      display.display();
      if (valor > 400){
        delay(600);
        display.clear();
        // Configurar el tamanio de la fuente
        display.setFont(ArialMT_Plain_16);
        // Dibujar el texto en la pantalla
        display.drawString(0, 0, "El valor ingresado");
        display.drawString(0, 20, "debe ser menor a");
        display.drawString(0, 40, "400 [cm]");
        // Mostrar el contenido en la pantalla
        display.display();
        delay(1000);
      }
    }
  }
  return valor;
}

// Definición de la función
String verificarYConcatenar(int arriba, int abajo, int izq, int der, int fret, int atras) {
    String resultados = "TAKEOFF ";

    if (arriba != 0) {
        resultados += "UP," + String(arriba) + " ";
    }
    if (abajo != 0) {
        resultados += "DOWN," + String(abajo) + " ";
    }
    if (izq != 0) {
        resultados += "LEFT," + String(izq) + " ";
    }
    if (der != 0) {
        resultados += "RIGHT," + String(der) + " ";
    }
    if (fret != 0) {
        resultados += "FORWARD," + String(fret) + " ";
    }
    if (atras != 0) {
        resultados += "BACK," + String(atras) + " ";
    }
    resultados += "LAND";
    return resultados;
}

void impRuta(String cadenaTexto, int posX, int posY) {
    //display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(posX, posY, cadenaTexto);
    //display.display();
    //delay(2000);
}

void imprimirRutaEnColumnas(String cadenaTexto) {
    display.clear();
    // Dividir el texto en palabras
    String palabras[10]; // Ajusta el tamanio si es necesario
    int palabraIndex = 0;

    int inicio = 0;
    for (int i = 0; i < cadenaTexto.length(); i++) {
        if (cadenaTexto[i] == ' ') {
            palabras[palabraIndex] = cadenaTexto.substring(inicio, i);
            palabraIndex++;
            inicio = i + 1;
        }
    }
    // Agregar la ultima palabra
    palabras[palabraIndex] = cadenaTexto.substring(inicio);
    palabraIndex++;

    // Determinar el punto de division para las dos columnas
    int mitad = (palabraIndex + 1) / 2;

    // Imprimir la primera columna
    for (int i = 0; i < mitad; i++) {
        impRuta(palabras[i], 0, i * 12); // Ajusta la posición según sea necesario
    }

    // Imprimir la segunda columna
    for (int i = mitad; i < palabraIndex; i++) {
        impRuta(palabras[i], 64, (i - mitad) * 12); // Ajusta la posición según sea necesario
    }

    display.display();
    delay(3000);
}

/**@brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void)
{
  Serial.println("OnTxDone");
  // delay(5000);
  // send();
}

/**@brief Function to be executed on Radio Tx Timeout event
 */
void OnTxTimeout(void)
{
  Serial.println("OnTxTimeout");
}

int contarInstrucciones(String str) {
  int count = 0;
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == ' ') {
      count++;
    }
  }
  return (count + 1); // Dado que cada instrucción tiene un parámetro
}

uint8_t mapearInstruccion(String instruccion) {
  if (instruccion == "RIGHT") return 0b000001;
  if (instruccion == "LEFT") return 0b000010;
  if (instruccion == "UP") return 0b000011;
  if (instruccion == "DOWN") return 0b000100;
  if (instruccion == "FORWARD") return 0b000101;
  if (instruccion == "BACK") return 0b000110;
  return 0; // En caso de instrucción no reconocida
}

void parsearInstrucciones(String str, uint8_t *trama) {
  int lastIndex = 0;
  int indice_trama=1;

  while (lastIndex < str.length()) {
    // Encontrar el índice de la coma y el espacio más cercanos
    int commaIndex = str.indexOf(',', lastIndex);
    int spaceIndex = str.indexOf(' ', commaIndex);

    // Extraer la instrucción y el parámetro
    String instruccionStr = str.substring(lastIndex, commaIndex);
    String parametroStr = str.substring(commaIndex + 1, (spaceIndex == -1) ? str.length() : spaceIndex);

    int instruccion = mapearInstruccion(instruccionStr);
    int parametro  = parametroStr.toInt();

    // MASCARAS
    parametro  &= 0x1FF; // 0x1FF es 511 en decimal, que tiene 9 bits en binario

    uint16_t comando = ((uint16_t)instruccion << 12) | (parametro & 0x1FF);
    // Separar en 2 bytes
    uint8_t highByte = (comando >> 8) & 0xFF; // Byte más significativo (High Byte)
    uint8_t lowByte = comando & 0xFF;         // Byte menos significativo (Low Byte)
    
    trama[indice_trama]=highByte;
    trama[indice_trama+1]=lowByte;
    
    // Mostrar la instrucción y el valor actual
    Serial.print("Instrucción: ");
    Serial.print(instruccionStr);
    Serial.print("  -->  ");
    uint8_t instruccionComando = (comando >> 12) & 0xF;
    Serial.print(instruccionComando);

    
    Serial.print(", Parámetro: ");
    Serial.print(parametroStr);
    Serial.print("  -->  ");
    uint16_t parametroComando = comando & 0x1FF;
    Serial.print(parametroComando);
    Serial.print("\n");


    // Actualizar lastIndex para la siguiente iteración
    indice_trama+=2;
    lastIndex = (spaceIndex == -1) ? str.length() : spaceIndex + 1;
  }
}

void send(const char *message)
{
  String inputString = message;

    // Verificar si el mensaje es exactamente "TAKEOFF LAND"
    if (inputString == "TAKEOFF LAND") {
        // Crear una trama de tamaño 1 con todos los bits en 1
        uint8_t trama[1] = {0xFF};
        
        Serial.println("Enviando trama especial de TAKEOFF LAND");
        Serial.print("Trama: ");
        Serial.println(trama[0], BIN); // Imprimir la trama en formato binario
        
        // Enviar la trama
        Radio.Send(trama, 1);
    } else {
        // Proceder con el comportamiento normal
        int N_instrucciones = contarInstrucciones(inputString);  // Cantidad de instrucciones totales
        int trama_size = 2 * N_instrucciones + 1;                // Tamaño de la trama total
        uint8_t *trama = new uint8_t[trama_size];                // Seteo de tamaño de la trama
        trama[0] = N_instrucciones;                              // El primer byte es la cantidad total de instrucciones
        
        Serial.print("Cantidad de instrucciones: ");
        Serial.println(N_instrucciones);
        Serial.print("Tamaño de la trama: ");
        Serial.println(trama_size);
        
        parsearInstrucciones(inputString, trama);
        Radio.Send(trama, trama_size);
        
        // Liberar la memoria asignada dinámicamente
        delete[] trama;
    }
}

int readBatLevel(){
  int analogValue = analogRead(VBAT_PIN);
  float voltage = 0.097751711 * analogValue;  // Calcular el voltaje
  int roundedVoltage = round(voltage);  // Redondear el voltaje al entero más cercano
  return roundedVoltage;
}

// Funcion para eliminar "TAKEOFF " y " LAND" de una cadena
String eliminarTakeoffYLand(String cadena) {
  // Eliminar "TAKEOFF "
  if (cadena.startsWith("TAKEOFF ")) {
    cadena = cadena.substring(8);  // "TAKEOFF " tiene 8 caracteres
  }
  
  // Eliminar " LAND"
  if (cadena.endsWith(" LAND")) {
    cadena = cadena.substring(0, cadena.length() - 5);  // " LAND" tiene 5 caracteres
  }
  
  return cadena;
}