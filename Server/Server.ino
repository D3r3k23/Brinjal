#include "Brinjal.h"
#include "Tests.h"
#include <WiFi.h>


Brinjal brinjal;
Tests tests(&brinjal);

const char* ssid     = "Brinjal";
const char* password = "password123";

String output = "OFF";



WiFiServer server(80);
LiquidCrystal_I2C LCD1(0x27, 16, 2);
void setup()
{
    Serial.begin(115200);
    brinjal.begin();

    Serial.println("\nSetting up WiFi Access Point");
   
    Wire.begin(41, 42);
    Serial.print(" - Initializing LCD...\n");
    delay(100);
    LCD1.init();
    LCD1.backlight();
    LCD1.print("WELLCOME");
    delay(3000);
   
    
    Serial.println();
    Serial.println("Configuring access point...");

    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    
    server.begin();
    LCD1.clear();
    LCD1.print("SETTING WIFI");
    delay(4000);

//    LCD1.clear();
//    LCD1.print("EVSE TEST...");
//    delay(5000);
    
//    if (tests.gfci()==passed,tests.relay()==true)
//    { 
//       LCD1.clear();
//       LCD1.print("TEST SUCCESSFUL");
//       delay(5000);
//    }
//    else{
//       LCD1.clear();
//       LCD1.print("TEST FAILED?");
//       brinjal.stop();
//       delay(5000);
//    }
    // tests.pilot();
    // tests.gfci();
    // tests.relay();

    LCD1.clear();
    LCD1.print("EVSE READY");
    delay(5000);
}

void loop()
 {
     
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
                         if (header.indexOf("GET /chargerstat/on") >= 0)
                         {
                          Serial.println("\nCharger is ON");
                          output = "ON";
                          brinjal.loop();
                         }
                         else if (header.indexOf("GET /chargerstat/off") >= 0)
                         {
                          Serial.println("\nCharger is ON");
                          output = "OFF";
                          LCD1.clear();
                          LCD1.print("EVSE is OFF");
                         }

                         // Display the HTML web page
                         client.println("<!DOCTYPE html><html>");
                         client.println("<head><meta name=\"viewport\" content=\"width=LCD-width, initial-scale=1\">");
                         client.println("<link rel=\"icon\" href=\"data:,\">");

                         // CSS to style the on/off buttons
                         client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                         client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                         client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                         client.println(".button2 {background-color: #555555;}</style></head>");

                         // Web Page Heading

                         
                         client.println("<body><p><h1>Brinjal EVSE CHARGER</h1>");
            
                        // Display current state, and ON/OFF buttons for GPIO 26  
                        client.println("<p><p>EVSE STATE: " + output + "</p>");
                        // If the output26State is off, it displays the ON button       
                        if (output=="OFF") {
                          client.println("<p><a href=\"/chargerstat/on\"><button class=\"button\">Start Charging</button></a></p>");
                        } else {
                          client.println("<p><a href=\"/chargerstat/off\"><button class=\"button button2\">Stop Charging</button></a></p>");
                        } 

                        
                        // Display current state
                         //String cahrger_state = brinjal.
                         //client.println("<p>Chargaing state:<p>" + cahrger_state + "</p>");
                         //client.println("<p>Chargaing AT:<p>" + brinjal.chargingCurrent + "</p>");
                         
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
