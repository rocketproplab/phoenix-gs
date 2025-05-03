// Using binary literal for rocket state
#include <system_error>
const uint8_t PRE_ARM = 0b000000; // 0  decimal
const uint8_t ABORT = 0b010101;   // 21 decimal
const uint8_t ARMED = 0b100000;   // 32 decimal
const uint8_t LAUNCH = 0b101010;  // 42 decimal

uint8_t launchModeState = 0b000000;

// Current state of the rocket
uint8_t rocketState = 0b000000;

// Operation modes
enum MODE
{
  NONE_MODE,
  LAUNCH_MODE,
  FUELING_MODE,
  DEV_MODE
};

enum LAUNCH_MODE_ENUM
{
  PREARM_BTN,
  ABORT_BTN,
  ARM_ON,
  ARM_OFF,
  LAUNCH_BTN
};

const int PIN_GN2_F = 1;
const int PIN_LNG_F = 2;
const int PIN_LOX_F = 3;
const int PIN_GN2_V = 4;
const int PIN_LNG_V = 5;
const int PIN_LOX_V = 6;
const int PIN_ARM = 7;
const int PIN_ABORT = 8;
const int PIN_LAUNCH = 9;
const int PIN_LAUNCH_M = 10;
const int PIN_FUELING_M = 11;
const int PIN_DEV_M = 12;

// —— GLOBAL STATE ——
// Raw + debounced states:
struct DebouncedInput {
  unsigned int pin;
  unsigned int currState;
  unsigned int lastState;
  unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
};

DebouncedInput gn2Flow = { PIN_ARM_SWITCH, LOW, LOW, 0 };
DebouncedInput lngFlow = {PIN_START_BUTTON, LOW, LOW, 0};
DebouncedInput loxFlow = {PIN_LAUNCH_BUTTON, LOW, LOW, 0};
DebouncedInput gn2Vault = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput lngVault = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput loxVault = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput arm = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput abort = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput launch = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput launch_M = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput fueling_M = {PIN_ABORT_BUTTON, LOW, LOW, 0};
DebouncedInput dev_M = {PIN_ABORT_BUTTON, LOW, LOW, 0};

// the debounce time; increase if the output flickers
unsigned long debounceDelay = 50;

// TODO: this function updates the DebouncedInput struct when called
//       this reads a button
void debounceButtonRead(DebouncedInput *input)
{
  unsigned int pin = input->pin;
  unsigned int currState = input->currState;
  unsigned int lastState = input->lastState;
  unsigned long lastDebounceTime = input->lastDebounceTime;

  int reading = digitalRead(pin);
  

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastState)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != lastState)
    {
      currState = reading;

      // only toggle if the new button state is HIGH
      if (currState == HIGH)
      {
        MachineState = controlButton;
      }
    }
  }

  // set the LED:
  digitalWrite(ledPin, ledState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}


// TODO: this function updates the DebouncedInput struct when called
//       this reads a switch
void debounceSwitchRead(DebouncedInput *input)
{

}


MODE getModePress()
{
  // TODO: Replace with real pin reading logic or test code
  // only one button press should be registered at one time
  // if multiple button reads are registered, must have set priority.

  debounceSwitchRead(&launch_M);
  debounceSwitchRead(&fueling_M);
  debounceSwitchRead(&dev_M);

  unsigned int launch_button = ;



  return NONE_MODE;
}

LAUNCH_MODE_ENUM getLaunchModePress()
{
  // TODO: Replace with real pin reading logic or test code
  // only one button press should be registered at one time
  // if multiple button reads are registered, must have set priority.
  return PREARM_BTN;
}

void launch_mode_logic(){
  LAUNCH_MODE_ENUM launchModePress = getLaunchModePress();

  // State machine for rocket
  switch (rocketState)
  {
  case PRE_ARM:
    if (launchModePress == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    else if (launchModePress == ARM_ON)
    {
      rocketState = ARMED;
    }
    break;

  case ARMED:
    if (launchModePress == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    else if (launchModePress == LAUNCH_BTN)
    {
      rocketState = LAUNCH;
    }
    else if (launchModePress == ARM_OFF)
    {
      rocketState = PRE_ARM;
    }
    break;

  case LAUNCH:
    // rocket is launching
    // in static fire: abort is possible
    // in launch: abort is not possible
    if (launchModePress == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    break;

  case ABORT:
    // if in abort cannot get out
    break;
  }
}


void fueling_mode_logic(){


}


void dev_mode_logic(){


}

void setup() {
  MODE operationMode = NONE_MODE;
  LAUNCH_MODE_ENUM launchState = PREARM_BTN;

  // initialize all inputs:
  pinMode(PIN_GN2_F, INPUT);
  pinMode(PIN_LNG_F, INPUT);
  pinMode(PIN_LOX_F, INPUT);
  pinMode(PIN_GN2_V, INPUT);
  pinMode(PIN_LNG_V, INPUT);
  pinMode(PIN_LOX_V, INPUT);
  pinMode(PIN_ARM, INPUT);
  pinMode(PIN_ABORT, INPUT);
  pinMode(PIN_LAUNCH, INPUT);
  pinMode(PIN_LAUNCH_M, INPUT);
  pinMode(PIN_FUELING_M, INPUT);
  pinMode(PIN_DEV_M, INPUT);
}

void loop() {
  MODE modePress = getModePress();
  switch (modePress)
  {
  case LAUNCH_MODE:
    launch_mode_logic();
    break;

  case FUELING_MODE:
    fueling_mode_logic();
    break;

  case DEV_MODE:
    dev_mode_logic();
    break;
  
  default:
    break;
  }


  // TODO: based on current state modify valves
  setValves(rocketState); // TODO: implement
  delay(50);              // small debounce or loop delay
}
