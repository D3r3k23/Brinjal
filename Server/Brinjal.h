#ifndef BRINJAL_H
#define BRINJAL_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

enum EVSU_State
{
    EVSU_UNKNOWN = 0,
    EVSU_IDLE,
    EVSU_READY,
    EVSU_CHARGING
};

String evsu_state_to_string(EVSU_State state);

enum RelayState
{
    RELAY_OPEN = 0,
    RELAY_CLOSED
};

enum LedState
{
    LED_OFF = 0,
    LED_ON
};

enum LedColor
{
    RED_LED,
    GRN_LED
};

enum VehicleState
{
    EV_UNKNOWN = 0,
    EV_NOT_CONNECTED,
    EV_CONNECTED,
    EV_READY
};

String ev_state_to_string(VehicleState state);

/*
|     State     | CP_MON | ADC  |
|---------------|--------|------|
| Not Connected |  2.7   | 4096 |
|   Connected   |  2.3   | 3850 |
|     Ready     |   2    | 3340 |
*/

class Brinjal
{
public:
    Brinjal();
    void begin();
    void reset();
    void loop();

    bool ready_to_charge();
    bool request_charge();
    void start_charging();
    void stop_charging();
    EVSU_State get_evsu_state();

    // Pilot
    void enable_cp();
    void disable_cp();
    void enable_cp_oscillation();
    void disable_cp_oscillation();
    int read_cp_peak();
    void update_vehicle_state(int cp_peak);
    VehicleState get_vehicle_state();
    int get_max_current();
    int max_current_to_duty_cycle(int current);
    void set_max_current(int current);

    // Relay
    void close_relay();
    void open_relay();
    RelayState get_relay_state();
    bool relay_closed();
    bool relay_open();
    bool relay_test_T1();
    bool relay_test_T2();

    // GFCI
    void enter_fault_mode();
    void exit_fault_mode();
    bool in_fault_mode();
    bool gfci_check_fault();
    void gfci_test_start();
    void gfci_test_end();

    // LCD display
    void lcd_display(int line, String text);
    void lcd_display(String line1, String line2);
    void lcd_clear();

    // Buzzer
    void buzz();
    void buzzer_on();
    void buzzer_off();

    // LEDs
    void led_on(LedColor color);
    void led_off(LedColor color);
    void led_toggle(LedColor color);
    LedState get_led_state(LedColor color);

    // Buttons
    bool check_rst_btn();
    bool check_charge_btn();
    static void IRAM_ATTR rst_btn_isr();
    static void IRAM_ATTR charge_btn_isr();

public:
    const int PWM_RESOLUTION = 8;

    const int CP_SAMPLES = 50;

    const int CP_FREQ = 1000;
    const float CP_PERIOD = 1.0 / CP_FREQ;

    EVSU_State evsu_state = EVSU_IDLE;
    VehicleState ev_state = EV_NOT_CONNECTED;
    RelayState relay_state = RELAY_OPEN;

    bool fault_mode = false;

    LedState red_led_state = LED_OFF;
    LedState grn_led_state = LED_OFF;

    LiquidCrystal_I2C lcd;

    // PWM channels
    const int CP_DRIVE_pwm  = 0;
    const int BUZ_CTRL_pwm  = 2;

    // Input pins
    const int RELAY_TEST2_pin = 7;
    const int RELAY_TEST1_pin = 8;
    const int CP_MON_pin      = 9;
    const int FAULT_pin       = 10;

    // Output pins
    const int GFCI_TEST_pin  = 11;
    const int CP_DRIVE_pin   = 12;
    const int CP_DISABLE_pin = 13;
    const int RELAY_CTRL_pin = 14;

    // IO header pins
    // Using GPIO43 (UART0_TX) interferes with serial monitor
    const int J[12+1] = { -1, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 26 };

    // LCD display pins
    const int LCD_SDA_pin = J[12];
    const int LCD_SCL_pin = J[11];

    // Buzzer pin
    const int BUZ_CTRL_pin = J[10];

    // Button pins
    const int CHARGE_BTN_pin = J[9];
    const int RST_BTN_pin    = J[8];

    // LED pins
    const int RED_LED_pin = J[7];
    const int GRN_LED_pin = J[6];
};

#endif // BRINJAL_H
