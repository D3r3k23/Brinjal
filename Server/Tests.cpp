#include "Tests.h"

#include <Arduino.h>

bool Tests::gfci()
{
    Serial.println("Running GFCI test");

    brinjal->gfci_test_start();
    delay(50);
    bool passed = brinjal->gfci_check_fault();

    if (!passed)
        Serial.println("GFCI test failed");

    brinjal->gfci_test_end();
    return passed;
}

// Call after closing relay
static bool test_relay(bool relay_output, String Tname)
{
    if (relay_output)
        return true;
    else
    {
        Serial.println("Relay test failed: " + Tname);
        return false;
    }
}

#define TEST_RELAY(RELAY_OUTPUT) test_relay(RELAY_OUTPUT, #RELAY_OUTPUT)

bool Tests::relay()
{
    Serial.println("Running Relay test");
    brinjal->lcd_display("CAUTION:", "Testing Relay");

    brinjal->buzz();
    delay(1000);
    brinjal->buzz();
    delay(200);
    brinjal->buzz();
    delay(200);

    brinjal->close_relay();
    delay(4000);

    bool T1 = brinjal->relay_test_T1();
#if BRINJAL_240V
    bool T2 = brinjal->relay_test_T2();
#endif

    bool passed = TEST_RELAY(T1);
    if (passed)
    {
    #if BRINJAL_240V
        passed = TEST_RELAY(T2);
    #endif
    }
    brinjal->open_relay();

    return passed;
}
