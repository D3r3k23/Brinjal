#include "Brinjal.hpp"

// #include <FunctionalInterrupt.h>

Brinjal::Brinjal()
//  : lcd(LCD_RS_pin, LCD_EN_pin, LCD_DATA0_pin, LCD_DATA1_pin, LCD_DATA2_pin, LCD_DATA3_pin)
{

}

void Brinjal::begin()
{
    analogReadResolution(12);

    // Inputs

    pinMode(RELAY_TEST1_pin, INPUT);
    pinMode(RELAY_TEST2_pin, INPUT);
    pinMode(CP_MON_pin, INPUT);
    pinMode(FAULT_pin, INPUT);

    pinMode(RST_BTN_pin, INPUT);
    pinMode(CHARGE_BTN_pin, INPUT);

    attachInterrupt(RST_BTN_pin, rst_btn_isr, RISING);
    attachInterrupt(CHARGE_BTN_pin, charge_btn_isr, RISING);

    // Outputs

    pinMode(GFCI_TEST_pin, OUTPUT);
    pinMode(CP_DISABLE_pin, OUTPUT);
    pinMode(RELAY_CTRL_pin, OUTPUT);
    digitalWrite(GFCI_TEST_pin, LOW);
    digitalWrite(CP_DISABLE_pin, LOW);
    digitalWrite(RELAY_CTRL_pin, LOW);

    ledcSetup(CP_DRIVE_pwm, CP_FREQ, PWM_RESOLUTION);
    ledcAttachPin(CP_DRIVE_pin, CP_DRIVE_pwm);

    ledcWrite(CP_DRIVE_pwm, 127);

    pinMode(RED_LED_pin, OUTPUT);
    pinMode(GRN_LED_pin, OUTPUT);
    digitalWrite(RED_LED_pin, LOW);
    digitalWrite(GRN_LED_pin, LOW);

    ledcSetup(BUZ_CTRL_pwm, 1000, PWM_RESOLUTION);
    ledcAttachPin(BUZ_CTRL_pin, BUZ_CTRL_pwm);
    ledcWrite(BUZ_CTRL_pwm, 0);
}

void Brinjal::loop()
{
    // Check buttons



    if (!in_fault_mode())
    {
        if (gfci_check_fault())
        {
            fault_mode = true;
            open_relay();

            Serial.println("ERROR: FAULT DETECTED");
            lcd_display("FAULT DETECTED", "Reset system");

            for (int i = 0; i < 5; i++)
            {
                led_toggle(RED_LED);
                buzz();
                delay(100);
            }
            led_on(RED_LED);
        }
        else
        {
            if (ev_state == EV_CHARGE)
                close_relay();
            else
                open_relay();

            float cp = read_cp_peak();
            Serial.println("CP: " + String(cp));

            update_vehicle_state(cp);
            Serial.println("EV state: " + String((int)ev_state));
        }
    }

    if (relay_state == RELAY_OPEN)
        led_on(GRN_LED);
    else
        led_off(GRN_LED);

    if (in_fault_mode())
        led_on(RED_LED);
    else
        led_off(RED_LED);

    delay(500);
}

/////////////////////////
//        Pilot        //
/////////////////////////

int Brinjal::read_cp_peak()
{
    int peak = 0;
    for (int i = 0; i < CP_SAMPLES; i++)
    {
        const int cp_mon_sample = analogRead(CP_MON_pin);

        if (cp_mon_sample > peak)
            peak = cp_mon_sample;

        delay(10);
    }
    return peak;
}

void Brinjal::update_vehicle_state(int cp_peak)
{
    if (3200 < cp_peak && cp_peak < 3500) ev_state = EV_CHARGE;
    if (3600 < cp_peak && cp_peak < 4000) ev_state = EV_READY;
    if (cp_peak > 4000)                   ev_state = EV_NOT_CONNECTED;
}

VehicleState Brinjal::get_vehicle_state()
{
    return ev_state;
}

bool Brinjal::ready_to_charge()
{
    return ev_state == EV_READY;
}

/////////////////////////
//        Relay        //
/////////////////////////

void Brinjal::close_relay()
{
    Serial.println("Closing Relay");
    buzz();
    delay(100);
    buzz();
    delay(400);

    digitalWrite(RELAY_CTRL_pin, HIGH);
    relay_state = RELAY_CLOSED;
}

void Brinjal::open_relay()
{
    Serial.println("Opening Relay");
    buzz();

    digitalWrite(RELAY_CTRL_pin, LOW);
    relay_state = RELAY_OPEN;
}

RelayState Brinjal::get_relay_state()
{
    return relay_state;
}

bool Brinjal::relay_test1()
{
    return digitalRead(RELAY_TEST1_pin);
}

bool Brinjal::relay_test2()
{
    return digitalRead(RELAY_TEST2_pin);
}

////////////////////////
//        GFCI        //
////////////////////////

bool Brinjal::in_fault_mode()
{
    return fault_mode;
}

bool Brinjal::gfci_check_fault()
{
    return digitalRead(FAULT_pin);
}

void Brinjal::gfci_test_start()
{
    digitalWrite(GFCI_TEST_pin, HIGH);
}

void Brinjal::gfci_test_end()
{
    digitalWrite(GFCI_TEST_pin, LOW);
}

///////////////////////////////
//        LCD Display        //
///////////////////////////////

void Brinjal::lcd_display(int line, String text)
{

}

void Brinjal::lcd_display(String line1, String line2)
{

}

void Brinjal::lcd_clear()
{

}

//////////////////////////
//        Buzzer        //
//////////////////////////

static const int BUZZER_FREQ = 1000;

void Brinjal::buzz()
{
    static const int BUZZ_DURATION_ms = 50;

    // tone(BUZ_CTRL_pin, BUZZER_FREQ, BUZZ_DURATION_ms);

    buzzer_on();
    delay(BUZZ_DURATION_ms);
    buzzer_off();
}

void Brinjal::buzzer_on()
{
    static const note_t BUZZER_NOTE = NOTE_E;
    static const int BUZZER_OCTAVE = 1;

    ledcWriteNote(BUZ_CTRL_pwm, BUZZER_NOTE, BUZZER_OCTAVE);

    // ledcWriteTone(BUZ_CTRL_pwm, BUZZER_FREQ_hz);
}

void Brinjal::buzzer_off()
{
    ledcWriteTone(BUZ_CTRL_pwm, 0);
}

///////////////////////
//        LED        //
///////////////////////

void Brinjal::led_on(LedColor color)
{
    const int led_pin = (color == RED_LED) ? RED_LED_pin : GRN_LED_pin;
    digitalWrite(led_pin, HIGH);

    LedState* led_state = (color == RED_LED) ? &red_led_state : &grn_led_state;
    *led_state = LED_ON;
}

void Brinjal::led_off(LedColor color)
{
    const int led_pin = (color == RED_LED) ? RED_LED_pin : GRN_LED_pin;
    digitalWrite(led_pin, LOW);

    LedState* led_state = (color == RED_LED) ? &red_led_state : &grn_led_state;
    *led_state = LED_OFF;
}

void Brinjal::led_toggle(LedColor color)
{
    LedState state = (color == RED_LED) ? red_led_state : grn_led_state;

    if (state == LED_OFF)
        led_on(color);
    else
        led_off(color);
}

LedState Brinjal::get_led_state(LedColor color)
{
    return (color == RED_LED) ? red_led_state : grn_led_state;
}

///////////////////////////
//        Buttons        //
///////////////////////////

static volatile bool rst_btn_pressed = false;
static volatile bool charge_btn_pressed = false;

bool Brinjal::check_rst_btn()
{
    bool rst = rst_btn_pressed;
    rst_btn_pressed = false;
    return rst;
}

bool Brinjal::check_charge_btn()
{
    bool charge = charge_btn_pressed;
    charge_btn_pressed = false;
    return charge;
}

// ISRs

void ARDUINO_ISR_ATTR Brinjal::rst_btn_isr()
{
    Serial.println("RST BTN interrupt");
    rst_btn_pressed = true;
}

void ARDUINO_ISR_ATTR Brinjal::charge_btn_isr()
{
    Serial.println("CHARGE BTN interrupt");
    charge_btn_pressed = true;
}
