#include "Brinjal.h"

#define ENABLE_SERVER 1

#if ENABLE_SERVER
    #include <WiFi.h>
    const char* ssid     = "Brinjal";
    const char* password = "password123";

    WiFiServer server(80);
    WiFiClient client;
#endif

Brinjal brinjal;

#if ENABLE_SERVER
    void write_html_webpage()
    {
        client.print("<h1 style='font-size:60px;'><a href='/'>");
        client.print("Brinjal");
        client.println("</a></h1>");

        if (brinjal.in_fault_mode())
        {
            client.println("<font style='color:red'>");
            client.println("<h2>FAULT DETECTED</h2>");
            client.println("<font style='color:red'>");
            client.println("<p>See LCD display</p>");
        }
        else if (brinjal.get_evsu_state() == EVSU_CHARGING)
        {
            client.println("<font style='color:green'>");
            client.println("<h2>CHARGING</h2>");
        }
        else if (brinjal.get_evsu_state() == EVSU_READY)
        {
            client.println("<font style='color:green'>");
            client.println("<h3>Ready to charge</h3>");
        }

        // CSS buttons
        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
        client.println(".button2 {background-color: #555555;}</style></head>");

        if (brinjal.get_evsu_state() == EVSU_CHARGING)
            client.println("<p><a href='/stop'><button class='button button2'>STOP</button></a></p>");
        else
            client.println("<p><a href='/charge'><button class='button'>CHARGE</button></a></p>");

        client.println("<font style='color:black'>");
        client.println("<p>EVSU state: " + evsu_state_to_string(brinjal.get_evsu_state()) + "</p>");
        client.println("<p>EV (CP) state: " + ev_state_to_string(brinjal.get_vehicle_state()) + "</p>");
        client.println("<p>Relay state: " + String((brinjal.relay_closed() ? "Closed" : "Open")) + "</p>");
        client.println("<p>Fault mode: " + String((brinjal.in_fault_mode() ? "ON" : "OFF")) + "</p>");
    }
#endif

void setup()
{
    Serial.begin(115200);

    brinjal.begin();

#if ENABLE_SERVER
    Serial.println("Setting up WiFi Access Point");
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP address: ");
    Serial.println(IP);

    server.begin();
#endif
}

void loop()
{
    brinjal.loop();

#if ENABLE_SERVER
    if (client = server.available()) // If a new client connects
    {
        Serial.println("Client Connected");
        String header = "";
        String currentLine = "";

        while (client.available()) // Client data available
        {
            char c = client.read();
            // Serial.write(c);
            header += c;
            if (c == '\n')
            {
                // 2 newlines in a row -> end of client HTTP request
                if (currentLine.length() == 0)
                {
                    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                    // and a content-type so the client knows what's coming, then a blank line:
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println("Connection: close");
                    client.println();

                    ///////////////////
                    // Check buttons //
                    if (header.indexOf("GET /charge") >= 0)
                    {
                        Serial.println("Charge requested by webpage");
                        brinjal.request_charge();
                    }
                    if (header.indexOf("GET /stop") >= 0)
                    {
                        if (brinjal.get_evsu_state() == EVSU_CHARGING)
                        {
                            Serial.println("Charge stopped by webpage");
                            brinjal.stop_charging();
                        }
                    }

                    // HTML header
                    client.println("<!DOCTYPE html><html>");
                    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                    client.println("<link rel=\"icon\" href=\"data:,\">");

                    client.print("<body>");
                    write_html_webpage();
                    client.println("</body></html>");

                    // HTTP response ends with newline
                    client.println();
                    break;
                }
                else
                {
                    currentLine = "";
                }
            }
            else if (c != '\r')
            {
                currentLine += c;
            }
        }

        // Close connection
        client.stop();
        Serial.println("Client disconnected");
    }
#endif
}
