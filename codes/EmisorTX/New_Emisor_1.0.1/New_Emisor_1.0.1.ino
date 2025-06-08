#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "img.h"
#include "HT_SSD1306Wire.h"
#include <Keypad.h>
#include "arduino.h"
#include <SPI.h>

// ********************************************** Configuration Display ********************************************** 
SSD1306Wire mydisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);


// ********************************************** Parameters Read Battery **********************************************
#define VBAT_PIN 1
#define VBAT_READ_CNTRL_PIN 37 // Heltec GPIO to toggle VBatt read connection …
// Also, take care NOT to have ADC read connection
// in OPEN DRAIN when GPIO goes HIGH
#define ADC_READ_STABILIZE 10 // in ms (delay from GPIO control and ADC connections times)

// ********************************************** Parameters Read LoRa **********************************************
#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             22        // dBm

#define LORA_BANDWIDTH                              1         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            3000
#define TX_TIMEOUT_VALUE                            3000
#define BUFFER_SIZE                                 64 // Define the payload size here
static RadioEvents_t RadioEvents;

// ********************************************** Configuration KeyPad  ********************************************** 
#define ROWS                            4
#define COLUMNS                         3

//const byte filas = 4; 
//const byte columnas = 3;
//byte pinesFilas[]  = {45,38,39,41};
//byte pinesColumnas[] = {42,46,40};
byte pinesFilas[]    = {41,7,5,1};
byte pinesColumnas[] =	{39,45,3};
char teclas[4][3] = {{'1','2','3'},
                     {'4','5','6'},
                     {'7','8','9'},
                     {'*','0','#'}};
Keypad teclado1 = Keypad( makeKeymap(teclas), pinesFilas, pinesColumnas, ROWS, COLUMNS);


// ********************************************** Global Variables and Structures **********************************************
int nVoltage = 0;
int velocity = 50;
int distance_total = 0;
struct text_display {
  int x;                //Pos X of Text
  int y;                //Pos Y of Text
  const char *text;     //Text
  const uint8_t *font;  //Font of Text
};

struct inst_drone {
  int  d;                //Distance
  const char *ints;             //Instrucction
};


// ********************************************** Declaration of Functions ********************************************** 
// It reads the batery level in the device
int readBatLevel(void);

// It draw any amounts of lines (Text) in the display. 
void drawText(text_display *lines, int number_lines,bool clc, bool disp);

// This function makes the procesing about the option Example (This means that emitter sends a path programed)
void selection2a(void);

// This function prints the path programed in the display
void printPathsColumns(inst_drone *instructions, int number_ints);

// This funtion sends the drone instrucctions
void send(inst_drone *instructions, int number_ints, bool test);

// This funtion creates a payload in base of design
void createPayload(inst_drone *instructions, uint8_t *trama, int number_ints, int start);

// This funtion maps a char to uint8_t in base of design payload
uint8_t mapInstruction(char *instruction);

// This funtions returns a integer given by user. 
int showWriting(int posX, int posY,int lim_Inf, int lim_Sup, char *titule);

// This funtion calculate the distance total in the RX
void totalDistance(inst_drone *instructions,int number_ints);
// LORA Functions
void OnTxDone( void );
void OnTxTimeout( void );

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
	
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

    mydisplay.init();
    // Limpiar la pantalla
    text_display text_0[]={
      {20, 0, "U. CUENCA",ArialMT_Plain_16},
      {0, 20, "J. Cambisaca",ArialMT_Plain_10},
      {0, 35, "H. Castro",ArialMT_Plain_10},
      {0, 50, "H. Maldonado",ArialMT_Plain_10}
    };
    drawText(text_0, 4,true,false);
    mydisplay.drawXbm(80, 30, 40, 40, logos[0]);
    mydisplay.display();

    nVoltage = readBatLevel();

    delay(3000);
   }


void loop() {
  // Handle Radio events
  Radio.IrqProcess();
  delay(100);// We are on FreeRTOS, give other tasks a chance to run
  yield();  
  
  ////////////////////////// PRINCIPAL MENU //////////////////////////
  char level_bat_char[4];
  itoa(nVoltage, level_bat_char, 10);
  text_display menu_0[]={
      {0, 0, "FLIGHT TYPE",ArialMT_Plain_16},
      {0, 22, " 1. Example",ArialMT_Plain_16},
      {0, 44, " 2. Path Planning",ArialMT_Plain_16},
      {114,0, level_bat_char,ArialMT_Plain_10}
    };
  drawText(menu_0, 4,true,true);
  ////////////////////////////////////////////////////////////////////

  ////////////////////// ACTION OF THE OPTIONS ///////////////////////
  char tecla_presionada = teclado1.getKey();
  if (tecla_presionada){
    velocity = 100;
    nVoltage = readBatLevel(); //Actualiza el nivel de bateria
    switch (tecla_presionada) {
      case '1':
        Serial.println("Caso - Ejemplo:");
        selection2a();
        break;
      case '2':
        selection3a();
        break;
      /*
      case '2':
        Serial.println("Caso - Path Planning:");
        seleccion3a();
        break;*/
      default:
        // Warning to user
        menu_0[0].y = 10;
        menu_0[1].y = 20;

        menu_0[0].text = "You MUST choose one";
        menu_0[1].text = "option between [1-3]!!";

        menu_0[0].font = ArialMT_Plain_10;
        menu_0[1].font = ArialMT_Plain_10;
        drawText(menu_0, 2,true,true);
        delay(1000);
    }
  }

}

void OnTxDone(void)
{
  Serial.println("OnTxDone");
  // delay(5000);
  // send();
}

void OnTxTimeout(void)
{
  Serial.println("OnTxTimeout");
}

int readBatLevel(){
  int analogValue = analogRead(VBAT_PIN);
  float voltage = 0.097751711 * analogValue;  // Calcular el voltaje
  int roundedVoltage = round(voltage);  // Redondear el voltaje al entero más cercano
  return roundedVoltage;
}

void drawText(text_display *lines, int number_lines, bool clc, bool disp){
  if (clc) mydisplay.clear();
  for (int i = 0; i < number_lines; i++) {
        mydisplay.setFont(lines[i].font);  // Apply the font
        mydisplay.drawString(lines[i].x, lines[i].y, lines[i].text);
    }
  if (disp) mydisplay.display();
}

void selection2a (){
  bool band = false;
  // Path Example:
  //inst_drone instruction_drone[] = {{350,"Up"},{400,"Ahead"},{200,"Left"},{20,"Up"},{212,"Down"},{212,"Back"},{212,"Right"},{212,"Right"}};
  //inst_drone instruction_drone[] = {{400,"Ahead"},{400,"Back"}};
  inst_drone instruction_drone[] = {{400,"Ahead"},{400,"Ahead"},{400,"Ahead"},{400,"Ahead"},{400,"Ahead"},{400,"Ahead"}};
  //inst_drone instruction_drone[] = {{20,"Up"}};
  int number_paths = sizeof(instruction_drone) / sizeof(instruction_drone[0]);               
  totalDistance(instruction_drone,number_paths);
  while (band == false){
    ///// It shows the options in the display //////////////
    char level_bat_char[4];
    itoa(nVoltage, level_bat_char, 10);
    text_display menu_1[]={
      {0, 0, "1.Route Check",ArialMT_Plain_16},
      {0, 20,"2.TX",ArialMT_Plain_16},
      {50,20,"3.LoRA Ev",ArialMT_Plain_16},
      {0, 40,"4.Vel",ArialMT_Plain_16},
      {50,40,"5.Back",ArialMT_Plain_16},
      {114,0, level_bat_char,ArialMT_Plain_10}
    };
    drawText(menu_1, 6,true,true);
    ///////////////////////////////////////////////////////

    char op2 = teclado1.getKey();
    
    if (op2){
      switch (op2) {
        case '1':
          printPathsColumns(instruction_drone, number_paths);
          break;
        case '2':
          menu_1[0].text = "Sending...";
          drawText(menu_1, 1,true,true);
          send(instruction_drone, number_paths,false);
          delay(3000);
          break;
        case '3':
          menu_1[0].text = "Testing LORA...";
          drawText(menu_1, 1,true,true);
          //send(instruction_drone, number_paths,false);
          //delay(3000);
          send(instruction_drone, number_paths,true);
          break;
        case '4':
          velocity = showWriting(0, 25,10, 100,"Velocity [cm/s]");
          break;
        case '5':
          band=true;
          break;
        default:
          menu_1[0].y = 10;
          menu_1[1].y = 20;

          menu_1[0].text = "You MUST choose one";
          menu_1[1].text = "option between [1-3]!!";

          menu_1[0].font = ArialMT_Plain_10;
          menu_1[1].font = ArialMT_Plain_10;
          
          drawText(menu_1, 2,true,true);
          delay(1000);
      }
    }
  }
}
void printPathsColumns(inst_drone *instructions, int number_ints) {
  char instructions_buffer[number_ints+1][15];
  text_display textDisplay[number_ints+2];

  textDisplay[0].x = 0;
  textDisplay[0].y = 0;
  textDisplay[0].text = "1 TAKEOFF";
  textDisplay[0].font = ArialMT_Plain_10;

  for (int i = 1; i <= number_ints + 1; i++) {
    if (i != number_ints + 1) {
      sprintf(instructions_buffer[i - 1], "%d %s-%d", i + 1, instructions[i-1].ints, instructions[i-1].d);
    } else {
      sprintf(instructions_buffer[i - 1], "%d LAND", i + 1);
    }
    textDisplay[i].x = (i < 5) ? 0 : 64;
    textDisplay[i].y = (i < 5) ? i * 12 : (i % 5) * 12;
    textDisplay[i].text = instructions_buffer[i - 1];
    textDisplay[i].font = ArialMT_Plain_10;
  }

  drawText(textDisplay, number_ints + 2, true, true);
  delay(3000);
}
void send(inst_drone *instructions, int number_ints, bool test){
  if (test){
    int num_payloads = round(5*(((float)distance_total)/((float)velocity)+((float)number_ints*1.5)));
    char text_charge[1][32];
    //Serial.println(num_payloads);
    text_display menu_2[]={
      {0, 0, "Testing LORA...",ArialMT_Plain_16},
      {0, 20,"Any",ArialMT_Plain_16}
    };
    //for (int i=0; i<num_payloads;i++){
    for (int i=0; i<250;i++){
      int trama_size = number_ints+2;
      uint8_t *trama = new uint8_t[trama_size];          // Creation of payload
      trama[0] = (uint8_t) velocity;                     // The first byte is the number of drone instrucctions
      trama[0] = trama[0] | 0b10000000;
      trama[1] = (uint8_t) (i%256);
      //Serial.print("Tamaño de la trama: ");
      //Serial.println(trama_size);
      createPayload(instructions, trama, number_ints,2);
      Radio.Send(trama, trama_size);
      //Serial.println(trama[0],BIN);
      //Serial.println("TEST");
      //Serial.println(trama[1],BIN);
      delete[] trama;
      sprintf(text_charge[0], "%d/%d", i+1,250);
      menu_2[1].text=text_charge[0];
      drawText(menu_2, 2,true,true);
      delay(200);
    }
  }else{
    int trama_size = number_ints+1;
    uint8_t *trama = new uint8_t[trama_size];          // Creation of payload
    trama[0] = (uint8_t) velocity;                     // The first byte is the number of drone instrucctions
    trama[0] = trama[0];
    //Serial.print("Tamaño de la trama: ");
    //Serial.println(trama_size);
    createPayload(instructions, trama, number_ints,1);
    Radio.Send(trama, trama_size);
    //Serial.println("SEND INSTRUCTIONS");
    //Serial.println(trama[0],BIN);
    //Serial.println(trama[1],BIN);
    // Free dynamically allocated memory
    delete[] trama;
  }
}
void createPayload(inst_drone *instructions, uint8_t *trama, int number_ints,int start) {
  int indice_trama=start;
  for (int i=0;i<number_ints;i++){
    int instruccion  = mapInstruction(instructions[i].ints);
    
    uint8_t subpayload1 = ((uint8_t) instruccion <<5)& 0xE0;
    uint8_t subpayload2 = ((uint8_t) map(instructions[i].d, 20, 500, 0, 31))& 0x1F;
    
    trama[indice_trama] = subpayload1 | subpayload2;
    indice_trama++;
    // Mostrar la instrucción y el valor actual
    /*
    Serial.print(instruccion);
    Serial.print("-");
    Serial.print(subpayload1);
    Serial.print("-Instrucción: ");
    Serial.print(instructions[i].ints);
    Serial.print("  -->  ");
    uint8_t instruccionComando = trama[indice_trama] >>5;
    Serial.print(instruccionComando);

    Serial.print(", Parámetro: ");
    Serial.print(instructions[i].d);
    Serial.print("  -->  ");
    uint8_t parametroComando = trama[indice_trama] & 0x1F;
    Serial.print(parametroComando);
    Serial.print("  -->  ");
    uint16_t result = ((uint16_t) map(parametroComando, 0,31, 20, 500));
    Serial.print(result);
    Serial.print("\n");*/
  }
}
uint8_t mapInstruction(const char *instruction) {
  if (strcmp(instruction, "Right") == 0) return 0b000001;
  if (strcmp(instruction, "Left")  == 0) return 0b000010;
  if (strcmp(instruction, "Up")    == 0) return 0b000011;
  if (strcmp(instruction, "Down")  == 0) return 0b000100;
  if (strcmp(instruction, "Ahead") == 0) return 0b000101;
  if (strcmp(instruction, "Back")  == 0) return 0b000110;
  return 0;
}

int showWriting(int posX, int posY, int lim_Inf, int lim_Sup, char *titule) {
  bool band = false;
  char val_char[4] = {0};       // Enough for 3 digits + null terminator
  char warning[32] = {0};       // Just a single buffer is enough
  int result = 0;
  uint8_t conta_char = 0;

  text_display text1[] = {
    {0, 0, "The value MUST be", ArialMT_Plain_10},
    {0, 15, "Any", ArialMT_Plain_10}
  };

  text_display value_display[] = {
    {0, 0, titule, ArialMT_Plain_16},
    {posX, posY, "Any", ArialMT_Plain_16},
    {0, 50, "* Delete    # Enter", ArialMT_Plain_10}
  };

  mydisplay.clear();

  while (!band) {
    char key1 = teclado1.getKey();
    if (key1) {
      switch (key1) {
        case '*': {
          memset(val_char, 0, sizeof(val_char));
          result = 0;
          conta_char = 0;
          mydisplay.clear();
          break;
        }
        case '#': {
          result = atoi(val_char);
          if (result > lim_Sup) {
            sprintf(warning, "LESS THAN %d", lim_Sup);
            text1[1].text = warning;
            drawText(text1, 2, true, true);

            memset(val_char, 0, sizeof(val_char));
            result = 0;
            conta_char = 0;
            delay(3000);
            mydisplay.clear();
          } else if (result < lim_Inf) {
            sprintf(warning, "MORE THAN %d", lim_Inf);
            text1[1].text = warning;
            drawText(text1, 2, true, true);

            memset(val_char, 0, sizeof(val_char));
            result = 0;
            conta_char = 0;
            delay(3000);
            mydisplay.clear();
          } else {
            band = true;
          }
          break;
        }
        default: {
          if (conta_char < 3 && isDigit(key1)) {
            val_char[conta_char++] = key1;
            val_char[conta_char] = '\0';  // ensure null-terminated
          } else {
            text1[1].text = "Only 3 digits!!";
            drawText(text1, 2, true, true);
            memset(val_char, 0, sizeof(val_char));
            conta_char = 0;
            delay(3000);
            mydisplay.clear();
          }
        }
      }
    }
    value_display[1].text = val_char;
    drawText(value_display, 3, false, true);
  }

  return result;
}

void totalDistance(inst_drone *instructions,int number_ints){
  distance_total = 0;
  int conversion = 0; 
  for (int i=0;i<number_ints;i++){
    conversion = round((31.0/480.0)*((float)instructions[i].d-20.0));
    distance_total = distance_total + round( ((float)conversion+31.0/24.0)*(480.0/31.0));
  }
}


void selection3a(){
  bool band = false;
  inst_drone instruction_drone[8];
  int number_paths =0;               
  while (band == false){
    ///// It shows the options in the display //////////////
    char level_bat_char[4];
    itoa(nVoltage, level_bat_char, 10);
    text_display menu_3[]={
      {0, 0, "1.Instructions",ArialMT_Plain_16},
      {0, 20,"2.Route Check",ArialMT_Plain_16},
      {0, 40,"3.TX 4.Vel 5.Back",ArialMT_Plain_16}
    };
    drawText(menu_3, 3,true,true);
    ///////////////////////////////////////////////////////

    char op2 = teclado1.getKey();
    
    if (op2){
      switch (op2) {
        case '1':
          selection3(instruction_drone,&number_paths);
          break;
        case '2':
          printPathsColumns(instruction_drone, number_paths);
          break;
        case '3':
          menu_3[0].text = "Sending...";
          drawText(menu_3, 1,true,true);
          send(instruction_drone, number_paths,false);
          delay(3000);
          break;
        case '4':
          velocity = showWriting(0, 25,10, 100,"Velocity [cm/s]");
          break;
        case '5':
          band=true;
          break;
        default:
          menu_3[0].y = 10;
          menu_3[1].y = 20;

          menu_3[0].text = "You MUST choose one";
          menu_3[1].text = "option between [1-3]!!";

          menu_3[0].font = ArialMT_Plain_10;
          menu_3[1].font = ArialMT_Plain_10;
          
          drawText(menu_3, 2,true,true);
          delay(1000);
      }
    }
  }
}

void selection3(inst_drone *instructions,int *number_paths){
  bool band = false; 
  int  conta =0;
  while(!band){
    char level_bat_char[4];
    itoa(nVoltage, level_bat_char, 10);
    text_display menu_4[]={
      {14, 0, "Instructions     7.Exit",ArialMT_Plain_10},
      {0, 20, "1.Up",ArialMT_Plain_10},
      {0, 35, "2.Down",ArialMT_Plain_10},
      {0, 50, "3.Left",ArialMT_Plain_10},
      {64,20, "4.Right",ArialMT_Plain_10},
      {64,35, "5.Ahead",ArialMT_Plain_10},
      {64,50, "6.Back",ArialMT_Plain_10}
    };
    drawText(menu_4, 7,true,true);
    char op2 = teclado1.getKey();
    if (op2){
      switch(op2){
        case '1':
          instructions[conta].ints = "Up";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Up [cm]");
          conta ++;
          break;
        case '2':
          instructions[conta].ints = "Down";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Down [cm]");
          conta ++;
          break;
        case '3':
          instructions[conta].ints = "Left";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Left [cm]");
          conta ++;
          break;
        case '4':
          instructions[conta].ints = "Right";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Right [cm]");
          conta ++;
          break;
        case '5':
          instructions[conta].ints = "Ahead";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Ahead [cm]");
          conta ++;
          break;
        case '6':
          instructions[conta].ints = "Back";
          instructions[conta].d    = showWriting(0, 25,20, 500,"Back [cm]");
          conta ++;
          break;
        case '7':
          band = true;
          break;
        default:
          menu_4[0].y = 10;
          menu_4[1].y = 20;

          menu_4[0].text = "You MUST choose one";
          menu_4[1].text = "option between [1-7]!!";

          menu_4[0].font = ArialMT_Plain_10;
          menu_4[1].font = ArialMT_Plain_10;
          
          drawText(menu_4, 2,true,true);
          delay(1000);
      }
      if (conta>=8){
        band = true;
        menu_4[0].x = 0;
        menu_4[0].y = 0;

        menu_4[0].text = "Maximun 8 Instructions !! ";

        menu_4[0].font = ArialMT_Plain_10;
        
        drawText(menu_4, 1,true,true);
        delay(1000);
      }
    }
  }
  *number_paths = conta;
}