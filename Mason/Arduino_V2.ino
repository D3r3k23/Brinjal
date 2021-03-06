#include <WiFi.h>

const char* ssid     = "Brinjal2";
const char* password = "password";

WiFiServer server(80);

enum VehicleState
{
    EV_UNKNOWN = 0,
    EV_NOT_CONNECTED,
    EV_CONNECTED,
    EV_CHARGE
};

VehicleState ev_state = EV_NOT_CONNECTED;
String relay_state = "off";
bool fault = false;

// Input pins
const int fault_pin = 4;
const int pilot_read_pin = 20;

// Output pins
const int relay_ctrl_pin = 45;
const int fault_mode_pin = 42;
const int pilot_drive_pin = 26;

// Pilot measurement states
const int PILOT_NOT_CONNECTED = 511; // 12 V
const int PILOT_CONNECTED = 383;     // 9 V
const int PILOT_CHARGE = 255;        // 6 V

const int PILOT_READ_TOLERANCE = 50;

int error_state = 0;

void setup()
{
    Serial.begin(115200);
  
    // Inputs
    pinMode(fault_pin, INPUT);
    
    pinMode(pilot_read_pin, INPUT);
    analogReadResolution(9);
  
    // Outputs
    pinMode(relay_ctrl_pin, OUTPUT);
    pinMode(fault_mode_pin, OUTPUT);
    pinMode(pilot_drive_pin, OUTPUT);
    
    digitalWrite(relay_ctrl_pin, LOW);
    digitalWrite(fault_mode_pin, LOW);
    digitalWrite(pilot_drive_pin, LOW);
  
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.softAP(ssid, password);
  
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.begin();
}

void loop()
{
    if (!fault && digitalRead(fault_pin) == HIGH)
    {
        fault = true;
        Serial.println("FAULT DETECTED");
        digitalWrite(fault_mode_pin, HIGH);

        Serial.println("Relay off");
        relay_state = "off";
        digitalWrite(relay_ctrl_pin, LOW);
    }

    int pilot_measurement = analogRead(pilot_read_pin);
    // Serial.print("Pilot: ");
    // Serial.println(pilot_measurement);
    if (pilot_measurement > PILOT_NOT_CONNECTED - PILOT_READ_TOLERANCE) // Not connected
    {
        if (ev_state != EV_NOT_CONNECTED)
            Serial.println("EV disconnected");
        ev_state = EV_NOT_CONNECTED;
    }
    else if (pilot_measurement > PILOT_CONNECTED - PILOT_READ_TOLERANCE && pilot_measurement < PILOT_CONNECTED + PILOT_READ_TOLERANCE) // Connected
    {
        if (ev_state != EV_CONNECTED)
            Serial.println("EV connected");
        ev_state = EV_CONNECTED;
    }
    else if (pilot_measurement > PILOT_CHARGE - PILOT_READ_TOLERANCE && pilot_measurement < PILOT_CHARGE + PILOT_READ_TOLERANCE) // Charge
    {
        if (ev_state != EV_CHARGE)
            Serial.println("EV charge ready");
        ev_state = EV_CHARGE;
    }
    else
    {
        Serial.print("Unknown pilot state: ADC=");
        Serial.println(pilot_measurement);
        ev_state = EV_UNKNOWN;
    }
    
    WiFiClient client = server.available();   // Listen for incoming clients
    
    if (client)                               // If a new client connects
    {
        Serial.println("Client Connected");     // print a message out in the serial port
        String header;
        String currentLine = "";                // make a String to hold incoming data from the client
        while (client.connected())              // loop while the client's connected
        {
            if (client.available())               // if there's bytes to read from the client
            {
                char c = client.read();             // read a byte, then
                Serial.write(c);                    // print it out the serial monitor
                header += c;
                if (c == '\n')                      // if the byte is a newline character
                {
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();
                        
                        // turns the GPIOs on and off
                        if (header.indexOf("GET /relay/on") >= 0)
                        {
                            if (fault)
                                Serial.println("Error: System is in fault mode");
                            else
                            {
                               // Serial.println("Relay on");
                               // relay_state = "on";
                                digitalWrite(relay_ctrl_pin, HIGH);
                            }
                        }
                        else if (header.indexOf("GET /relay/off") >= 0)
                        {
                            Serial.println("Relay off");
                            relay_state = "off";
                            digitalWrite(relay_ctrl_pin, LOW);
                        }
                        
                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        // CSS to style the on/off buttons 
                        // Feel free to change the background-color and font-size attributes to fit your preferences
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");
                        
                        // Web Page Heading
                        client.println("<body><h1>Brinjal</h1>");
                        
                        // Display current state, and ON/OFF buttons
                        client.println("<p>Relay state: " + relay_state + "</p>");


                        //Get Error States
                        if(ev_state == EV_UNKNOWN)
                        {
                          if(relay_state == "off")
                          {
                            relay_state = "on";
                            error_state = 1;
                          }
                        }
                        if(ev_state == EV_NOT_CONNECTED)
                        {
                          if(relay_state == "off")
                          {
                            relay_state = "on";
                            error_state = 2;
                          }
                        }
                        
                        
                        // If the relay is off, it displays the ON button
                        if (relay_state == "off")
                        {
                            client.println("<p><a href=\"/relay/on\"><button class=\"button\">ON</button></a></p>");
                        }
                        else
                        {
                            client.println("<p><a href=\"/relay/off\"><button class=\"button button2\">OFF</button></a></p>");
                        }

                        
                        //Display error messages
                        switch(error_state){
                          case 1:
                            client.println("<font style='color:red'>");
                            client.println("<p>EV State Unknown</p>");
                            break;
                          case 2:
                            client.println("<font style='color:red'>");
                            client.println("<p>EV Not Connected</p>");
                            break;
                        }
                        if(ev_state == EV_CONNECTED)
                        {
                          client.println("<font style='color:green'>");
                          client.println("<p>EV Connected</p>");
                        }
                        if(ev_state == EV_CHARGE)
                        {
                          client.println("<font style='color:green'>");
                          client.println("<p>EV Connected</p>");
                        }
                        
                        
                        client.println("</body></html>");
                        
                        // The HTTP response ends with another blank line
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else // if you got a newline, then clear currentLine
                    {
                        currentLine = "";
                    }
                }
                else if (c != '\r') // if you got anything else but a carriage return character
                {
                    currentLine += c;
                }
            }
        }
        // Close the connection
        client.stop();
        Serial.println("Client disconnected");
        Serial.println("");
    }
    delay(200);
}
