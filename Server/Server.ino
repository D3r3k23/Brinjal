#define ENABLE_SERVER 1

#include "Brinjal.h"
#include "Tests.h"

#if ENABLE_SERVER
    #include <WiFi.h>
    const char* ssid     = "Brinjal";
    const char* password = "password123";

    WiFiServer server(80);
#endif

Brinjal brinjal;
Tests tests(&brinjal);

void setup()
{
    Serial.begin(115200);

    brinjal.begin();

    tests.relay();

#if EN_SERVER
    Serial.println("Setting up WiFi Access Point");
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    server.begin();
#endif
}

void loop()
{
    brinjal.loop();

#if ENABLE_SERVER
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

                        if (header.indexOf("GET /relay/on") >= 0)
                        {

                        }
                        else if (header.indexOf("GET /relay/off") >= 0)
                        {

                        }

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");

                        // CSS to style the on/off buttons
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");

                        // Web Page Heading
                        client.println("<body><h1>Brinjal</h1>");

                        String relay_state = brinjal.get_relay_state() == RELAY_CLOSED ? "ON" : "OFF";

                        // Display current state, and ON/OFF buttons
                        client.println("<p>Relay state: " + relay_state + "</p>");

                        // Check fault EV state
                        if (0)
                        {
                            client.println("<font style='color:red'>");
                            client.println("<p>EV FAULT DETECTED - reset system</p>");
                        }
                        else if (0)
                        {
                            client.println("<font style='color:red'>");
                            client.println("<p>EV Not Connected</p>");
                        }
                        else if (1)
                        {
                            client.println("<font style='color:green'>");
                            client.println("<p>EV Connected</p>");

                            // If the relay is off, it displays the ON button
                            if (1)
                                client.println("<p><a href=\"/relay/on\"><button class=\"button\">CHARGE</button></a></p>");
                            else
                                client.println("<p><a href=\"/relay/off\"><button class=\"button button2\">STOP</button></a></p>");
                        }
                        else
                        {
                            client.println("<font style='color:red'>");
                            client.println("<p>EV State Unknown</p>");
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
#endif
    delay(200);
}
