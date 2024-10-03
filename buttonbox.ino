//BUTTON BOX 
//USE w ProMicro
//Tested in WIN11 + Assetto Corsa Competizione
//

#include <Keypad.h>
#include <Joystick.h>

#define ENABLE_PULLUPS
#define NUMROTARIES 4
#define NUMBUTTONS 12
#define NUMROWS 3
#define NUMCOLS 4
#define NUMSWITCHES 3
#define SINGLE_BUTTONS 1

const unsigned long debounceDelay = 50;
unsigned long lastDebounceTime[SINGLE_BUTTONS] = {0};
unsigned long lastDebounceTimeSW[NUMSWITCHES] = {0,0,0};
unsigned long resetSWTimer[NUMSWITCHES] = {0,0,0,};
const unsigned long resetDelay = 200;

byte switches[NUMSWITCHES] = {12,13,14};
byte switchesPin[NUMSWITCHES] = {0,2,3};
byte switchesState[NUMSWITCHES] = {HIGH,HIGH,HIGH};
bool justPressed[NUMSWITCHES] = {false,false,false};

byte single_buttons[SINGLE_BUTTONS] = {15};
byte singleButtonsPin[SINGLE_BUTTONS] = {4};
byte singleButtonsState[SINGLE_BUTTONS] = {HIGH};


byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3},
  {4,5,6,7},
  {8,9,10,11}
};

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;
  int cwchar;
  volatile unsigned char state;
};

rotariesdef rotaries[NUMROTARIES] {
  {12,13,16,17,0},
  {A0,A1,18,19,0},
  {A2,A3,20,21,0},
  {A4,A5,22,23,0},
};

#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#ifdef HALF_STEP
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

byte rowPins[NUMROWS] = {5,6,7}; 
byte colPins[NUMCOLS] = {8,9,10,11}; 

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS); 

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 32, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

void setup() {
  Serial.begin(9600);
  Joystick.begin();
  rotary_init();
  SetupSwitches();
  }

void loop() { 

  CheckAllEncoders();

  CheckAllButtons();

  CheckAllSwitches();

  CheckAllSingleButtons();

}

void SetupSwitches(){
    
  // Configuramos primero los switches
    for (int i = 0; i<NUMSWITCHES; i++ ){ 
      pinMode(switchesPin[i],INPUT_PULLUP);
    }
  // Configuramos los botones independientes
    for (int v =0; v<SINGLE_BUTTONS; v++){
      pinMode(singleButtonsPin[v],INPUT_PULLUP);
    }
    Serial.println("Setup completado");
}

void CheckAllButtons(void) {
      if (buttbx.getKeys())
    {
       for (int i=0; i<LIST_MAX; i++)   
        {
           if ( buttbx.key[i].stateChanged )   
            {
            switch (buttbx.key[i].kstate) {  
                    case PRESSED:
                    case HOLD:
                              Joystick.setButton(buttbx.key[i].kchar, 1);
                              break;
                    case RELEASED:
                    case IDLE:
                              Joystick.setButton(buttbx.key[i].kchar, 0);
                              break;
            }
           }   
         }
     }
}

void CheckAllSingleButtons() {
  for (int i = 0; i < SINGLE_BUTTONS; i++) {
    // Si hay un cambio en el estado del botón
    if (digitalRead(singleButtonsPin[i]) != singleButtonsState[i]) {
      // Reiniciamos el tiempo de debounce
      lastDebounceTime[i] = millis();
      // Actualizamos el estado del botón
      singleButtonsState[i] = digitalRead(singleButtonsPin[i]);
      
    }

    // Si ha pasado el tiempo de debounce y el botón está pulsado
    if (millis() > lastDebounceTime[i] + debounceDelay && singleButtonsState[i] == LOW) {
      Joystick.setButton(single_buttons[i], 1);
    }
    // Si el botón está liberado (estado alto)
    else if (singleButtonsState[i] == HIGH) {
      Joystick.setButton(single_buttons[i], 0);
    }
  }
}


void rotary_init() {
  for (int i=0;i<NUMROTARIES;i++) {
    pinMode(rotaries[i].pin1, INPUT);
    pinMode(rotaries[i].pin2, INPUT);
    #ifdef ENABLE_PULLUPS
      digitalWrite(rotaries[i].pin1, HIGH);
      digitalWrite(rotaries[i].pin2, HIGH);
    #endif
  }
}


unsigned char rotary_process(int _i) {
   unsigned char pinstate = (digitalRead(rotaries[_i].pin2) << 1) | digitalRead(rotaries[_i].pin1);
  rotaries[_i].state = ttable[rotaries[_i].state & 0xf][pinstate];
  return (rotaries[_i].state & 0x30);
}

void CheckAllEncoders(void) {
  for (int i=0;i<NUMROTARIES;i++) {
    unsigned char result = rotary_process(i);
    if (result == DIR_CCW) {
      Joystick.setButton(rotaries[i].ccwchar, 1); delay(50); Joystick.setButton(rotaries[i].ccwchar, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(rotaries[i].cwchar, 1); delay(50); Joystick.setButton(rotaries[i].cwchar, 0);
    };
  }
}

void CheckAllSwitches() {
  for (int i = 0; i < NUMSWITCHES; i++) {
    // Si hay un cambio en el estado del botón
    if (digitalRead(switchesPin[i]) != switchesState[i]) {
      if(millis() < resetSWTimer[i] + resetDelay){
        switchesState[i] = digitalRead(switchesPin[i]);
      }else{
      // Reiniciamos el tiempo de debounce
      lastDebounceTimeSW[i] = millis();
      // Actualizamos el estado del botón
      switchesState[i] = digitalRead(switchesPin[i]);
      justPressed[i] = true;
      }
    }

    // Si ha pasado el tiempo de debounce y el botón está pulsado
    if (millis() > lastDebounceTimeSW[i] + debounceDelay && justPressed[i]==true) {
      if(switchesState[i] == LOW){
      Joystick.setButton(switches[i], 1);
      delay(75);
      Joystick.setButton(switches[i], 0);
      justPressed[i] = false;;
      resetSWTimer[i] = millis();
    }
    // Si el botón está liberado (estado alto)
    else {
      Joystick.setButton(switches[i], 1);
      delay(75);
      Joystick.setButton(switches[i], 0);
      justPressed[i] = false;
      resetSWTimer[i] = millis();
    }
  }
}
}

