#include "Brinjal.hpp"
#include "Tests.hpp"

#include <WiFi.h>

Brinjal brinjal;

const char* ssid     = "Brinjal";
const char* password = "password123";

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");

    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    server.begin();

    ledcWrite(CP_DRIVE_pwm, 127);
}

void loop()
{
    if (digitalRead(FAULT_pin) == HIGH)
    {
        Serial.println("FAULT");
    }
    digitalWrite(GFCI_TEST_pin, HIGH);
    if (digitalRead(FAULT_pin) != HIGH)
    {
        Serial.println("GFCI_TEST failed");
    }
    digitalWrite(GFCI_TEST_pin, LOW);
    delay(500);
}

//void loop()
//{
//    if (!fault && digitalRead(fault_pin) == HIGH)
//    {
//        fault = true;
//        Serial.println("FAULT DETECTED");
//        digitalWrite(fault_mode_pin, HIGH);
//
//        Serial.println("Relay off");
//        relay_state = "off";
//        digitalWrite(relay_ctrl_pin, LOW);
//    }
//
//    int pilot_measurement = analogRead(pilot_read_pin);
//    // Serial.print("Pilot: ");
//    // Serial.println(pilot_measurement);
//    if (pilot_measurement > PILOT_NOT_CONNECTED - PILOT_READ_TOLERANCE) // Not connected
//    {
//        if (ev_state != EV_NOT_CONNECTED)
//            Serial.println("EV disconnected");
//        ev_state = EV_NOT_CONNECTED;
//        relay_state = "off";
//        digitalWrite(relay_ctrl_pin, LOW);
//    }
//    else if (pilot_measurement > PILOT_CONNECTED - PILOT_READ_TOLERANCE && pilot_measurement < PILOT_CONNECTED + PILOT_READ_TOLERANCE) // Connected
//    {
//        if (ev_state != EV_CONNECTED)
//            Serial.println("EV connected");
//        ev_state = EV_CONNECTED;
//    }
//    else if (pilot_measurement > PILOT_CHARGE - PILOT_READ_TOLERANCE && pilot_measurement < PILOT_CHARGE + PILOT_READ_TOLERANCE) // Charge
//    {
//        if (ev_state != EV_CHARGE)
//            Serial.println("EV charge ready");
//        ev_state = EV_CHARGE;
//    }
//    else
//    {
//        Serial.print("Unknown pilot state: ADC=");
//        Serial.println(pilot_measurement);
//        ev_state = EV_UNKNOWN;
//    }
//
//    WiFiClient client = server.available();   // Listen for incoming clients
//
//    if (client)                               // If a new client connects
//    {
//        Serial.println("Client Connected");     // print a message out in the serial port
//        String header;
//        String currentLine = "";                // make a String to hold incoming data from the client
//        while (client.connected())              // loop while the client's connected
//        {
//            if (client.available())               // if there's bytes to read from the client
//            {
//                char c = client.read();             // read a byte, then
//                Serial.write(c);                    // print it out the serial monitor
//                header += c;
//                if (c == '\n')                      // if the byte is a newline character
//                {
//                    // if the current line is blank, you got two newline characters in a row.
//                    // that's the end of the client HTTP request, so send a response:
//                    if (currentLine.length() == 0)
//                    {
//                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//                        // and a content-type so the client knows what's coming, then a blank line:
//                        client.println("HTTP/1.1 200 OK");
//                        client.println("Content-type:text/html");
//                        client.println("Connection: close");
//                        client.println();
//
//                        // turns the GPIOs on and off
//                        if (header.indexOf("GET /relay/on") >= 0)
//                        {
//                            if (fault)
//                                Serial.println("Error: System is in fault mode");
//                            else if (ev_state != EV_CONNECTED) // EV_CHARGE
//                                Serial.println("Error: EV is not ready to charge");
//                            else
//                            {
//                                Serial.println("Relay on");
//                                relay_state = "on";
//                                digitalWrite(relay_ctrl_pin, HIGH);
//                            }
//                        }
//                        else if (header.indexOf("GET /relay/off") >= 0)
//                        {
//                            Serial.println("Relay off");
//                            relay_state = "off";
//                            digitalWrite(relay_ctrl_pin, LOW);
//                        }
//
//                        // Display the HTML web page
//                        client.println("<!DOCTYPE html><html>");
//                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
//                        client.println("<link rel=\"icon\" href=\"data:,\">");
//
//                        // CSS to style the on/off buttons
//                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
//                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
//                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
//                        client.println(".button2 {background-color: #555555;}</style></head>");
//
//                        // Web Page Heading
//                        client.println("<body><h1>Brinjal</h1>");
//
//                        // Display current state, and ON/OFF buttons
//                        client.println("<p>Relay state: " + relay_state + "</p>");
//
//                        // Check fault EV state
//                        if (fault)
//                        {
//                            client.println("<font style='color:red'>");
//                            client.println("<p>EV FAULT DETECTED - reset system</p>");
//                        }
//                        else if (ev_state == EV_NOT_CONNECTED)
//                        {
//                            client.println("<font style='color:red'>");
//                            client.println("<p>EV Not Connected</p>");
//                        }
//                        else if (ev_state == EV_CONNECTED)
//                        {
//                            client.println("<font style='color:green'>");
//                            client.println("<p>EV Connected</p>");
//
//                            // If the relay is off, it displays the ON button
//                            if (relay_state == "off")
//                                client.println("<p><a href=\"/relay/on\"><button class=\"button\">CHARGE</button></a></p>");
//                            else
//                                client.println("<p><a href=\"/relay/off\"><button class=\"button button2\">STOP</button></a></p>");
//                        }
//                        else
//                        {
//                            client.println("<font style='color:red'>");
//                            client.println("<p>EV State Unknown</p>");
//                        }
//
//                        client.println("</body></html>");
//
//                        // The HTTP response ends with another blank line
//                        client.println();
//                        // Break out of the while loop
//                        break;
//                    }
//                    else // if you got a newline, then clear currentLine
//                    {
//                        currentLine = "";
//                    }
//                }
//                else if (c != '\r') // if you got anything else but a carriage return character
//                {
//                    currentLine += c;
//                }
//            }
//        }
//        // Close the connection
//        client.stop();
//        Serial.println("Client disconnected");
//        Serial.println("");
//    }
//    delay(200);
//}
