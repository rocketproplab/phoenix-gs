const uint8_t PIN_GN2_F = 1;
const uint8_t PIN_LNG_F = 2;
const uint8_t PIN_LOX_F = 3;
const uint8_t PIN_GN2_V = 4;
const uint8_t PIN_LNG_V = 5;
const uint8_t PIN_LOX_V = 6;
const uint8_t PIN_ARM = 7;
const uint8_t PIN_ABORT = 8;
const uint8_t PIN_LAUNCH = 9;
const uint8_t PIN_LAUNCH_M = 10;
const uint8_t PIN_FUELING_M = 11;
const uint8_t PIN_DEV_M = 12;

// —— GLOBAL STATE ——
// Raw + debounced states:
struct DebouncedInput {
  uint8_t  pin;
  bool     stableState;
  bool     lastReading;
  unsigned long lastChangeTime;
};
DebouncedInput gn2Flow = { PIN_ARM_SWITCH,   HIGH, HIGH, 0 };
DebouncedInput lngFlow = { PIN_START_BUTTON, HIGH, HIGH, 0 };
DebouncedInput loxFlow = { PIN_LAUNCH_BUTTON, HIGH, HIGH, 0 };
DebouncedInput gn2Vault = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput lngVault = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput loxVault = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput arm  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput abort  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput launch  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput launch_M  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput fueling_M  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };
DebouncedInput dev_M  = { PIN_ABORT_BUTTON,  HIGH, HIGH, 0 };



// High‑level sequence state:
enum MachineState { IDLE, ARMED, COUNTDOWN, LAUNCHED, ABORTED };
MachineState state = IDLE;

// Simple countdown timer (in ms)
unsigned long countdownStart = 0;
const unsigned long COUNTDOWN_DURATION = 5000; // e.g. 5 seconds

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
