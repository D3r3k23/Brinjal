#include "Tests.hpp"

bool Tests::pilot()
{
    Serial.println("Testing control control pilot measurement");
    return true;
}

bool Tests::gfci()
{
    Serial.println("Running GFCI test");

    brinjal->gfci_test_start();
    // Delay?
    bool passed = brinjal->gfci_fault();

    if (!passed)
        Serial.println("GFCI test failed");

    brinjal->gfci_test_end();
    return passed;
}

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
    brinjal->lcd_display("Caution:", "Testing relay");

    brinjal->buzz();
    delay(1000);
    brinjal->buzz();
    delay(250);
    brinjal->buzz();
    delay(500);

    brinjal->close_relay();

    bool T1 = brinjal->relay_test1();
#if BRINJAL_240V
    bool T2 = brinjal->relay_test2();
#endif

    bool passed = TEST_RELAY(T1);
    if (passed)
    {
    #if BRINJAL_240V
        passed = TEST_RELAY(T2);
    #endif
    }
    brinjal->open_relay();
    brinjal->lcd_clear();

    return passed;
}
