#ifndef BRINJAL_HPP
#define BRINJAL_HPP

#include <Arduino.h>

//////////////////////
#define BRINJAL_240V 0
//////////////////////

enum VehicleState
{
    EV_UNKNOWN = 0,
    EV_NOT_CONNECTED,
    EV_READY,
    EV_CHARGE
};

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

/*
|      State      | CP_MON | ADC  |
|-----------------|--------|------|
|  Not Connected  |  2.7   | 4096 |
| Connected/Ready |  2.3   | 3850 |
|    Charging     |   2    | 3340 |
*/

class Brinjal
{
public:
    Brinjal();
    void begin();
    void loop();

    int read_cp_peak();
    void update_vehicle_state(int cp_peak);
    VehicleState get_vehicle_state();
    bool ready_to_charge();

    void close_relay();
    void open_relay();
    RelayState get_relay_state();
    bool relay_test1();
    bool relay_test2();

    bool in_fault_mode();
    bool gfci_check_fault();
    void gfci_test_start();
    void gfci_test_end();

    void lcd_display(int line, String text);
    void lcd_display(String line1, String line2);
    void lcd_clear();

    void buzz();
    void buzzer_on();
    void buzzer_off();

    void led_on(LedColor color);
    void led_off(LedColor color);
    void led_toggle(LedColor color);
    LedState get_led_state(LedColor color);

    bool check_rst_btn();
    bool check_charge_btn();

private:
    static void ARDUINO_ISR_ATTR rst_btn_isr();
    static void ARDUINO_ISR_ATTR charge_btn_isr();

private:
    const int PWM_RESOLUTION = 8;

    const int CP_SAMPLES = 25;

    const int CP_FREQ = 1000;
    const float CP_PERIOD = 1.0 / CP_FREQ;

    VehicleState ev_state = EV_NOT_CONNECTED;
    RelayState relay_state = RELAY_OPEN;

    bool fault_mode = false;

    LedState red_led_state = LED_OFF;
    LedState grn_led_state = LED_OFF;

    // LiquidCrystal lcd;

    // PWM channels
    const int CP_DRIVE_pwm  = 0;
    const int LED_FLASH_pwm = 1;
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
    const int J[12+1] = { -1, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 26 };

    // LCD display pins
    const int LCD_SDA_pin = J[1];
    const int LCD_SCL_pin = J[2];

    // Buzzer pin
    const int BUZ_CTRL_pin = J[4];

    // LED pins
    const int RED_LED_pin = J[6];
    const int GRN_LED_pin = J[7];

    // Button pins
    const int RST_BTN_pin    = J[9];
    const int CHARGE_BTN_pin = J[10];
};

#endif // BRINJAL_HPP
