#include "Brinjal.hpp"

#include <FunctionalInterrupt.h>

Brinjal::Brinjal()
  : lcd(LCD_RS_pin, LCD_EN_pin, LCD_DATA0_pin, LCD_DATA1_pin, LCD_DATA2_pin, LCD_DATA3_pin)
{ }

void Brinjal::begin()
{
    // Inputs

    pinMode(RELAY_TEST1_pin, INPUT);
    pinMode(RELAY_TEST2_pin, INPUT);
    pinMode(PILOT_MON_pin, INPUT);
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

    ledcSetup(CP_DRIVE_pwm, CP_FREQ_hz, PWM_RESOLUTION);
    ledcAttachPin(CP_DRIVE_pin, CP_DRIVE_pwm);

    ledcWrite(CP_DRIVE_pwm, 127);

    ledcSetup(LED_FLASH_pwm, 1, PWM_RESOLUTION);
    ledcAttachPin(LED_CTRL_pin, LED_FLASH_pwm);
    ledcWrite(LED_FLASH_pwm, 0);

    ledcSetup(BUZ_CTRL_pwm, 1000, PWM_RESOLUTION);
    ledcAttachPin(BUZ_CTRL_pin, BUZ_CTRL_pwm);
    ledcWrite(BUZ_CTRL_pwm, 0);
}

void Brinjal::loop()
{

}

VehicleState Brinjal::get_vehicle_state()
{
    return ev_state;
}

/////////////////////////
//        Relay        //
/////////////////////////

void Brinjal::close_relay()
{
    Serial.println("Closing Relay");
    buzz();
    delay(200);
    buzz();
    delay(200);

    digitalWrite(RELAY_CTRL_pin, HIGH);
    relay_state = RELAY_CLOSED;
}

void Brinjal::open_relay()
{
    Serial.println("Opening Relay");
    buzz();
    delay(100);

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

bool Brinjal::gfci_fault()
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

static const int BUZZER_FREQ_hz = 1000;

void Brinjal::buzz()
{
    static const int BUZZ_DURATION_ms = 50;

    // tone(BUZ_CTRL_pin, BUZZER_FREQ_hz, BUZZ_DURATION_ms);

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
    ledcWriteTone(BUZ_CTRL_pin, 0);
}

///////////////////////
//        LED        //
///////////////////////

void Brinjal::led_on()
{
    ledcWrite(LED_FLASH_pwm, 255);
    led_state = LED_ON;
}

void Brinjal::led_off()
{
    ledcWrite(LED_FLASH_pwm, 0);
    led_state = LED_OFF;
}

void Brinjal::led_toggle()
{
    if (!led_state)
        led_on();
    else
        led_off();
}

void Brinjal::led_flash_slow()
{
    ledcChangeFrequency(LED_FLASH_pwm, 1, PWM_RESOLUTION);
    ledcWrite(LED_FLASH_pwm, 127);
    led_state = LED_FLASHING;
}

void Brinjal::led_flash_fast()
{
    ledcChangeFrequency(LED_FLASH_pwm, 4, PWM_RESOLUTION);
    ledcWrite(LED_FLASH_pwm, 127);
    led_state = LED_FLASHING;
}

LedState Brinjal::get_led_state()
{
    return led_state;
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
    rst_btn_pressed = true;
}

void ARDUINO_ISR_ATTR Brinjal::charge_btn_isr()
{
    charge_btn_pressed = true;
}

// CP sampling timer ISR

static portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;

void ARDUINO_ISR_ATTR Brinjal::cp_sample_isr()
{
    portENTER_CRITICAL_ISR(&timer_mux);



    portEXIT_CRITICAL_ISR(&timer_mux);
}
