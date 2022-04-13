#include "Brinjal.h"

Brinjal::Brinjal()
  : lcd(0x27, 16, 2)
{
    analogReadResolution(12);
}

void Brinjal::begin()
{
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
    ledcWrite(CP_DRIVE_pwm, 0);

    pinMode(RED_LED_pin, OUTPUT);
    pinMode(GRN_LED_pin, OUTPUT);
    digitalWrite(RED_LED_pin, LOW);
    digitalWrite(GRN_LED_pin, LOW);

    ledcSetup(BUZ_CTRL_pwm, 1000, PWM_RESOLUTION);
    ledcAttachPin(BUZ_CTRL_pin, BUZ_CTRL_pwm);
    ledcWrite(BUZ_CTRL_pwm, 0);

    Wire.begin(LCD_SDA_pin, LCD_SCL_pin);
    lcd.init();
    lcd.backlight();
    lcd.noCursor();

    rst();
}

void Brinjal::rst()
{
    open_relay();
    disable_cp();

    if (in_fault_mode())
        exit_fault_mode();

    disable_cp_oscillation();
    enable_cp();

    buzzer_off();
    led_off(RED_LED);
    led_off(GRN_LED);
    lcd_clear();
}

void Brinjal::loop()
{
    if (check_rst_btn())
    {
        rst();
    }
    else if (!in_fault_mode())
    {
        if (gfci_check_fault())
        {
            enter_fault_mode();
        }
        else
        {
            VehicleState prev_vehicle_state = get_vehicle_state();

            float cp = read_cp_peak();
            update_vehicle_state(cp);

            if (get_vehicle_state() != prev_vehicle_state)
            {
                Serial.println("EV state changed to: " + ev_state_to_string(get_vehicle_state()));

                if (prev_vehicle_state == EV_READY && relay_closed())
                    stop_charging();
            }

            if (get_vehicle_state() == EV_READY)
            {
                if (prev_vehicle_state != EV_READY)
                    lcd_display("READY", "");

                if (check_charge_btn())
                    start_charging();
            }
            else
                lcd_clear();
        }
    }
    delay(500);
}

bool Brinjal::ready_to_charge()
{
    return !in_fault_mode() && get_vehicle_state() == EV_READY;
}

bool Brinjal::request_charge()
{
    if (ready_to_charge())
    {
        start_charging();
        return true;
    }
    else
    {
        return false;
    }
}

void Brinjal::start_charging()
{
    close_relay();
    lcd_display("Charging", "");
    led_on(GRN_LED);
}

void Brinjal::stop_charging()
{
    open_relay();
    lcd_display("Charging", "");
    led_off(GRN_LED);
}

/////////////////////////
//        Pilot        //
/////////////////////////

void Brinjal::enable_cp()
{
    digitalWrite(CP_DISABLE_pin, LOW);
}

void Brinjal::disable_cp()
{
    digitalWrite(CP_DISABLE_pin, HIGH);
}

void Brinjal::enable_cp_oscillation()
{
    ledcWrite(CP_DRIVE_pwm, max_current_to_duty_cycle(get_max_current()));
}

void Brinjal::disable_cp_oscillation()
{
    ledcWrite(CP_DRIVE_pwm, 255);
}

int Brinjal::read_cp_peak()
{
    int peak = 0;
    for (int i = 0; i < CP_SAMPLES; i++)
    {
        const int cp_mon_sample = analogRead(CP_MON_pin);

        if (cp_mon_sample > peak)
            peak = cp_mon_sample;
    }
    return peak;
}

void Brinjal::update_vehicle_state(int cp_peak)
{
    if (3200 < cp_peak && cp_peak < 3500)
        ev_state = EV_READY;
    else if (3600 < cp_peak && cp_peak < 4000)
        ev_state = EV_CONNECTED;
    else if (cp_peak > 4000)
        ev_state = EV_NOT_CONNECTED;
}

VehicleState Brinjal::get_vehicle_state()
{
    return ev_state;
}

String Brinjal::ev_state_to_string(VehicleState state)
{
    switch (state)
    {
        case EV_NOT_CONNECTED:
            return "Not Connected";
        case EV_CONNECTED:
            return "Connected";
        case EV_READY:
            return "Charge Ready";
        case EV_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

int Brinjal::get_max_current()
{
#if BRINJAL_240V
    return 40;
#else
    return 16;
#endif
}

int Brinjal::max_current_to_duty_cycle(int current)
{
    return round(current * 4.2333);
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

bool Brinjal::relay_closed()
{
    return relay_state == RELAY_CLOSED;
}

bool Brinjal::relay_open()
{
    return relay_state == RELAY_OPEN;
}

bool Brinjal::relay_test_T1()
{
    return !digitalRead(RELAY_TEST1_pin);
}

bool Brinjal::relay_test_T2()
{
    return !digitalRead(RELAY_TEST2_pin);
}

////////////////////////
//        GFCI        //
////////////////////////

void Brinjal::enter_fault_mode()
{
    open_relay();

    Serial.println("ERROR: FAULT DETECTED");
    lcd_display("FAULT DETECTED", "Reset system");

    fault_mode = true;

    for (int i = 0; i < 5; i++)
    {
        led_toggle(RED_LED);
        buzz();
        delay(100);
    }
    led_on(RED_LED);
}

void Brinjal::exit_fault_mode()
{
    fault_mode = false;
    led_off(RED_LED);
}

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
//        LCD display        //
///////////////////////////////

void Brinjal::lcd_display(int line, String text)
{
    if (!(line == 1 || line == 2))
        line = 1;
    lcd.setCursor(0, line - 1);
    lcd.print(text);
}

void Brinjal::lcd_display(String line1, String line2)
{
    lcd_display(1, line1);
    lcd_display(2, line2);
}

void Brinjal::lcd_clear()
{
    lcd.clear();
}

// void Brinjal::timer()
// {
//     lcd.setCursor (8, 1);

//     int secs = millis () / 1000;
//     int mins = secs / 60;
//     int hours = mins / 60;
//     secs -= mins * 60;  mins -= hours * 60;
//     lcd.printf("%02d:%02d:%02d", hours, mins, secs);
// }

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
