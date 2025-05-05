// Using binary literal for rocket state
#include <system_error>
const uint8_t PRE_ARM = 0b000000; // 0  decimal
const uint8_t ABORT = 0b010101;   // 21 decimal
const uint8_t ARMED = 0b100000;   // 32 decimal
const uint8_t LAUNCH = 0b101010;  // 42 decimal

// Current state of the rocket
uint8_t rocketState = 0b000000;

// Possible operation modes
enum operationMode
{
  NONE_MODE,
  LAUNCH_MODE,
  FUELING_MODE,
  DEV_MODE
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

DebouncedInput gn2Flow = { PIN_ARM_SWITCH,   LOW, LOW, 0 };
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

void debounceButtonStateRead(DebouncedInput *input)
{
  unsigned int pin = input->pin;
  unsigned int currState = input->currState;
  unsigned int lastState = input->lastState;
  unsigned long lastDebounceTime = input->lastDebounceTime;

  int reading = digitalRead(pin);
  if (reading != lastState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != lastState) {
      currState = reading;
    }
  }
  lastState = reading;
}

void debounceSwitchRead(DebouncedInput *input)
{
  unsigned int pin = input->pin;
  unsigned int currState = input->currState;
  unsigned int lastState = input->lastState;
  unsigned long lastDebounceTime = input->lastDebounceTime;

  int reading = digitalRead(pin);
  if (reading != lastState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currState) {
      currState = reading;
    }
  }
  lastReading = reading;
}

ButtonPress getButtonPress()
{
  // TODO: Replace with real pin reading logic or test code
  // only one button press should be registered at one time
  // if multiple button reads are registered, must have set priority.
  return NONE;
}


void setup() {
  operationMode = NONE_MODE; // start in PRE_ARM
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
  ButtonPress press = getButtonPress();

  // State machine for rocket
  switch (rocketState)
  {

  case PRE_ARM:
    if (press == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    else if (press == ARM_BTN)
    {
      rocketState = ARMED;
    }
    // else remain in PRE_ARM
    break;

  case ARMED:
    if (press == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    else if (press == LAUNCH_BTN)
    {
      rocketState = LAUNCH;
    }
    else if (press == ARM_BTN)
    {
      // might have to modify depending on how switch is read
      rocketState = PRE_ARM;
    }
    break;

  case LAUNCH:
    // rocket is launching, possibly try to abort?
    if (press == ABORT_BTN)
    {
      rocketState = ABORT;
    }
    break;

  case ABORT:
    // if in abort cannot get out
    break;
  }

  // TODO: based on current state modify valves
  setValves(rocketState); // TODO: implement
  delay(50);              // small debounce or loop delay
}
