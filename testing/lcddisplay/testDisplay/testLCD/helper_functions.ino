#include "testLCD.ino"

void printDevAndFueling(int mode)
{

    /* ---------- 1. Prepare the state list ---------- */
    DebouncedInput *valve_list[] = {&gn2_lngFlow,
                                    &gn2Vent,
                                    &gn2_loxFlow,
                                    &lngVent,
                                    &lngFlow,
                                    &loxVent,
                                    &loxFlow,};

    /* ---------- 2. Prepare names (index 0 is the Dev-Mode header) ---------- */
    char *itemNames;

    const char *itemNames_Dev[8] = {
        "DEV MODE", "G2G-F",
        "GN2-V", "G2X-F",
        "LNG-V", "LNG-F"
        "LOX-V", "LOX-F",};

    const char *itemNames_Fueling[8] = {
        "FUEL MODE", "G2G-F",
        "GN2-V", "G2X-F",
        "LNG-V", "LNG-F"
        "LOX-V", "LOX-F",};

    // Decide on the mode to operate on
    itemNames = mode ? itemNames_Dev : itemNames_Fueling;
    
    /* ---------- 3. Draw the table: two cells (10 cols) per row ---------- */
    lcd.clear(); // wipe previous frame
    for (uint8_t i = 0; i < 8; ++i)
    {
        uint8_t row = i / 2;        // rows 0-3
        uint8_t col = (i % 2) * 10; // 0 or 10
        lcd.setCursor(col, row);

        if (i == 0)
        {                            // cell 0,0 → mode name
            lcd.print(itemNames[0]); // fits inside 10-char cell
            continue;                // nothing else on this cell
        }

        lcd.print(itemNames[i]); // valve label
        lcd.print(':');

        // i-1 maps to valve_list index (0-6)
        lcd.print((valve_list[i - 1]->mask & rocketState) ? "ON " : "OFF");
    }
}

void printLaunchMode(){
    lcd.setCursor(0, 0);
    lcd.print("  LAUNCH SEQUENCE  ");

    // determing the launch state
    if (PRE_ARM & rocketState){
        lcd.setCursor(0, 1);
        lcd.print("    PRE ARM MODE   ");
    }
    else if (ABORT & rocketState){
        lcd.setCursor(0, 1);
        lcd.print("   MISSION ABORT!  ");
    }
    else if (ARMED & rocketState)
    {
        lcd.setCursor(0, 1);
        lcd.print("      ARM MODE     ");
    }
    else if (LAUNCH & rocketState){
        lcd.setCursor(0, 1);
        lcd.print("  ROCKET LAUNCHING! ");
        lcd.setCursor(0, 3);
        lcd.print("Ad Astra Per Aspera!");
    }
}