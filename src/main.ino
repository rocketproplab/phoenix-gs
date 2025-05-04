// Using binary literal for rocket state
#include <system_error>

const uint8_t PRE_ARM = 0b000000; // 0  decimal
const uint8_t ABORT = 0b010101;   // 21 decimal
const uint8_t ARMED = 0b100000;   // 32 decimal
const uint8_t LAUNCH = 0b101010;  // 42 decimal

const uint8_t gn2_flow_mask = 0b100000;
const uint8_t gn2_vent_mask = 0b010000;
const uint8_t lng_flow_mask = 0b001000;
const uint8_t lng_vent_mask = 0b000100;
const uint8_t lox_flow_mask = 0b000010;
const uint8_t lox_vent_mask = 0b000001;

// Current state of the rocket
uint8_t rocketState;

// Operation modes
enum MODE
{
  NONE_MODE,
  LAUNCH_MODE,
  FUELING_MODE,
  DEV_MODE
};

MODE operationMode;

enum LAUNCH_MODE_ENUM
{
  PREARM_BTN,
  ABORT_BTN,
  ARM_ON,
  ARM_OFF,
  LAUNCH_BTN
};

LAUNCH_MODE_ENUM launchModePress;

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
  unsigned long lastDebounceTime;
};

DebouncedInput gn2Flow;
DebouncedInput lngFlow;
DebouncedInput loxFlow;
DebouncedInput gn2Vent;
DebouncedInput lngVent;
DebouncedInput loxVent;
DebouncedInput arm;
DebouncedInput abort;
DebouncedInput launch;
DebouncedInput launch_M;
DebouncedInput fueling_M;
DebouncedInput dev_M;

struct switch_control
{
  DebouncedInput *input;
  unsigned int mask;
};

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

uint8_t openValve(uint8_t rocket_state, uint8_t valve)
{
  return rocket_state | valve;
}

uint8_t closeValve(uint8_t rocket_state, uint8_t valve)
{
  return rocket_state & ~valve;
}

MODE getModePress(MODE PRE_MODE)
{
  // read relevent switches/buttons
  debounceButtonRead(&launch_M);
  debounceButtonRead(&fueling_M);
  debounceButtonRead(&dev_M);

  unsigned int launch_button = launch_M.currState;
  unsigned int fueling_button = fueling_M.currState; 
  unsigned int dev_button = dev_M.currState;

  if (launch_button + fueling_button + dev_button > 1){
    return NONE_MODE;
  }
  else if (launch_button){
    return LAUNCH_MODE;
  }
  else if (fueling_button){
    return FUELING_MODE;
  }
  else if (dev_button){
    return DEV_MODE;
  }
  else{
    return PRE_MODE;
  }
}

LAUNCH_MODE_ENUM getLaunchModePress(LAUNCH_MODE_ENUM PRE_MODE)
{
  // read relevent switches/buttons
  debounceSwitchRead(&arm);
  debounceButtonRead(&abort);
  debounceButtonRead(&launch);

  unsigned int arm_switch = arm.currState;
  unsigned int abort_button = abort.currState;
  unsigned int launch_button = launch.currState;

  if (abort_button & launch_button){
    return PREARM_BTN;
  }
  else if (arm_switch){
    return ARM_ON;
  }
  else if (!arm_switch){
    return ARM_OFF;
  }
  else if (abort_button){
    return ABORT_BTN;
  }
  else if (launch_button){
    return LAUNCH_BTN;
  }
  else{
    return PRE_MODE;
  }
}

void launch_mode_logic(){
  launchModePress = getLaunchModePress(launchModePress);

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

  switch_control control_list[] = {
      {&gn2Vent, gn2_vent_mask},
      {&lngVent, lng_vent_mask},
      {&loxVent, lox_vent_mask}};

  for (switch_control item : control_list)
  {
    debounceSwitchRead(item.input);

    if (item.input->currState)
    {
      rocketState = openValve(rocketState, control_list->mask);
    }
    else
    {
      rocketState = closeValve(rocketState, control_list->mask);
    }
  }

  rocketState = closeValve(rocketState, gn2_flow_mask);
  rocketState = closeValve(rocketState, lng_flow_mask);
  rocketState = closeValve(rocketState, lox_flow_mask);
}

void dev_mode_logic(){

  switch_control control_list[] = {
      {&gn2Flow, gn2_flow_mask},
      {&lngFlow, lng_flow_mask},
      {&loxFlow, lox_flow_mask},
      {&gn2Vent, gn2_vent_mask},
      {&lngVent, lng_vent_mask},
      {&loxVent, lox_vent_mask}};

  for (switch_control item : control_list)
  {
    debounceSwitchRead(item.input);

    if (item.input->currState)
    {
      rocketState = openValve(rocketState, control_list->mask);
    }
    else
    {
      rocketState = closeValve(rocketState, control_list->mask);
    }
  }
}


//TODO: implement send rocket state
void sendRocketState(uint8_t currRocketState)
{
}

void setup() {

  // set current state of the rocket
  rocketState = 0b000000;

  operationMode = NONE_MODE;
  launchModePress = PREARM_BTN;

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

  gn2Flow = {PIN_GN2_F, LOW, LOW, 0};
  lngFlow = {PIN_LNG_F, LOW, LOW, 0};
  loxFlow = {PIN_LOX_F, LOW, LOW, 0};
  gn2Vent = {PIN_GN2_V, LOW, LOW, 0};
  lngVent = {PIN_LNG_V, LOW, LOW, 0};
  loxVent = {PIN_LOX_V, LOW, LOW, 0};
  arm = {PIN_ARM, LOW, LOW, 0};
  abort = {PIN_ABORT, LOW, LOW, 0};
  launch = {PIN_LAUNCH, LOW, LOW, 0};
  launch_M = {PIN_LAUNCH_M, LOW, LOW, 0};
  fueling_M = {PIN_FUELING_M, LOW, LOW, 0};
  dev_M = {PIN_DEV_M, LOW, LOW, 0};
}

void loop() {
  operationMode = getModePress(operationMode);

  switch (operationMode)
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

  // TODO: implement sendRocketState that send
  //       rocketState to the flight computer
  sendRocketState(rocketState); // TODO: implement
  delay(50);              // small debounce or loop delay
}
