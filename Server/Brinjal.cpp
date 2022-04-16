#include "Brinjal.h"

String evsu_state_to_string(EVSU_State state)
{
    switch (state)
    {
        case EVSU_IDLE:
            return "Idle";
        case EVSU_READY:
            return "Ready";
        case EVSU_CHARGING:
            return "Charging";
        case EVSU_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

String ev_state_to_string(VehicleState state)
{
    switch (state)
    {
        case EV_NOT_CONNECTED:
            return "Not Connected";
        case EV_CONNECTED:
            return "Connected";
        case EV_READY:
            return "Ready";
        case EV_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

///////////////////////////
//        Brinjal        //
///////////////////////////

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

    attachInterrupt(digitalPinToInterrupt(RST_BTN_pin), rst_btn_isr, RISING);
    attachInterrupt(digitalPinToInterrupt(CHARGE_BTN_pin), charge_btn_isr, RISING);

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

    reset();
}

void Brinjal::reset()
{
    if (relay_closed())
        open_relay();

    if (in_fault_mode())
        exit_fault_mode();

    evsu_state = EVSU_IDLE;
    ev_state = EV_NOT_CONNECTED;

    disable_cp_oscillation();
    enable_cp();

    buzzer_off();
    led_off(RED_LED);
    led_off(GRN_LED);
    lcd_clear();
}

void Brinjal::loop()
{
    bool rst_btn_pressed = check_rst_btn();
    bool charge_btn_pressed = check_charge_btn();

    if (rst_btn_pressed)
        Serial.println("Rst btn pressed");
    if (charge_btn_pressed)
        Serial.println("Charge btn pressed");
    if (gfci_check_fault())
        Serial.println("Fault read!!");

    if (rst_btn_pressed)
    {
        Serial.println("Resetting system");
        reset();
    }
    else if (!in_fault_mode())
    {
        if (gfci_check_fault())
        {
            Serial.println("ERROR: GROUND FAULT DETECTED");
            lcd_display("FAULT DETECTED", "RESET SYSTEM");
            enter_fault_mode();
        }
        else if (!relay_test())
        {
            Serial.println("ERROR: RELAY FAULT DETECTED");
            lcd_display("FAULT DETECTED", "UNPLUG POWER");
            enter_fault_mode();
        }
        else
        {
            VehicleState prev_vehicle_state = get_vehicle_state();

            float cp = read_cp_peak();
            update_vehicle_state(cp);

            if (get_vehicle_state() != prev_vehicle_state) // New EV state
            {
                Serial.println("EV state: " + ev_state_to_string(get_vehicle_state()));

                if (get_evsu_state() == EVSU_CHARGING) // EV disconnected/unavailable
                    stop_charging();

                if (get_vehicle_state() == EV_READY)
                    lcd_display("READY", "");
                else if (get_evsu_state() == EVSU_CHARGING)
                {
                    lcd_display("CHARGE COMPLETE", "");
                }
                else
                    lcd_clear();

                switch (get_vehicle_state())
                {
                    case EV_NOT_CONNECTED:
                        disable_cp_oscillation();
                        break;
                    case EV_CONNECTED:
                        enable_cp_oscillation();
                        break;
                    case EV_READY:
                        evsu_state = EVSU_READY;
                        break;
                    default:
                        evsu_state = EVSU_IDLE;
                        break;
                }
            }
            if (charge_btn_pressed)
            {
                if (get_evsu_state() == EVSU_READY)
                    start_charging();
                else if (get_evsu_state() == EVSU_CHARGING)
                    stop_charging();
            }
        }
    }
    delay(100);
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
    lcd_display("CHARGING", "");
    led_on(GRN_LED);
    evsu_state = EVSU_CHARGING;

    s_charge_start_time = millis();
}

void Brinjal::stop_charging()
{
    if (!in_fault_mode() && millis() - s_charge_start_time > 1000)
    {
        open_relay();
        lcd_clear();
        led_off(GRN_LED);
        evsu_state = get_vehicle_state() == EV_READY ? EVSU_READY : EVSU_IDLE;
        if (evsu_state == EVSU_READY)
            lcd_display("READY", "");
    }
}

EVSU_State Brinjal::get_evsu_state()
{
    return evsu_state;
}

void Brinjal::enter_fault_mode()
{
    open_relay();

    fault_mode = true;

    disable_cp();

    for (int i = 0; i < 10; i++)
    {
        led_toggle(RED_LED);
        buzz();
        delay(200);
    }
    led_on(RED_LED);
}

void Brinjal::exit_fault_mode()
{
    fault_mode = false;
    led_off(RED_LED);
    enable_cp();
}

bool Brinjal::in_fault_mode()
{
    return fault_mode;
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
    return 127;
    return round(current * 4.2333);
}

/////////////////////////
//        Relay        //
/////////////////////////

void Brinjal::close_relay()
{
    Serial.println("Closing Relay");
    for (int i = 0; i < 3; i++)
    {
        buzz();
        delay(100);
    }
    delay(200);

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

bool Brinjal::relay_test()
{
    delay(50);
    bool T2 = !digitalRead(RELAY_TEST2_pin);

    if (relay_closed())
        return T2;
    else
        return !T2;
}

////////////////////////
//        GFCI        //
////////////////////////

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
    lcd_clear();
    lcd_display(1, line1);
    lcd_display(2, line2);
}

void Brinjal::lcd_clear()
{
    lcd.clear();
}

//////////////////////////
//        Buzzer        //
//////////////////////////

static const int BUZZER_FREQ = 1000;

void Brinjal::buzz()
{
    static const int BUZZ_DURATION_ms = 50;

    buzzer_on();
    delay(BUZZ_DURATION_ms);
    buzzer_off();
}

void Brinjal::buzzer_on()
{
    static const note_t BUZZER_NOTE = NOTE_E;
    static const int BUZZER_OCTAVE = 1;

    ledcWriteNote(BUZ_CTRL_pwm, BUZZER_NOTE, BUZZER_OCTAVE);
}

void Brinjal::buzzer_off()
{
    ledcWriteTone(BUZ_CTRL_pwm, 0);
}

////////////////////////
//        LEDs        //
////////////////////////

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

static volatile bool s_rst_btn_was_pressed = false;
static volatile bool s_charge_btn_was_pressed = false;

static unsigned long s_last_btn_interrupt = 0;

bool Brinjal::check_rst_btn()
{
    bool rst = s_rst_btn_was_pressed;
    s_rst_btn_was_pressed = false;
    return rst;
}

bool Brinjal::check_charge_btn()
{
    bool charge = s_charge_btn_was_pressed;
    s_charge_btn_was_pressed = false;
    return charge;
}

void IRAM_ATTR Brinjal::rst_btn_isr()
{
    unsigned long interrupt_time = millis();
    if (interrupt_time - s_last_btn_interrupt > 1000)
    {
        s_last_btn_interrupt = interrupt_time;
        s_rst_btn_was_pressed = true;
    }
}

void IRAM_ATTR Brinjal::charge_btn_isr()
{
    unsigned long interrupt_time = millis();
    if (interrupt_time - s_last_btn_interrupt > 1000)
    {
        s_last_btn_interrupt = interrupt_time;
        s_charge_btn_was_pressed = true;
    }
}
