#ifndef CONSTANTS_H
#define CONSTANTS_H

// Include all of the constant values here for easier modification.

// ─────────────────────────────────────────────────────────────────────────────
//                               State Encoding
// ─────────────────────────────────────────────────────────────────────────────
/** An 8 bits packet that holds the current valve & control state */
// Individual valve masks
const uint8_t LNG_PRES_MASK = 0b1000000; // Fail-Closed
const uint8_t LOX_PRES_MASK = 0b0100000; // Fail-Closed
const uint8_t LNG_FLOW_MASK = 0b0001000; // Fail-Closed
const uint8_t LNG_VENT_MASK = 0b0000100; // Fail-Open
const uint8_t LOX_FLOW_MASK = 0b0000010; // Fail-Closed
const uint8_t LOX_VENT_MASK = 0b0000001; // Fail-Open
const uint8_t NULL_MASK     = 0b0000000;
// Fail-Open Mask: easily flip valves between fail-closed and fail-open here
const uint8_t FAIL_OPEN_MASK = LNG_VENT_MASK | LOX_VENT_MASK; // Should be 0b0000101

// Launch sequence coil power encoding (Logical State ^ FAIL_OPEN_MASK)
const uint8_t PRE_ARM = FAIL_OPEN_MASK; // Safe: LNG-V & LOX-V energized closed (Fail Open mask), others unpowered closed, 0b0000101
const uint8_t ARMED   = PRE_ARM | (LNG_PRES_MASK | LOX_PRES_MASK); // Armed: Pressures powered open, LNG-V & LOX-V powered closed, 0b1100101
const uint8_t LAUNCH  = ARMED | LNG_FLOW_MASK | LOX_FLOW_MASK; // Launch: Pressures & Flows powered open, LNG-V & LOX-V powered closed
const uint8_t ABORT   = NULL_MASK; // Abort: WAS GN2-V powered open, LNG-V & LOX-V unpowered (spring open). Now we don't have GN2 so it's NULL

// ─────────────────────────────────────────────────────────────────────────────
//                                 Pin Mapping
// ─────────────────────────────────────────────────────────────────────────────
// Note: Pin 19 is broken
const int PIN_LNG_P = 48;
const int PIN_LOX_P = 46;
const int PIN_LNG_F = 49;
const int PIN_LOX_F = 47;
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

// ─────────────────────────────────────────────────────────────────────────────
//                           Sensor Count Setup
// ─────────────────────────────────────────────────────────────────────────────
const size_t NUM_PTS = 7;
#endif
