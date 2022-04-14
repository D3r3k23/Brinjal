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
    delay(2000);

    // bool T1 = brinjal->relay_test_T1();
    int T2 = brinjal->relay_test_T2();

    bool passed = (T2 == HIGH);

    brinjal->open_relay();

    return passed;
}
