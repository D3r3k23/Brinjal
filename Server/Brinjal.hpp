#ifndef BOARD_HPP
#define BOARD_HPP

#include <Arduino.h>
#include <LiquidCrystal.h>

#define BRINJAL_240V 0

enum VehicleState
{
    EV_UNKNOWN = 0,
    EV_NOT_CONNECTED,
    EV_CONNECTED,
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
    LED_ON,
    LED_FLASHING
};

class Brinjal
{
public:
    Brinjal();
    void begin();

    void loop();

    VehicleState get_vehicle_state();

    void close_relay();
    void open_relay();
    RelayState get_relay_state();
    bool relay_test1();
    bool relay_test2();

    bool gfci_fault();
    void gfci_test_start();
    void gfci_test_end();

    void lcd_display(int line, String text);
    void lcd_display(String line1, String line2);
    void lcd_clear();

    void buzz();
    void buzzer_on();
    void buzzer_off();

    void led_on();
    void led_off();
    void led_toggle();
    void led_flash_slow();
    void led_flash_fast();
    LedState get_led_state();

    bool check_rst_btn();
    bool check_charge_btn();

private:
    static void ARDUINO_ISR_ATTR rst_btn_isr();
    static void ARDUINO_ISR_ATTR charge_btn_isr();
    static void ARDUINO_ISR_ATTR cp_sample_isr();

private:
    VehicleState ev_state = EV_NOT_CONNECTED;
    RelayState relay_state = RELAY_OPEN;

    bool fault = false;

    LedState led_state = LED_OFF;

    LiquidCrystal lcd;

    const int CP_FREQ_hz = 1000;
    const float CP_PERIOD_s = 1.0 / CP_FREQ_hz;

    const int CP_SAMPLES_PER_PERIOD = 4;
    const int CP_SAMPLE_RATE_hz = CP_FREQ_hz * CP_SAMPLES_PER_PERIOD;

    hw_timer_t* cp_sample_timer = nullptr;
    volatile int cp_sample_counter = 0;

    const int PWM_RESOLUTION = 8;

    // PWM channels
    const int CP_DRIVE_pwm  = 0;
    const int LED_FLASH_pwm = 1;
    const int BUZ_CTRL_pwm  = 2;

    // Input pins
    const int RELAY_TEST2_pin = 7;
    const int RELAY_TEST1_pin = 8;
    const int PILOT_MON_pin   = 9;
    const int FAULT_pin       = 10;

    // Output pins
    const int GFCI_TEST_pin  = 11;
    const int CP_DRIVE_pin   = 12;
    const int CP_DISABLE_pin = 13;
    const int RELAY_CTRL_pin = 14;

    // IO header pins
    const int J[12+1] = { -1, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 26 };

    // LCD display pins
    const int LCD_DATA0_pin = J[1];
    const int LCD_DATA1_pin = J[2];
    const int LCD_DATA2_pin = J[3];
    const int LCD_DATA3_pin = J[4];
    const int LCD_RS_pin    = J[5];
    const int LCD_EN_pin    = J[6];

    // Buzzer & LED pins
    const int BUZ_CTRL_pin = J[8];
    const int LED_CTRL_pin = J[9];

    // Button pins
    const int RST_BTN_pin    = J[11];
    const int CHARGE_BTN_pin = J[12];



    // Pilot measurement states
    const int PILOT_NOT_CONNECTED = 511; // 12 V
    const int PILOT_CONNECTED = 383;     // 9 V
    const int PILOT_CHARGE = 255;        // 6 V

    const int PILOT_READ_TOLERANCE = 50;
};

#endif // BOARD_HPP
