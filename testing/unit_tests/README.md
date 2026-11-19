# Phoenix Ground Station: Automated C++ Unit Test Suite

This directory contains a native C++ automated unit test harness that compiles and runs directly on a development PC (Windows/Linux/macOS) or in a GitHub Actions CI pipeline.

It tests the **actual production firmware** (`gs/gs.ino` and `gs/constants.h`) without requiring physical hardware or modifying any code in the `gs/` directory.

---

## What It Tests

The test suite validates 25 critical flight-software scenarios:

1. **State & Valve Mask Encodings:** Validates `0b00000000` (PRE_ARM), `0b00000101` (VENTS_CLOSED), `0b01100101` (ARMED), `0b01101111` (LAUNCH), `0b10000000` (ABORT), and all individual 8-bit valve masks.
2. **Developer Mode (`DEV MODE`):** Direct toggle control of all 6 valves and accurate physical state translation (`OPN` vs `CLD`).
3. **Fueling Mode (`FUELING MODE`):** Strict software lockout of pressure and flow valves, independent manual control of tank vents, and rapid vent exercising routines.
4. **Launch Mode Entry Interlocks:** Event ID 0 (blocked if any of the 6 valve levers is UP) and Event ID 1 (blocked if ARM, Vent switch, Launch button, or ABORT is active).
5. **Decoupled 2-Stage Arming (The Happy Trail):** `PRE_ARM` $\rightarrow$ Vent Switch UP (`VENTS_CLOSED`) $\rightarrow$ ARM Switch UP (`ARMED`) $\rightarrow$ LAUNCH button (`LAUNCH`).
6. **Core Safety Invariants:**
   * **Arming without vents sealed is IMPOSSIBLE:** Verifies that flipping ARM UP while Vent switch is DOWN produces zero response and keeps pressure valves closed.
   * **Hold Count / Disarming:** Disarming returns safely to `VENTS_CLOSED` without dumping tank pressure.
   * **Automatic Pressurant Cutoff:** Dropping the Vent switch while in `ARMED` immediately cuts high-pressure $\text{GN}_2$ and drops to `PRE_ARM`.
   * **Lever Lockout:** Moving valve toggles during Launch Mode is completely ignored by software.
7. **Emergency Abort Operations:** Instant electrical power drop (0V) across all modes, mechanical E-stop latch lockout, and the 5 Abort Exit Interlock scenarios.
8. **In-Flight Mode Lockouts:** Verifies that pressing `DEV MODE` or `FUELING MODE` while in `VENTS_CLOSED`, `ARMED`, or `LAUNCH` is strictly rejected.

---

## How to Run

### Windows (PowerShell)
```powershell
.\testing\unit_tests\run_tests.ps1
```

### Windows (Command Prompt / Batch)
```cmd
testing\unit_tests\run_tests.bat
```

### Manual Compilation (`g++`)
```bash
g++.exe -std=c++17 -fpermissive -static -I./testing/unit_tests/mock_arduino -I./gs \
    ./testing/unit_tests/mock_arduino/mock_globals.cpp \
    ./testing/unit_tests/mock_arduino/mock_w5500.cpp \
    ./testing/unit_tests/unit_tests.cpp \
    -o ./testing/unit_tests/unit_tests.exe
./testing/unit_tests/unit_tests.exe
```

---

## How It Works Under the Hood

1. **Zero Production Code Changes:** The test harness directly `#include`s `../../gs/gs.ino` and `../../gs/constants.h`. The production code is never modified.
2. **Mock Arduino Environment (`mock_arduino/`):** Mocks AVR register and standard library functions (`millis()`, `delay()`, `digitalRead()`, `LiquidCrystal_I2C`, `Wiznet5500`, `Serial`).
3. **Real Time Simulation:** Mocks debounce intervals ($>30\text{ ms}$) to ensure the actual software debouncers in firmware are tested with authentic edge transitions.
