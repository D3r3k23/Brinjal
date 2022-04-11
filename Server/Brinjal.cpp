#include "Brinjal.h"

// #include <FunctionalInterrupt.h>
LiquidCrystal_I2C LCD(0x27, 16, 2);

Brinjal::Brinjal()
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

    ledcWrite(CP_DRIVE_pwm, set_cp_duty(chargingCurrent));
   
    pinMode(RED_LED_pin, OUTPUT);
    pinMode(GRN_LED_pin, OUTPUT);
    digitalWrite(RED_LED_pin, LOW);
    digitalWrite(GRN_LED_pin, LOW);

    ledcSetup(BUZ_CTRL_pwm, 1000, PWM_RESOLUTION);
    ledcAttachPin(BUZ_CTRL_pin, BUZ_CTRL_pwm);
    ledcWrite(BUZ_CTRL_pwm, 0);
    
    //LCD
    Wire.begin(41, 42);
    LCD.backlight();
    LCD.clear();
}


void Brinjal::loop()
{
    
    
    if (!in_fault_mode())
    {
        if (gfci_check_fault())
        {
            fault_mode = true;
            open_relay();

            Serial.println("ERROR: FAULT DETECTED");
            LCD.clear();
            stateStr="ERROR....";

            for (int i = 0; i < 5; i++)
            {
                led_toggle(RED_LED);
                buzz();
                delay(100);
            }
            led_on(RED_LED);
        }
        else{
          if (ev_state ==EV_NOT_CONNECTED)
          {
              
              stateStr="EV NOT CONNECTED";
      
            }     
                float cp = read_cp_peak();
                Serial.println("CP: " + String(cp));
            
                update_vehicle_state(ev_state,cp);
                Serial.println("EV state: " + String((int)ev_state));       
        }  
    }

    if (relay_state == RELAY_OPEN)
      {
        led_on(GRN_LED);
      }
    else
    {
        led_off(GRN_LED);
    }
    if (in_fault_mode())
     {
        led_on(RED_LED);
     }
    else{
        led_off(RED_LED);
    }
    delay(500);
     
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    
    previousMillis = currentMillis;
    printDisplayData(); 
    
    }
    printDisplayData(); 
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

        
        
    }
    return peak;
}

void Brinjal::update_vehicle_state(VehicleState oldChargingState,int cp_peak)
{
    
    int ampsPWM = set_cp_duty(chargingCurrent);
    Serial.print("pwm for charging = ");
    Serial.println(ampsPWM);
    
    if (3200 < cp_peak && cp_peak < 3500) 
    {
        ev_state = EV_READY;
    } 
    else if (3600 < cp_peak && cp_peak < 4000)
    {
        ev_state = EV_CONNECTED;
    }
    else if (cp_peak > 4000)    
    {               
        ev_state = EV_NOT_CONNECTED;
    }
    if (!(oldChargingState == ev_state)){
       
       switch (ev_state){
         case EV_READY:
            close_relay();
           
            stateStr="CHARGING ";
            chargingCurr();
            timer();
            break;
            
            
         case EV_CONNECTED:
           
            stateStr="CONNECTED       ";
            chargingCurr();
            open_relay();
            break;
            
        
         case EV_NOT_CONNECTED:
        
              open_relay();
              break;
    }
  }
  
}


VehicleState Brinjal::get_vehicle_state()
{
    return ev_state;
}

bool Brinjal::ready_to_charge()
{
    return ev_state == EV_READY;
}

int Brinjal::set_cp_duty(int ampsToConvert)
{
  
  float pwmsignal = ampsToConvert*4.2333;
  
  return (round(pwmsignal));
  
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
    #ifdef EN_CONTACTOR
    Serial.println("Opening Relay");
    buzz();

    digitalWrite(RELAY_CTRL_pin, LOW);
    relay_state = RELAY_OPEN;
    #endif
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

void Brinjal::printDisplayData()
{
 
  LCD.setCursor(0,0);
  LCD.print(stateStr); // print status on screen
}
void Brinjal::chargingCurr()
{
  LCD.setCursor(0,1);
  LCD.print("Amp:");
  LCD.setCursor(4,1);
  LCD.print(chargingCurrent);
  LCD.setCursor(6,1);
  LCD.print("A");
}
void Brinjal::timer()
{
  LCD.setCursor (8, 1);
  
  int secs = millis () / 1000;
  int mins = secs / 60;
  int hours = mins / 60;
  secs -= mins * 60;
  mins -= hours * 60;
  LCD.printf("%02d:%02d:%02d", hours, mins, secs);
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
