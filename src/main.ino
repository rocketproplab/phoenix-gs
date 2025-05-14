// ============================================================================
// RPL Phoenix Rocket Ground‑Control Firmware Based on Arduino
// ----------------------------------------------------------------------------
// This sketch drives the ground‑control panel.  It
// reads the operator’s switches, debounces them, keeps a bit‑encoded “rocket
// state” word, and sends that state once per loop iteration over raw Ethernet
// (W5500 MAC‑raw frame).  Three operating modes are supported:
//   • LAUNCH_MODE   – normal launch countdown
//   • FUELING_MODE  – tank‑farm/fueling operations
//   • DEV_MODE      – developer mode; every valve is individually controllable
// ============================================================================

// ─────────────────────────────────────────────────────────────────────────────
//                                  Includes
// ─────────────────────────────────────────────────────────────────────────────
#include "w5500.h" // WIZnet W5500 Ethernet MAC/PHY driver (MAC‑RAW mode)
#include <Wire.h>  // I2C bus (LCD display, not currently enabled)

// #include <LiquidCrystal_I2C.h>
// LiquidCrystal_I2C lcd(0x27, 20, 4);

// ─────────────────────────────────────────────────────────────────────────────
//                               State Encoding
// ─────────────────────────────────────────────────────────────────────────────
/** An 8 bits packet that holds the current valve & control state */
uint8_t rocketState;

// Launch sequence encoding
const uint8_t PRE_ARM = 0b0000000; // Safe: all valves closed
const uint8_t ABORT   = 0b0010101; // Abort: open all vent valves
const uint8_t ARMED   = 0b1100000; // Armed: ready to launch, waiting trigger
const uint8_t LAUNCH  = 0b1101010; // Launch: ignition/flight started

// Individual valve masks
const uint8_t gn2_lgn_flow_mask = 0b1000000;
const uint8_t gn2_lox_flow_mask = 0b0100000;
const uint8_t gn2_vent_mask     = 0b0010000;
const uint8_t lng_flow_mask     = 0b0001000;
const uint8_t lng_vent_mask     = 0b0000100;
const uint8_t lox_flow_mask     = 0b0000010;
const uint8_t lox_vent_mask     = 0b0000001;

const uint8_t null_mask = 0b0000000;

// ─────────────────────────────────────────────────────────────────────────────
//                                 Pin Mapping
// ─────────────────────────────────────────────────────────────────────────────
const int PIN_GN2_LNG_F = 2; // GN2 flow connected to LGN
const int PIN_GN2_LOX_F = 3; // GN2 flow connected to LOX
const int PIN_LNG_F     = 4;
const int PIN_LOX_F     = 5;
const int PIN_GN2_V     = 6;
const int PIN_LNG_V     = 7;
const int PIN_LOX_V     = 8;
const int PIN_ARM       = 9;
const int PIN_LAUNCH    = 10;
const int PIN_ABORT     = 11;
const int PIN_LAUNCH_M  = 14; // Launch mode
const int PIN_FUELING_M = 15; // Fueling mode
const int PIN_DEV_M     = 16; // Dev mode

// ─────────────────────────────────────────────────────────────────────────────
//                                  Typedefs
// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief  Runtime filtering data for one physical input (switch / button).
 *
 * All inputs are wired as active‑LOW.  The struct tracks the debounced state
 * and implements a simple time‑based debounce; state only changes if the new
 * level remains stable for `debounceButtonDelay` ms.
 */
struct DebouncedInput
{
  unsigned int pin;
  unsigned int currState;
  unsigned int lastState;
  unsigned long lastDebounceTime;
  uint8_t mask;
};

// Instances – declared here for global lifetime; initialised in setup()
DebouncedInput gn2_lngFlow, gn2_loxFlow, lngFlow, loxFlow;
DebouncedInput gn2Vent, lngVent, loxVent;
DebouncedInput arm, abort_mission, launch;
DebouncedInput launch_M, fueling_M, dev_M;

// ─────────────────────────────────────────────────────────────────────────────
//                               Debounce Timing
// ─────────────────────────────────────────────────────────────────────────────
unsigned long debounceButtonDelay = 30; // for button
unsigned long debounceSwitchDelay = 30; // for switches

// ─────────────────────────────────────────────────────────────────────────────
//                              Operating Modes
// ─────────────────────────────────────────────────────────────────────────────
enum MODE
{
  NONE_MODE,
  LAUNCH_MODE,
  FUELING_MODE,
  DEV_MODE
};
MODE operationMode = NONE_MODE;

// ─────────────────────────────────────────────────────────────────────────────
//                           Ethernet (MAC‑RAW) Setup
// ─────────────────────────────────────────────────────────────────────────────
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

// ============================================================================
//                              Helper Functions
// ============================================================================
/**
 * @brief  Print a 6‑byte MAC address to the serial monitor.
 *
 * @param address  Pointer to 6‑byte array containing the MAC address.
 * @return void
 */
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

/**
 * @brief  Set a bit in rocket_state to open a valve.
 * @param rocket_state Current packed state word.
 * @param valve        Bit mask of the valve to open.
 * @return New packed state with valve bit set.
 */
uint8_t openValve(uint8_t rocket_state, uint8_t valve)
{ return rocket_state | valve; }

/**
 * @brief  Clear a bit in rocket_state to close a valve.
 * @param rocket_state Current packed state word.
 * @param valve        Bit mask of the valve to close.
 * @return New packed state with valve bit cleared.
 */
uint8_t closeValve(uint8_t rocket_state, uint8_t valve)
{ return rocket_state & ~valve; }

/**
 * @brief  Debounce a momentary push‑button (PULL-UP Wiring).
 *
 * Updates the `currState` field in input when a stable edge is detected.
 *
 * @param input Pointer to DebouncedInput describing the button.
 * @return void
 */
void debounceButtonRead(DebouncedInput *input)
{
  int reading = digitalRead(input->pin);
  reading = (reading == LOW) ? HIGH : LOW; // convert active‑LOW → logic‑high

  if (reading != input->lastState)
  { input->lastDebounceTime = millis(); }

  if ((millis() - input->lastDebounceTime) > debounceButtonDelay)
  {
    if (reading != input->currState)
    {
      input->currState = reading;
    }
  }
  input->lastState = reading;
}

/**
 * @brief  Debounce a momentary toggle/rotary (PULL-UP Wiring).
 *
 * Updates the `currState` field in input when a stable edge is detected.
 *
 * @param input Pointer to DebouncedInput describing the button.
 * @return void
 */
void debounceSwitchRead(DebouncedInput *input)
{
  int reading = digitalRead(input->pin);
  reading = (reading == LOW) ? HIGH : LOW; // convert active‑LOW → logic‑high

  if (reading != input->lastState)
  { input->lastDebounceTime = millis(); }

  if ((millis() - input->lastDebounceTime) > debounceSwitchDelay)
  {
    if (reading != input->currState)
    {
      input->currState = reading;
    }
  }
  input->lastState = reading;
}

/**
 * @brief  Determine the operator‑selected mode based on the three mode buttons.
 *
 * The function enforces mutual exclusion: if more than one mode button is held
 * it ignores the input and keeps the previous mode. When switching mode the
 * global rocketState is reset to PRE_ARM.
 *
 * @param PRE_MODE Previously active mode.
 * @return The (possibly updated) MODE value.
 */
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
    // Invalid: multiple selections → stay in previous mode
    return PRE_MODE;
  }
  else if (launch_button)
  {
    if (PRE_MODE != LAUNCH_MODE)      rocketState = PRE_ARM;
    return LAUNCH_MODE;
  }
  else if (fueling_button)
  {
    if (PRE_MODE != FUELING_MODE)     rocketState = PRE_ARM;
    return FUELING_MODE;
  }
  else if (dev_button)
  {
    if (PRE_MODE != DEV_MODE)         rocketState = PRE_ARM;
    return DEV_MODE;
  }
  else
  {
    // No buttons pressed → keep mode
    return PRE_MODE;
  }
}

// ============================================================================
//                            Mode‑Specific Logic
// ============================================================================
/**
 * @brief  State machine handling the full launch sequence.
 *
 * Reads ARM, ABORT and LAUNCH inputs; updates the high‑level `rocketState`
 * accordingly. Valve control is assumed to be hard‑wired to the bit pattern
 * encoded in PRE_ARM / ARMED / LAUNCH / ABORT.
 *
 * @return void
 */
void launch_mode_logic()
{
  // Read selectors
  debounceSwitchRead(&arm);
  debounceButtonRead(&abort_mission);
  debounceButtonRead(&launch);

  unsigned int arm_switch    = arm.currState;
  unsigned int abort_button  = abort_mission.currState;
  unsigned int launch_button = launch.currState;

  // State machine for rocket
  switch (rocketState)
  {
    case PRE_ARM:
      if (abort_button)       rocketState = ABORT;
      else if (arm_switch)    rocketState = ARMED;
      break;

    case ARMED:
      if (abort_button)       rocketState = ABORT;
      else if (!arm_switch)   rocketState = PRE_ARM;
      else if (launch_button) rocketState = LAUNCH;
      break;

    case LAUNCH:
      // In launch state abort may still be possible
      // In static fire: abort is possible
      // In launch: abort is not possible
      if (abort_button)       rocketState = ABORT;
      break;

    case ABORT:
      // Latched until system reset
      break;
  }
}

/**
 * @brief  Valve logic for FUELING_MODE
 *
 * Only vent valves are controllable; main flow valves are forced CLOSED.
 *
 * @return void
 */
void fueling_mode_logic()
{
  DebouncedInput *valve_list[] = {&gn2Vent,
                                  &lngVent,
                                  &loxVent};

  for (DebouncedInput *item : valve_list)
  {
    debounceSwitchRead(item);

    if (item->currState)
    { rocketState = openValve(rocketState, item->mask); }
    else
    { rocketState = closeValve(rocketState, item->mask); }
  }

  rocketState = closeValve(rocketState, gn2_lgn_flow_mask);
  rocketState = closeValve(rocketState, gn2_lox_flow_mask);
  rocketState = closeValve(rocketState, lng_flow_mask);
  rocketState = closeValve(rocketState, lox_flow_mask);
}

/**
 * @brief  Valve logic for DEV_MODE (every valve individually controllable).
 *
 * Reads all seven valve toggles and applies them directly to rocketState.
 *
 * @return void
 */
void dev_mode_logic()
{

  DebouncedInput *valve_list[] = {&gn2_lngFlow,
                                  &gn2_loxFlow,
                                  &lngFlow,
                                  &loxFlow,
                                  &gn2Vent,
                                  &lngVent,
                                  &loxVent};

  for (DebouncedInput *item : valve_list)
  {
    debounceSwitchRead(item);

    if (item->currState)
    { rocketState = openValve(rocketState, item->mask); }
    else
    { rocketState = closeValve(rocketState, item->mask); }
  }
}

// ============================================================================
//                           Communications Helpers
// ============================================================================
/**
 * @brief  Transmit the current rocket state over Ethernet (MAC‑RAW frame).
 *
 * @param currRocketState 8‑bit packed state word to be transmitted.
 * @return void
 */
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

// ============================================================================
//                                   setup()
// ============================================================================
/**
 * @brief  Arduino entry point – hardware initialisation.
 *
 * Configures pin modes, instantiates DebouncedInput structs and starts the
 * W5500 Ethernet controller.
 * @return void
 */
void setup()
{
  Serial.begin(9600);

  // Start in safe state
  rocketState = PRE_ARM;

  // Configure all control inputs as pull‑ups (active‑LOW)
  pinMode(PIN_GN2_LNG_F,  INPUT_PULLUP);
  pinMode(PIN_GN2_LOX_F,  INPUT_PULLUP);
  pinMode(PIN_LNG_F,      INPUT_PULLUP);
  pinMode(PIN_LOX_F,      INPUT_PULLUP);
  pinMode(PIN_GN2_V,      INPUT_PULLUP);
  pinMode(PIN_LNG_V,      INPUT_PULLUP);
  pinMode(PIN_LOX_V,      INPUT_PULLUP);
  pinMode(PIN_ARM,        INPUT_PULLUP);
  pinMode(PIN_ABORT,      INPUT_PULLUP);
  pinMode(PIN_LAUNCH,     INPUT_PULLUP);
  pinMode(PIN_LAUNCH_M,   INPUT_PULLUP);
  pinMode(PIN_FUELING_M,  INPUT_PULLUP);
  pinMode(PIN_DEV_M,      INPUT_PULLUP);

  // Map physical pins to DebouncedInput objects
  gn2_lngFlow  = { PIN_GN2_LNG_F, LOW, LOW, 0, gn2_lgn_flow_mask };
  gn2_loxFlow  = { PIN_GN2_LOX_F, LOW, LOW, 0, gn2_lox_flow_mask };
  lngFlow      = { PIN_LNG_F,     LOW, LOW, 0, lng_flow_mask     };
  loxFlow      = { PIN_LOX_F,     LOW, LOW, 0, lox_flow_mask     };
  gn2Vent      = { PIN_GN2_V,     LOW, LOW, 0, gn2_vent_mask     };
  lngVent      = { PIN_LNG_V,     LOW, LOW, 0, lng_vent_mask     };
  loxVent      = { PIN_LOX_V,     LOW, LOW, 0, lox_vent_mask     };
  arm          = { PIN_ARM,       LOW, LOW, 0, null_mask         };
  abort_mission= { PIN_ABORT,     LOW, LOW, 0, null_mask         };
  launch       = { PIN_LAUNCH,    LOW, LOW, 0, null_mask         };
  launch_M     = { PIN_LAUNCH_M,  LOW, LOW, 0, null_mask         };
  fueling_M    = { PIN_FUELING_M, LOW, LOW, 0, null_mask         };
  dev_M        = { PIN_DEV_M,     LOW, LOW, 0, null_mask         };

  // Bring up Ethernet in MAC‑RAW mode
  Serial.println("Ethernet Module Starting...");
  Serial.println("[W5500MacRaw]");
  printMACAddress(mac_address);
  w5500.begin(mac_address);
  Serial.println("Ethernet Start Success...");

  // lcd.init();      // Initialize the LCD
  // lcd.backlight(); // Turn on the backlight
}

// ============================================================================
//                                    loop()
// ============================================================================
/**
 * @brief  Main control loop (runs every 50 ms).
 *
 * 1. Determine selected user mode.
 * 2. Run the mode‑specific logic state machine.
 * 3. Send the packed rocket state to the flight computer.
 * @return void
 */
void loop()
{
  operationMode = getModePress(operationMode); // Mode select
  sendRocketState(rocketState);

  switch (operationMode)
  {
  case LAUNCH_MODE:
    Serial.println("Launch Mode; State = " + String(rocketState, BIN));
    launch_mode_logic();
    break;

  case FUELING_MODE:
    Serial.println("Fueling Mode; State = " + String(rocketState, BIN));
    fueling_mode_logic();
    break;

  case DEV_MODE:
    Serial.println("Dev Mode; State = " + String(rocketState, BIN));
    dev_mode_logic();
    break;

  default:
    Serial.println("Idling Mode");
    break;
  }

  delay(50);
}
