#ifndef CONSTANTS_H
#define CONSTANTS_H

// Include all of the constant values here for easier modification.

// ─────────────────────────────────────────────────────────────────────────────
//                               State Encoding
// ─────────────────────────────────────────────────────────────────────────────
/** An 8 bits packet that holds the current valve & control state */

// Launch sequence encoding
const uint8_t PRE_ARM = 0b0000000; // Safe: all valves closed
const uint8_t ABORT = 0b0010101;   // Abort: open all vent valves
const uint8_t ARMED = 0b1100000;   // Armed: ready to launch, waiting trigger
const uint8_t LAUNCH = 0b1101010;  // Launch: ignition/flight started

// Individual valve masks
const uint8_t LNG_PRES_MASK = 0b1000000;
const uint8_t LOX_PRES_MASK = 0b0100000;
const uint8_t GN2_VENT_MASK = 0b0010000;
const uint8_t LNG_FLOW_MASK = 0b0001000;
const uint8_t LNG_VENT_MASK = 0b0000100;
const uint8_t LOX_FLOW_MASK = 0b0000010;
const uint8_t LOX_VENT_MASK = 0b0000001;
const uint8_t NULL_MASK = 0b0000000;

// ─────────────────────────────────────────────────────────────────────────────
//                                 Pin Mapping
// ─────────────────────────────────────────────────────────────────────────────
const int PIN_LNG_P = 48;     // GN2 flow connected to LNG
const int PIN_LOX_P = 19;     // GN2 flow connected to LOX
const int PIN_LNG_F = 49;
const int PIN_LOX_F = 47;
const int PIN_GN2_V = 46;
const int PIN_LNG_V = 45;
const int PIN_LOX_V = 44;
const int PIN_ARM = 18;
const int PIN_LAUNCH = 17;
const int PIN_ABORT = 26;         //Active-HIGH wiring
const int PIN_LAUNCH_M = 8;       // Launch mode
const int PIN_FUELING_M = 9;      // Fueling mode
const int PIN_DEV_M = 10;         // Dev mode

// ─────────────────────────────────────────────────────────────────────────────
//                               Debounce Timing
// ─────────────────────────────────────────────────────────────────────────────
unsigned long DEBOUNCE_BUTTON_DELAY = 30; // for button
unsigned long dEBOUNCE_SWITCH_DELAY = 30; // for switches

// ─────────────────────────────────────────────────────────────────────────────
//                           Ethernet (MAC‑RAW) Setup
// ─────────────────────────────────────────────────────────────────────────────
// first byte 0x02 = locally‑administered, unicast
const uint8_t MAC_GROUND_STATION[6] = {0x02, 0x47, 0x53,
                                       0x00, 0x00, 0x01}; // GS:  ground station
const uint8_t MAC_RELIEF_VALVE[6] = {0x02, 0x52, 0x56,
                                     0x00, 0x00, 0x02}; // RV:  relief valve
const uint8_t MAC_FLOW_VALVE[6] = {0x02, 0x46, 0x4C,
                                   0x00, 0x00, 0x03}; // FL:  flow valve
const uint8_t MAC_SENSOR_GIGA[6] = {0x02, 0x53, 0x49,
                                    0x00, 0x00, 0x04}; // SI:  sensor interface

#endif