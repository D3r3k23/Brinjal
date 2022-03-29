#include "Brinjal.hpp"

Brinjal::Brinjal()
{
    // Inputs

    pinMode(RELAY_TEST1_pin, INPUT);
    pinMode(RELAY_TEST2_pin, INPUT);
    pinMode(PILOT_MON_pin, INPUT);
    pinMode(FAULT_pin, INPUT);

    // Outputs

    pinMode(GFCI_TEST_pin, OUTPUT);
    pinMode(CP_DISABLE_pin, OUTPUT);
    pinMode(RELAY_CTRL_pin, OUTPUT);

    digitalWrite(GFCI_TEST_pin, LOW);
    digitalWrite(CP_DISABLE_pin, LOW);
    digitalWrite(RELAY_CTRL_pin, LOW);

    ledcSetup(CP_DRIVE_pwm, 1000, 8);
    ledcAttachPin(CP_DRIVE_pin, CP_DRIVE_pwm);
}

void IRAM_ATTR Brinjal::cp_sample_isr()
{
    portENTER_CRITICAL_ISR(&timer_mux);



    portEXIT_CRITICAL_ISR(&timer_mux);
}
