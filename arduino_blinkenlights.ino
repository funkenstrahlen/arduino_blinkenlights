/*

  A small webserver that parses the GET request URL to extract color values.

  this is how a common HTTP request looks like:
  -------------------------------
  GET /?red=1&green=255&blue=255 HTTP/1.1
  Host: 192.168.178.55
  Accept-Encoding: identity
  
  -------------------------------

  This code will extract the red, green and blue value and set
  the color to the attached RGB LEDs.

  The webserver also replies to the client with a small webpage that contains
  some links to click. Clicking on that links will simply create a new GET request
  with the correct RGB values in the URL for the color the link stands for.

  created 24 Feb 2013
  by Stefan Trauth

 */

#include <SPI.h>
#include <Ethernet.h>
#include <String.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 178, 55);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

// LED Color Pins
int redPin = 5;
int greenPin = 6;
int bluePin = 9;

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("request received");
    // this string will contain the request string
    String requestQuery = "";
    while (client.connected()) {
      if (client.available()) {
        // this is how a common HTTP request looks like:
        // -------------------------------
        // GET /?red=1&green=255&blue=255 HTTP/1.1
        // Host: 192.168.178.55
        // Accept-Encoding: identity
        // 
        // -------------------------------
        // KEEP IN MIND: The last empty line marks the end!
        char c = client.read();
        // if the first newline is detected, we stop reading the request as we are
        // not interested in the rest of the request data
        if(c != '\n') {
          requestQuery += c;
        } else break;
      }
    }

    // debug output to serial
    Serial.println(requestQuery);

    // extract all the color values from the request query string
    // the request query string looks like this now:
    // GET /?red=1&green=255&blue=255 HTTP/1.1
    int red = extractColor(requestQuery, "red");
    int green = extractColor(requestQuery, "green");
    int blue = extractColor(requestQuery, "blue");

    // only apply the values if they are valid color values
    if(colorIsValid(red, green, blue)) {
      // print all the extracted values to serial for debug usage
      Serial.print("RED: ");
      Serial.println(red);
      Serial.print("GREEN: ");
      Serial.println(green);
      Serial.print("BLUE: ");
      Serial.println(blue);

      // set the colors to the LED color controller
      analogWrite(redPin, red);
      analogWrite(greenPin, green);
      analogWrite(bluePin, blue);
    } else {
      Serial.println("Invalid color definition in request. Color will not be changed.");
    }


    // send a response website which contains some links to click for a more comfortable
    // selection of colors
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html><header></header><body>");
    // add some links here to click
    client.println("<h1><a href=\"/?red=255&green=0&blue=0\">RED</a></h1>");
    client.println("<h1><a href=\"/?red=0&green=255&blue=0\">GREEN</a></h1>");
    client.println("<h1><a href=\"/?red=0&green=0&blue=255\">BLUE</a></h1>");
    client.println("<h1><a href=\"/?red=255&green=0&blue=255\">PINK</a></h1>");
    client.println("<h1><a href=\"/?red=0&green=255&blue=255\">TURQUOISE</a></h1>");
    client.println("<h1><a href=\"/?red=255&green=255&blue=0\">YELLOW</a></h1>");
    client.println("<h1><a href=\"/?red=255&green=255&blue=255\">WHITE</a></h1>");
    client.println("</body></html>");

    delay(1);

    // close the connection:
    client.stop();
    Serial.println("request parsed");
  }
}

/**
 * Extracts the integer color value of a specific color from a query string looking like this:
 * GET /?red=1&green=255&blue=255 HTTP/1.1
 * Example: You want to extract the red value from the query. Do the following:
 * int red = extractColor(query, "red");
 * The red integer variable will now be 1.
 * 
 * @param  requestQuery     the request query string that contains the URL formated color values
 * @param  colorToExtract   the string representation of the color you want to extract the value from
 * @return                  the integer color value
 */
int extractColor(String requestQuery, String colorToExtract) {
  // find the position in the string where the value definition starts
  // for example: find the position of "red="
  int startPos = requestQuery.indexOf(colorToExtract + "=");
  boolean colorFound = startPos != -1;
  // as we found the position of "r" in the example of "red=" we need to jump
  // forward some chars to find the position of the actual value.
  // For "red=255" we need to skip "red=". In this case 4 chars.
  startPos += (colorToExtract.length() + 1);

  // we do have the start position but we also need the end position of the value as 
  // we want to get a substring later to isolate the actual color value.
  // in the most cases the end of the values is marked with the "&". so start searching for the
  // "&" from the start position we just found.
  int endPos = requestQuery.indexOf('&', startPos);
  // for the last color value (in the example "blue=255") there is no "&" to mark the end.
  // in this case endPos will be -1 here.
  if(endPos == -1){
    // we give it a second chance to find a correct end position. We know that it will end with
    // a simple whitespace.
    endPos = requestQuery.indexOf(' ', startPos);
  }

  // indexOf returns -1 if it could not find something. Check here if the values are ok
  // before calling substring
  if(colorFound && startPos != endPos) {
    // create a substring, so extract the actual color value as string
    // for "red=1" this will create "1" as colorValueAsString here
    String colorValueAsString = requestQuery.substring(startPos, endPos);
    // create a char array to store the colorValueAsString
    char convert[colorValueAsString.length() + 1];
    // put the colorValueAsString data into the char array
    colorValueAsString.toCharArray(convert, colorValueAsString.length() + 1);
    // the char array is required, because atoi() only works on char arrays
    // atoi() converts the string represented integer value into an actual integer variable
    return atoi(convert);
  } else return -1;
}

/**
 * Check if the RGB color values are in range and therefore a valid useable color.
 * @param  red   the red color value
 * @param  green the green color value
 * @param  blue  the blue color value
 * @return       true if all the color values are in range, else false.
 */
boolean colorIsValid(int red, int green, int blue) {
  // check every color value if it is in range
  if(red < 0 || red > 255) return false;
  if(green < 0 || green > 255) return false;
  if(blue < 0 || blue > 255) return false;
  // only return true if all the checks above passed
  return true;
}

