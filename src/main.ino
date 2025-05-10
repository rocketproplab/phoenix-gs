// for ethernet module
#include "w5500.h"

// for LCD display
#include <Wire.h>
// #include <LiquidCrystal_I2C.h>

// Initialize the LCD object, set the LCD I2C address to 0x27 for a 20x4 display
// LiquidCrystal_I2C lcd(0x27, 20, 4);

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

const uint8_t null_mask = 0b000000;

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

const int PIN_GN2_F = 22;
const int PIN_LNG_F = 2;
const int PIN_LOX_F = 3;
const int PIN_GN2_V = 4;
const int PIN_LNG_V = 5;
const int PIN_LOX_V = 6;
const int PIN_ARM = 7;
const int PIN_LAUNCH = 8;
const int PIN_ABORT = 9;
const int PIN_LAUNCH_M = 10;
const int PIN_FUELING_M = 11;
const int PIN_DEV_M = 12;

// —— GLOBAL STATE ——
// Raw + debounced states:
struct DebouncedInput
{
  unsigned int pin;
  unsigned int currState;
  unsigned int lastState;
  unsigned long lastDebounceTime;
  uint8_t mask;
};

DebouncedInput gn2Flow;
DebouncedInput lngFlow;
DebouncedInput loxFlow;
DebouncedInput gn2Vent;
DebouncedInput lngVent;
DebouncedInput loxVent;
DebouncedInput arm;
DebouncedInput abort_mission;
DebouncedInput launch;
DebouncedInput launch_M;
DebouncedInput fueling_M;
DebouncedInput dev_M;

// the debounce time; increase if the output flickers
unsigned long debounceButtonDelay = 30;
unsigned long debounceSwitchDelay = 30;

// networking vars
const byte mac_address[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
byte pkt[] = {
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, // destination
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, // source
    0x63, 0xe4,                         // experimental ethertype
    0xFF,                               // payload
};

uint8_t buffer[1514];
uint32_t send_count = 0;
Wiznet5500 w5500;

void printMACAddress(const uint8_t address[6])
{
  for (uint8_t i = 0; i < 6; ++i)
  {
    // printPaddedHex(address[i]);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();
}

void debounceButtonRead(DebouncedInput *input)
{
  int reading = digitalRead(input->pin);

  if (reading == LOW)
  {
    reading = HIGH;
  }
  else if (reading == HIGH)
  {
    reading = LOW;
  }

  if (reading != input->lastState)
  {
    input->lastDebounceTime = millis();
  }

  if ((millis() - input->lastDebounceTime) > debounceButtonDelay)
  {
    if (reading != input->currState)
    {
      input->currState = reading;
    }
  }
  input->lastState = reading;
}

void debounceSwitchRead(DebouncedInput *input)
{
  int reading = digitalRead(input->pin);

  if (reading == LOW)
  {
    reading = HIGH;
  }
  else if (reading == HIGH)
  {
    reading = LOW;
  }

  if (reading != input->lastState)
  {
    input->lastDebounceTime = millis();
  }

  if ((millis() - input->lastDebounceTime) > debounceButtonDelay)
  {
    if (reading != input->currState)
    {
      input->currState = reading;
    }
  }
  input->lastState = reading;
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

  if (launch_button + fueling_button + dev_button > 1)
  {
    return PRE_MODE;
  }
  else if (launch_button)
  {
    if (PRE_MODE != LAUNCH_MODE)
    {
      rocketState = PRE_ARM;
    }
    return LAUNCH_MODE;
  }
  else if (fueling_button)
  {
    if (PRE_MODE != FUELING_MODE)
    {
      rocketState = PRE_ARM;
    }
    return FUELING_MODE;
  }
  else if (dev_button)
  {
    if (PRE_MODE != DEV_MODE)
    {
      rocketState = PRE_ARM;
    }
    return DEV_MODE;
  }
  else
  {
    return PRE_MODE;
  }
}

void launch_mode_logic()
{
  // read relevent switches/buttons
  debounceSwitchRead(&arm);
  debounceButtonRead(&abort_mission);
  debounceButtonRead(&launch);

  unsigned int arm_switch = arm.currState;
  unsigned int abort_button = abort_mission.currState;
  unsigned int launch_button = launch.currState;

  // State machine for rocket
  switch (rocketState)
  {
  case PRE_ARM:
    if (abort_button)
    {
      rocketState = ABORT;
    }
    else if (arm_switch)
    {
      rocketState = ARMED;
    }
    break;

  case ARMED:
    if (abort_button)
    {
      rocketState = ABORT;
    }
    else if (!arm_switch)
    {
      rocketState = PRE_ARM;
    }
    else if (launch_button)
    {
      rocketState = LAUNCH;
    }
    break;

  case LAUNCH:
    // rocket is launching
    // in static fire: abort is possible
    // in launch: abort is not possible
    if (abort_button)
    {
      rocketState = ABORT;
    }
    break;

  case ABORT:
    // if in abort cannot get out
    break;
  }
}

void fueling_mode_logic()
{
  DebouncedInput *valve_list[] = {&gn2Vent,
                                  &lngVent,
                                  &loxVent};

  for (DebouncedInput *item : valve_list)
  {
    debounceSwitchRead(item);

    if (item->currState)
    {
      rocketState = openValve(rocketState, item->mask);
    }
    else
    {
      rocketState = closeValve(rocketState, item->mask);
    }
  }

  rocketState = closeValve(rocketState, gn2_flow_mask);
  rocketState = closeValve(rocketState, lng_flow_mask);
  rocketState = closeValve(rocketState, lox_flow_mask);
}

void dev_mode_logic()
{

  DebouncedInput *valve_list[] = {&gn2Flow,
                                  &lngFlow,
                                  &loxFlow,
                                  &gn2Vent,
                                  &lngVent,
                                  &loxVent};

  for (DebouncedInput *item : valve_list)
  {
    debounceSwitchRead(item);
    // Serial.println("Pin Number: ");
    // Serial.println(item.input->pin);
    // Serial.println("State: ");
    // Serial.println(item.input->currState);

    if (item->currState)
    {
      // Serial.println("Opening valve: ");
      // Serial.println(control_list->mask);
      rocketState = openValve(rocketState, item->mask);
    }
    else
    {
      // Serial.println("Closing valve: ");
      rocketState = closeValve(rocketState, item->mask);
    }
  }
}

// TODO: implement send rocket state
void sendRocketState(uint8_t currRocketState)
{

  // Serial.println("sending signals");
  pkt[14] = currRocketState;
  if (w5500.sendFrame(pkt, sizeof(pkt)) < 0)
  {
    Serial.println("Failed to send packet " + send_count);
  }
  ++send_count;
}

// void displatyRocketState()
// {
//   // Set cursor to the top left corner and print the string on the first row
//   lcd.setCursor(0, 0);
//   lcd.print("    Hello, world!    ");

//   // Move to the second row and print the string
//   lcd.setCursor(0, 1);
//   lcd.print("   IIC/I2C LCD2004  ");

//   // Move to the third row and print the string
//   lcd.setCursor(0, 2);
//   lcd.print("  20 cols, 4 rows   ");

//   // Move to the fourth row and print the string
//   lcd.setCursor(0, 3);
//   lcd.print(" www.sunfounder.com ");
// }

void setup()
{
  Serial.begin(9600);

  // set current state of the rocket
  rocketState = 0b000000;

  operationMode = NONE_MODE;

  // initialize all inputs:
  pinMode(PIN_GN2_F, INPUT_PULLUP);
  pinMode(PIN_LNG_F, INPUT_PULLUP);
  pinMode(PIN_LOX_F, INPUT_PULLUP);
  pinMode(PIN_GN2_V, INPUT_PULLUP);
  pinMode(PIN_LNG_V, INPUT_PULLUP);
  pinMode(PIN_LOX_V, INPUT_PULLUP);
  pinMode(PIN_ARM, INPUT_PULLUP);
  pinMode(PIN_ABORT, INPUT_PULLUP);
  pinMode(PIN_LAUNCH, INPUT_PULLUP);
  pinMode(PIN_LAUNCH_M, INPUT_PULLUP);
  pinMode(PIN_FUELING_M, INPUT_PULLUP);
  pinMode(PIN_DEV_M, INPUT_PULLUP);

  gn2Flow = {PIN_GN2_F, LOW, LOW, 0, gn2_flow_mask};
  lngFlow = {PIN_LNG_F, LOW, LOW, 0, lng_flow_mask};
  loxFlow = {PIN_LOX_F, LOW, LOW, 0, lox_flow_mask};
  gn2Vent = {PIN_GN2_V, LOW, LOW, 0, gn2_vent_mask};
  lngVent = {PIN_LNG_V, LOW, LOW, 0, lng_vent_mask};
  loxVent = {PIN_LOX_V, LOW, LOW, 0, lox_vent_mask};
  arm = {PIN_ARM, LOW, LOW, 0, null_mask};
  abort_mission = {PIN_ABORT, LOW, LOW, 0, null_mask};
  launch = {PIN_LAUNCH, LOW, LOW, 0, null_mask};
  launch_M = {PIN_LAUNCH_M, LOW, LOW, 0, null_mask};
  fueling_M = {PIN_FUELING_M, LOW, LOW, 0, null_mask};
  dev_M = {PIN_DEV_M, LOW, LOW, 0, null_mask};

  Serial.println("[W5500MacRaw]");
  printMACAddress(mac_address);
  w5500.begin(mac_address);

  // lcd.init();      // Initialize the LCD
  // lcd.backlight(); // Turn on the backlight
}

void loop()
{
  // Serial.println("Running");
  operationMode = getModePress(operationMode);
  sendRocketState(rocketState);
  // displatyRocketState();

  switch (operationMode)
  {
  case LAUNCH_MODE:
    Serial.println("Launch Mode");
    Serial.println("Rocket State: ");
    Serial.println(rocketState, BIN);
    launch_mode_logic();
    break;

  case FUELING_MODE:
    Serial.println("Fueling Mode");
    Serial.println("Rocket State: ");
    Serial.println(rocketState, BIN);
    fueling_mode_logic();
    break;

  case DEV_MODE:
    Serial.println("Dev Mode");
    Serial.println("Rocket State: ");
    Serial.println(rocketState, BIN);
    dev_mode_logic();
    break;

  default:
    Serial.println("Idling Mode");
    break;
  }

  // TODO: implement sendRocketState that send
  //       rocketState to the flight computer
  // sendRocketState(rocketState); // TODO: implement
  delay(50); // small debounce or loop delay
}