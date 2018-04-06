#include <WiFiLink.h>
#include <UnoWiFiDevEdSerial1.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

char ssid[] = "dem nets"; //  your network SSID (name)
char pass[] = "SECRET_PASS";    // your network password (use for WPA, or use as key for WEP)
WiFiServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(11111);

boolean debug = true; // verbose serial printing

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  configureWiFi();
  server.begin();
  
  configureMag();
}

void loop() {
  // wait for a connected client:
  WiFiClient client = server.available();
  if (client) {
    if (debug) Serial.println("Client connected");
    // when the client sends the first byte, say hello:
    if (client.available()>0) {
      if (!alreadyConnected) {
        // clean out the input buffer:
        client.flush();
        Serial.println("We have a new client");
        client.println("Hello, client!");
        displayMenu(&client);
        alreadyConnected = true;
      }
      //char input = displayMenu(&client);
      char input = client.read();
      
      switch(input) {
        case '1':
          client.println("do stuff");
          break;
        case '2':
          client.println("Resetting sensor not yet implemented");
          break;
        default:
          return;
      }

    }
  }
  else {
   alreadyConnected = false;
   client.stop();
   if (debug) Serial.println("Purging connections");
  }
  if (debug) Serial.println("finished loop");
  delay(5000);
}



void configureWiFi() {
  Serial1.begin(115200); // speed must match with BAUDRATE_COMMUNICATION setting in firmware config.h
  WiFi.init(&Serial1);
  WiFi.resetESP(); // to clear 'sockets' after sketch upload
  delay(3000);
  Serial.println("ESP reset");
  
  // attempt to connect to Wifi network:
  int status = WL_IDLE_STATUS;
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(3000);  // wait 5 seconds for connection:
  }
  
  // print connection information for network:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}


void configureMag() {
  /* Initialise the sensor */
  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
  
  //Display some basic information on this sensor 
  displayMagDetails();
}


void displayMenu( WiFiClient *client) {
  
  client->println("Select an option: ");
  client->println(" (1) Display sensor readings");
  client->println(" (2) Reset sensor");
}


void displayMagDetails() {
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor: magnetometer array 1 "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
}


void displayMagReadings( WiFiClient *client) {
  //* Get a new sensor event*/
  sensors_event_t event; 
  mag.getEvent(&event);
 
  // Display the results (magnetic vector values are in micro-Tesla (uT))
  Serial.print("X: "); Serial.print(event.magnetic.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.magnetic.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.magnetic.z); Serial.print("  ");Serial.println("uT");

  // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Seattle is: +15* 45' E, which is ~15 Degrees, or (which we need) 0.26 radians
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = 0.26;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  
  Serial.print("Heading (degrees): "); Serial.println(headingDegrees);
  client->println(headingDegrees);

  delay(1000);
}
