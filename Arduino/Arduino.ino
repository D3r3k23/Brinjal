// Load Wi-Fi library
#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "Brinjal";
const char* password = "password";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output36State = "off";
String output37State = "off";

// Assign output variables to GPIO pins
const int output36 = 36;
const int output37 = 37;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output36, OUTPUT);
  pinMode(output37, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output36, LOW);
  digitalWrite(output37, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /36/on") >= 0) {
              Serial.println("GPIO 36 on");
              output36State = "on";
              digitalWrite(output36, HIGH);
            } else if (header.indexOf("GET /36/off") >= 0) {
              Serial.println("GPIO 36 off");
              output36State = "off";
              digitalWrite(output36, LOW);
            } else if (header.indexOf("GET /37/on") >= 0) {
              Serial.println("GPIO 37 on");
              output37State = "on";
              digitalWrite(output37, HIGH);
            } else if (header.indexOf("GET /37/off") >= 0) {
              Serial.println("GPIO 37 off");
              output37State = "off";
              digitalWrite(output37, LOW);
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
            
            // Display current state, and ON/OFF buttons for GPIO 36  
            client.println("<p>GPIO 36 - State " + output36State + "</p>");
            // If the output36State is off, it displays the ON button       
            if (output36State=="off") {
              client.println("<p><a href=\"/36/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/36/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 37  
            client.println("<p>GPIO 37 - State " + output37State + "</p>");
            // If the output37State is off, it displays the ON button       
            if (output37State=="off") {
              client.println("<p><a href=\"/37/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/37/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
