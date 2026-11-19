#include "mock_arduino/Arduino.h"
#include "../../gs/constants.h"

// Forward declarations (matching Arduino IDE preprocessor)
void LCD_LaunchMode();
void LCD_DevAndFueling(int mode);
static void printIsoTimestamp();
void sendRocketState(uint8_t currRocketState);

#include "../../gs/gs.ino"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

extern std::vector<std::vector<uint8_t>> mock_sent_packets;
extern std::vector<std::vector<uint8_t>> mock_rx_queue;

// Test framework statistics
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_CASE(name) static void name(); \
    static struct Register_##name { \
        Register_##name() { register_test(#name, name); } \
    } reg_##name; \
    static void name()

typedef void (*TestFunc)();
struct TestCaseEntry {
    std::string name;
    TestFunc func;
};

static std::vector<TestCaseEntry>& get_test_registry() {
    static std::vector<TestCaseEntry> registry;
    return registry;
}

static void register_test(const std::string& name, TestFunc func) {
    get_test_registry().push_back({name, func});
}

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cerr << "\n    \033[1;31mASSERTION FAILED:\033[0m " #cond \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error(#cond); \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(actual, expected) do { \
    auto a = (actual); \
    auto e = (expected); \
    if (a != e) { \
        std::cerr << "\n    \033[1;31mASSERTION FAILED:\033[0m Expected " #actual " == " #expected \
                  << " (" << a << " != " << e << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error("Equality failure"); \
    } \
} while(0)

#define ASSERT_HEX_EQ(actual, expected) do { \
    uint32_t a = (actual); \
    uint32_t e = (expected); \
    if (a != e) { \
        std::cerr << "\n    \033[1;31mASSERTION FAILED:\033[0m Expected 0x" \
                  << std::hex << (int)e << " but got 0x" << (int)a \
                  << std::dec << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error("Hex equality failure"); \
    } \
} while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) do { \
    std::string h = (haystack); \
    std::string n = (needle); \
    if (h.find(n) == std::string::npos) { \
        std::cerr << "\n    \033[1;31mASSERTION FAILED:\033[0m Expected to find '" << n \
                  << "' in '" << h << "' at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error("String contains failure"); \
    } \
} while(0)

static void advance_time(unsigned long ms) {
    simulated_time_ms += ms;
}

static void debounce_all() {
    debounceSwitchRead(&lngPressure);
    debounceSwitchRead(&loxPressure);
    debounceSwitchRead(&lngFlow);
    debounceSwitchRead(&loxFlow);
    debounceSwitchRead(&lngVent);
    debounceSwitchRead(&loxVent);
    debounceSwitchRead(&vent_sw);
    debounceSwitchRead(&arm);
    debounceAbortRead(&abort_mission);
    debounceButtonRead(&launch);
    debounceButtonRead(&launch_M);
    debounceButtonRead(&fueling_M);
    debounceButtonRead(&dev_M);
}

static void debounce_settle() {
    // 1st cycle: detect edges and record lastDebounceTime
    debounce_all();
    // Advance time past debounce delay (30 ms)
    advance_time(50);
    // 2nd cycle: debounce interval elapsed, latch into currState
    debounce_all();
}

static void reset_test_environment() {
    simulated_pins.clear();
    
    // Active-LOW switches & buttons: HIGH = unpressed / DOWN
    simulated_pins[PIN_LNG_P] = HIGH;
    simulated_pins[PIN_LOX_P] = HIGH;
    simulated_pins[PIN_LNG_F] = HIGH;
    simulated_pins[PIN_LOX_F] = HIGH;
    simulated_pins[PIN_LNG_V] = HIGH;
    simulated_pins[PIN_LOX_V] = HIGH;
    simulated_pins[PIN_VENT_SW] = HIGH;
    simulated_pins[PIN_ARM] = HIGH;
    simulated_pins[PIN_LAUNCH] = HIGH;
    simulated_pins[PIN_LAUNCH_M] = HIGH;
    simulated_pins[PIN_FUELING_M] = HIGH;
    simulated_pins[PIN_DEV_M] = HIGH;

    // Active-HIGH abort button: LOW = unlatched/normal
    simulated_pins[PIN_ABORT] = LOW;

    simulated_time_ms = 1000;
    mock_sent_packets.clear();
    mock_rx_queue.clear();
    
    // Reset firmware globals
    operationMode = NONE_MODE;
    pre_operationMode = NONE_MODE;
    launchEntryLocked = false;
    rocketState = PRE_ARM;
    
    setup();
    debounce_settle();
}

static void set_switch_up(int pin) {
    simulated_pins[pin] = LOW; // Active-LOW
}

static void set_switch_down(int pin) {
    simulated_pins[pin] = HIGH;
}

static void set_button_pressed(int pin) {
    simulated_pins[pin] = LOW;
}

static void set_button_released(int pin) {
    simulated_pins[pin] = HIGH;
}

static void set_abort_latched() {
    simulated_pins[PIN_ABORT] = HIGH; // Active-HIGH
}

static void set_abort_unlatched() {
    simulated_pins[PIN_ABORT] = LOW;
}

// ============================================================================
// Group 1: Valve Masks & State Encoding Constants
// ============================================================================

TEST_CASE(Test_Constants_Bitmasks) {
    reset_test_environment();
    ASSERT_HEX_EQ(LNG_PRES_MASK, 0b01000000);
    ASSERT_HEX_EQ(LOX_PRES_MASK, 0b00100000);
    ASSERT_HEX_EQ(LNG_FLOW_MASK, 0b00001000);
    ASSERT_HEX_EQ(LNG_VENT_MASK, 0b00000100);
    ASSERT_HEX_EQ(LOX_FLOW_MASK, 0b00000010);
    ASSERT_HEX_EQ(LOX_VENT_MASK, 0b00000001);
    ASSERT_HEX_EQ(NULL_MASK,     0b00000000);
    ASSERT_HEX_EQ(FAIL_OPEN_MASK,0b00000101);
}

TEST_CASE(Test_Constants_LaunchStates) {
    reset_test_environment();
    ASSERT_HEX_EQ(PRE_ARM,      0b00000000); // 0x00: all coils unpowered
    ASSERT_HEX_EQ(VENTS_CLOSED, 0b00000101); // 0x05: vent coils powered shut
    ASSERT_HEX_EQ(ARMED,        0b01100101); // 0x65: vents shut, pressures open
    ASSERT_HEX_EQ(LAUNCH,       0b01101111); // 0x6F: vents shut, pressures open, flows open
    ASSERT_HEX_EQ(ABORT,        0b10000000); // 0x80: non-valve flag, all coils 0V
}

TEST_CASE(Test_Helper_OpenCloseValve) {
    reset_test_environment();
    uint8_t state = 0;
    state = openValve(state, LNG_PRES_MASK);
    ASSERT_HEX_EQ(state, 0b01000000);
    state = openValve(state, LOX_VENT_MASK);
    ASSERT_HEX_EQ(state, 0b01000001);
    state = closeValve(state, LNG_PRES_MASK);
    ASSERT_HEX_EQ(state, 0b00000001);
    state = closeValve(state, LOX_VENT_MASK);
    ASSERT_HEX_EQ(state, 0b00000000);
}

// ============================================================================
// Group 2: Developer Mode (`DEV MODE`)
// ============================================================================

TEST_CASE(Test_DevMode_AllLeversDown_Unpowered) {
    reset_test_environment();
    operationMode = DEV_MODE;
    pre_operationMode = DEV_MODE;
    
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);
}

TEST_CASE(Test_DevMode_IndividualLevers) {
    reset_test_environment();
    operationMode = DEV_MODE;
    pre_operationMode = DEV_MODE;
    
    // 1. LNG_P
    set_switch_up(PIN_LNG_P);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LNG_PRES_MASK);
    set_switch_down(PIN_LNG_P);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);

    // 2. LOX_P
    set_switch_up(PIN_LOX_P);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LOX_PRES_MASK);
    set_switch_down(PIN_LOX_P);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);

    // 3. LNG_F
    set_switch_up(PIN_LNG_F);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LNG_FLOW_MASK);
    set_switch_down(PIN_LNG_F);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);

    // 4. LOX_F
    set_switch_up(PIN_LOX_F);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LOX_FLOW_MASK);
    set_switch_down(PIN_LOX_F);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);

    // 5. LNG_V (Fail-open: UP energizes coil shut)
    set_switch_up(PIN_LNG_V);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LNG_VENT_MASK);
    set_switch_down(PIN_LNG_V);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);

    // 6. LOX_V
    set_switch_up(PIN_LOX_V);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, LOX_VENT_MASK);
    set_switch_down(PIN_LOX_V);
    debounce_settle();
    dev_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);
}

// ============================================================================
// Group 3: Fueling Mode (`FUELING MODE`)
// ============================================================================

TEST_CASE(Test_FuelingMode_Lockouts) {
    reset_test_environment();
    operationMode = FUELING_MODE;
    pre_operationMode = FUELING_MODE;

    // Flip flow and pressure levers UP
    set_switch_up(PIN_LNG_P);
    set_switch_up(PIN_LOX_P);
    set_switch_up(PIN_LNG_F);
    set_switch_up(PIN_LOX_F);
    debounce_settle();

    fueling_mode_logic();

    // In Fueling Mode, pressures and flows MUST be locked to 0
    ASSERT_FALSE(rocketState & LNG_PRES_MASK);
    ASSERT_FALSE(rocketState & LOX_PRES_MASK);
    ASSERT_FALSE(rocketState & LNG_FLOW_MASK);
    ASSERT_FALSE(rocketState & LOX_FLOW_MASK);
    ASSERT_HEX_EQ(rocketState, 0x00);
}

TEST_CASE(Test_FuelingMode_VentControl) {
    reset_test_environment();
    operationMode = FUELING_MODE;
    pre_operationMode = FUELING_MODE;

    // Vents should still be independently controllable in Fueling Mode
    set_switch_up(PIN_LNG_V);
    debounce_settle();
    fueling_mode_logic();
    ASSERT_HEX_EQ(rocketState, LNG_VENT_MASK);

    set_switch_up(PIN_LOX_V);
    debounce_settle();
    fueling_mode_logic();
    ASSERT_HEX_EQ(rocketState, LNG_VENT_MASK | LOX_VENT_MASK);

    // Drop LNG_V down
    set_switch_down(PIN_LNG_V);
    debounce_settle();
    fueling_mode_logic();
    ASSERT_HEX_EQ(rocketState, LOX_VENT_MASK);

    // Drop LOX_V down
    set_switch_down(PIN_LOX_V);
    debounce_settle();
    fueling_mode_logic();
    ASSERT_HEX_EQ(rocketState, 0x00);
}

// ============================================================================
// Group 4: Launch Mode Entry Interlocks
// ============================================================================

TEST_CASE(Test_LaunchEntry_Clean_Passes) {
    reset_test_environment();
    bool allowed = launch_mode_check();
    ASSERT_TRUE(allowed);
    ASSERT_HEX_EQ(rocketState, PRE_ARM);
}

TEST_CASE(Test_LaunchEntry_BlockedByAnyValveLever) {
    int levers[] = {PIN_LNG_P, PIN_LOX_P, PIN_LNG_F, PIN_LOX_F, PIN_LNG_V, PIN_LOX_V};

    for (int pin : levers) {
        reset_test_environment();
        set_switch_up(pin);
        // Note: check_all_levers_down uses direct digitalRead, so instant detection!
        bool allowed = launch_mode_check();
        ASSERT_FALSE(allowed);
    }
}

TEST_CASE(Test_LaunchEntry_BlockedByArmSwitch) {
    reset_test_environment();
    set_switch_up(PIN_ARM);
    debounce_settle();

    bool allowed = launch_mode_check();
    ASSERT_FALSE(allowed);
}

TEST_CASE(Test_LaunchEntry_BlockedByVentSwitch) {
    reset_test_environment();
    set_switch_up(PIN_VENT_SW);
    debounce_settle();

    bool allowed = launch_mode_check();
    ASSERT_FALSE(allowed);
}

TEST_CASE(Test_LaunchEntry_BlockedByAbortLatched) {
    reset_test_environment();
    set_abort_latched();
    debounce_settle();

    bool allowed = launch_mode_check();
    ASSERT_FALSE(allowed);
}

TEST_CASE(Test_LaunchEntry_BlockedByLaunchButtonHeld) {
    reset_test_environment();
    set_button_pressed(PIN_LAUNCH);
    debounce_settle();

    bool allowed = launch_mode_check();
    ASSERT_FALSE(allowed);
}

// ============================================================================
// Group 5: Launch Sequencer Decoupled Arming (The Core Safety Guarantee)
// ============================================================================

TEST_CASE(Test_LaunchSequencer_HappyTrail) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = PRE_ARM;
    LCD_LaunchMode();

    // 1. Initial State: PRE_ARM
    ASSERT_HEX_EQ(rocketState, PRE_ARM);
    ASSERT_STR_CONTAINS(lcd.getRow(1), "PRE ARM MODE");
    ASSERT_STR_CONTAINS(lcd.getRow(2), "V:OPN  P:CLD  F:CLD");

    // 2. Operator flips Vent Switch UP -> VENTS_CLOSED
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);
    ASSERT_STR_CONTAINS(lcd.getRow(1), "VENTS CLOSED");
    ASSERT_STR_CONTAINS(lcd.getRow(2), "V:CLD  P:CLD  F:CLD");

    // 3. Operator flips covered ARM Switch UP -> ARMED
    set_switch_up(PIN_ARM);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, ARMED);
    ASSERT_STR_CONTAINS(lcd.getRow(1), "ARM MODE");
    ASSERT_STR_CONTAINS(lcd.getRow(2), "V:CLD  P:OPN  F:CLD");

    // 4. Operator presses momentary LAUNCH button -> LAUNCH
    set_button_pressed(PIN_LAUNCH);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, LAUNCH);
    ASSERT_STR_CONTAINS(lcd.getRow(1), "ROCKET LAUNCHING!");
    ASSERT_STR_CONTAINS(lcd.getRow(2), "V:CLD  P:OPN  F:OPN");
}

TEST_CASE(Test_Invariant_ArmWithoutVentsIsImpossible) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = PRE_ARM;

    // Operator attempts to flip ARM UP while Vent Switch remains DOWN
    set_switch_up(PIN_ARM);
    set_switch_down(PIN_VENT_SW);
    debounce_settle();

    launch_mode_logic();

    // CORE SAFETY INVARIANT: ARM switch MUST be ignored! rocketState MUST stay PRE_ARM!
    ASSERT_HEX_EQ(rocketState, PRE_ARM);
    ASSERT_FALSE(rocketState & LNG_PRES_MASK);
    ASSERT_FALSE(rocketState & LOX_PRES_MASK);
}

TEST_CASE(Test_Disarm_HoldCount_Behavior) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    
    // Arm the rocket properly
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    set_switch_up(PIN_ARM);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, ARMED);

    // Count is held: Disarm (flip ARM switch DOWN, Vent Switch remains UP)
    set_switch_down(PIN_ARM);
    debounce_settle();
    launch_mode_logic(); // Transitions from ARMED to VENTS_CLOSED

    // In VENTS_CLOSED with vent switch UP, loop latches vent_ready_to_open = true
    launch_mode_logic();

    // Pressure valves must close immediately; vents remain closed holding pressure
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);
    ASSERT_FALSE(rocketState & LNG_PRES_MASK);
    ASSERT_FALSE(rocketState & LOX_PRES_MASK);

    // Operator then depressurizes (flip Vent Switch DOWN)
    set_switch_down(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();

    // Returns to PRE_ARM (all valves unpowered, vents spring open)
    ASSERT_HEX_EQ(rocketState, PRE_ARM);
}

TEST_CASE(Test_ArmedMode_VentSwitchIgnored) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;

    // 1. Arm the rocket properly
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    set_switch_up(PIN_ARM);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, ARMED);

    // 2. Operator pushes down Vent Switch while ARM is still UP
    set_switch_down(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();

    // INVARIANT: Vent switch MUST be ignored while ARMED!
    // Cannot drop from ARMED to PRE_ARM directly. Must stay ARMED.
    ASSERT_HEX_EQ(rocketState, ARMED);
    ASSERT_TRUE(rocketState & FAIL_OPEN_MASK); // Vents stay powered closed
    ASSERT_TRUE(rocketState & LNG_PRES_MASK);  // Pressure stays powered open
    ASSERT_TRUE(rocketState & LOX_PRES_MASK);

    // 3. To de-escalate, operator flips ARM switch DOWN
    set_switch_down(PIN_ARM);
    debounce_settle();
    launch_mode_logic();

    // PNEUMATIC SEQUENCING PROTECTION:
    // Even though Vent Switch is DOWN, console MUST NOT immediately open vents!
    // Pressures snap shut, but vents STAY POWERED SHUT in VENTS_CLOSED!
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);
    ASSERT_TRUE(rocketState & FAIL_OPEN_MASK); // Vents stay powered closed
    ASSERT_FALSE(rocketState & LNG_PRES_MASK); // Pressures are shut off
    ASSERT_FALSE(rocketState & LOX_PRES_MASK);

    // Running additional logic loops still holds VENTS_CLOSED (no simultaneous depower!)
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);

    // 4. Operator manually cycles Vent switch UP, then DOWN to command vent opening
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);

    set_switch_down(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, PRE_ARM); // Cleanly transitions to PRE_ARM!
    ASSERT_FALSE(rocketState & FAIL_OPEN_MASK); // Vents now unpower and spring open
}

TEST_CASE(Test_CannotCloseVentsWhileArmIsUp) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = PRE_ARM;
    
    // In PRE_ARM: ARM switch is UP, Vent switch is DOWN
    set_switch_up(PIN_ARM);
    set_switch_down(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, PRE_ARM);

    // Operator flips Vent switch UP while ARM is still UP
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();

    // INVARIANT: Must NOT enter VENTS_CLOSED or ARMED! Must stay PRE_ARM!
    ASSERT_HEX_EQ(rocketState, PRE_ARM);
    ASSERT_FALSE(rocketState & FAIL_OPEN_MASK);
    ASSERT_FALSE(rocketState & LNG_PRES_MASK);
    ASSERT_FALSE(rocketState & LOX_PRES_MASK);

    // Operator resets both switches DOWN
    set_switch_down(PIN_VENT_SW);
    set_switch_down(PIN_ARM);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, PRE_ARM);

    // Now that ARM is DOWN, flipping Vent switch UP cleanly transitions to VENTS_CLOSED
    set_switch_up(PIN_VENT_SW);
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, VENTS_CLOSED);
}

TEST_CASE(Test_ValveLeversIgnoredInLaunchMode) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = PRE_ARM;

    // Toggling manual valve levers during Launch Mode must have zero effect
    set_switch_up(PIN_LNG_P);
    set_switch_up(PIN_LOX_F);
    debounce_settle();
    launch_mode_logic();

    ASSERT_HEX_EQ(rocketState, PRE_ARM);
}

// ============================================================================
// Group 6: Emergency Abort & Recovery Interlocks
// ============================================================================

TEST_CASE(Test_AbortTrigger_FromAllStates) {
    uint8_t states[] = {PRE_ARM, VENTS_CLOSED, ARMED, LAUNCH};

    for (uint8_t st : states) {
        reset_test_environment();
        operationMode = LAUNCH_MODE;
        pre_operationMode = LAUNCH_MODE;
        rocketState = st;

        set_abort_latched();
        debounce_settle();
        launch_mode_logic();

        ASSERT_HEX_EQ(rocketState, ABORT);
        ASSERT_STR_CONTAINS(lcd.getRow(1), "MISSION ABORT!");
        ASSERT_STR_CONTAINS(lcd.getRow(2), "V:OPN  P:CLD  F:CLD");
        ASSERT_STR_CONTAINS(lcd.getRow(3), "ALL VENTS OPENED!");
    }
}

TEST_CASE(Test_AbortExitInterlock_CleanExit) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = ABORT;
    
    // 1. Unlatch abort button with all levers DOWN
    set_abort_unlatched();
    debounce_settle();

    // 2. Press DEV MODE button
    set_button_pressed(PIN_DEV_M);
    debounce_settle();

    MODE nextMode = getModePress(LAUNCH_MODE);
    ASSERT_EQ(nextMode, DEV_MODE);
}

TEST_CASE(Test_AbortExitInterlock_BlockedWhenLeverUp) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = ABORT;

    // Flip LNG_P lever UP during abort
    set_switch_up(PIN_LNG_P);
    set_abort_unlatched();
    debounce_settle();

    // Attempt to press DEV MODE
    set_button_pressed(PIN_DEV_M);
    debounce_settle();

    MODE nextMode = getModePress(LAUNCH_MODE);
    
    // ABORT EXIT INTERLOCK: MUST REJECT TRANSITION! Must stay LAUNCH_MODE (ABORT)
    ASSERT_EQ(nextMode, LAUNCH_MODE);
    ASSERT_HEX_EQ(rocketState, ABORT);
}

TEST_CASE(Test_AbortExitInterlock_BlockedWhenVentLeverUp) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    pre_operationMode = LAUNCH_MODE;
    rocketState = ABORT;

    // Flip LOX_V lever UP during abort
    set_switch_up(PIN_LOX_V);
    set_abort_unlatched();
    debounce_settle();

    // Attempt to press FUELING MODE
    set_button_pressed(PIN_FUELING_M);
    debounce_settle();

    MODE nextMode = getModePress(LAUNCH_MODE);
    
    // Rejection required
    ASSERT_EQ(nextMode, LAUNCH_MODE);
    ASSERT_HEX_EQ(rocketState, ABORT);
}

// ============================================================================
// Group 7: In-Flight Mode Lockouts
// ============================================================================

TEST_CASE(Test_InFlightLockout_VentsClosed) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    rocketState = VENTS_CLOSED;

    set_button_pressed(PIN_DEV_M);
    debounce_settle();
    MODE m = getModePress(LAUNCH_MODE);
    ASSERT_EQ(m, LAUNCH_MODE); // Locked out!
}

TEST_CASE(Test_InFlightLockout_Armed) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    rocketState = ARMED;

    set_button_pressed(PIN_FUELING_M);
    debounce_settle();
    MODE m = getModePress(LAUNCH_MODE);
    ASSERT_EQ(m, LAUNCH_MODE); // Locked out!
}

TEST_CASE(Test_InFlightLockout_Launch) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    rocketState = LAUNCH;

    set_button_pressed(PIN_DEV_M);
    debounce_settle();
    MODE m = getModePress(LAUNCH_MODE);
    ASSERT_EQ(m, LAUNCH_MODE); // Locked out!
}

TEST_CASE(Test_ProtocolA_PostLaunch_Abort_To_Dev) {
    reset_test_environment();
    operationMode = LAUNCH_MODE;
    rocketState = LAUNCH;

    // 1. While in active burn (LAUNCH), DEV mode button is locked out
    set_button_pressed(PIN_DEV_M);
    debounce_settle();
    MODE m = getModePress(LAUNCH_MODE);
    ASSERT_EQ(m, LAUNCH_MODE);

    // 2. Operator triggers Engine Cutoff / Safing via ABORT mushroom button
    set_abort_latched();
    debounce_settle();
    launch_mode_logic();
    ASSERT_HEX_EQ(rocketState, ABORT); // 0V to all valves, vents spring open!

    // 3. Clear abort button (unlatch UP) with all levers DOWN
    set_abort_unlatched();
    debounce_settle();

    // 4. Operator presses DEV MODE -> Transition accepted!
    set_button_pressed(PIN_DEV_M);
    debounce_settle();
    m = getModePress(LAUNCH_MODE);
    ASSERT_EQ(m, DEV_MODE);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "  PHOENIX GROUND STATION - AUTOMATED C++ UNIT TEST SUITE    " << std::endl;
    std::cout << "============================================================" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& test : get_test_registry()) {
        g_tests_run++;
        std::cout << "  [RUN] " << std::left << std::setw(45) << test.name << " ... " << std::flush;
        try {
            test.func();
            g_tests_passed++;
            std::cout << "\033[1;32m[PASS]\033[0m" << std::endl;
        } catch (const std::exception& e) {
            g_tests_failed++;
            std::cout << "\033[1;31m[FAIL]\033[0m" << std::endl;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "\n------------------------------------------------------------" << std::endl;
    std::cout << "  Total Tests Run:  " << g_tests_run << std::endl;
    std::cout << "  \033[1;32mTests Passed:     " << g_tests_passed << "\033[0m" << std::endl;
    if (g_tests_failed > 0) {
        std::cout << "  \033[1;31mTests Failed:     " << g_tests_failed << "\033[0m" << std::endl;
    } else {
        std::cout << "  Tests Failed:     0" << std::endl;
    }
    std::cout << "  Execution Time:   " << duration_ms << " ms" << std::endl;
    std::cout << "============================================================\n" << std::endl;

    return (g_tests_failed == 0) ? 0 : 1;
}
