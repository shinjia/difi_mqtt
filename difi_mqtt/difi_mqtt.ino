/* Shinjia  v1.0.9  2022/08/18 */

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YOUR-WIFI-SSID"
#define WLAN_PASS       "YOUR-WIFI-PASSWORD"

/************************* MQTT Server Setup *********************************/

#define AIO_SERVER      "YOUR-MQTT-SERVER"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOUR-MQTT-USERNAME"
#define AIO_KEY         "YOUR-MQTT-PASSWORD"

/************************* MQTT Topic **************************************/

#define MQTT_TOPIC_LDRSENSOR   "user00/difi00/ldrsensor"
#define MQTT_TOPIC_TEMPERATURE "user00/difi00/temperature"
#define MQTT_TOPIC_HUMIDITY    "user00/difi00/humidity"
#define MQTT_TOPIC_BUTTON      "user00/difi00/button"
#define MQTT_TOPIC_IRSW        "user00/difi00/irsw"
#define MQTT_TOPIC_BUZZER      "user00/difi00/buzzer"
#define MQTT_TOPIC_RELAY       "user00/difi00/relay"
#define MQTT_TOPIC_LED         "user00/difi00/led"
#define MQTT_TOPIC_RGB         "user00/difi00/rgb"
#define MQTT_TOPIC_COUNTER     "user00/difi00/counter"
#define MQTT_TOPIC_COUNTER_SET "user00/difi00/counter/set"
#define MQTT_TOPIC_LIGHT       "user00/difi00/light"
#define MQTT_TOPIC_QUERY       "user00/difi00/query"
#define MQTT_TOPIC_RESPONSE    "user00/difi00/response"

// Time control (delay time)
int time_delay_dht       = 60 * 1000;
int time_delay_ldrsensor =  2 * 1000;
int time_delay_led       =  5 * 1000;
int time_delay_rgb       =  5 * 1000;
int time_delay_light     = 60 * 1000;
int time_delay_button    = 20;  // for debounce
int time_delay_irsw      = 2000; // delay when state change

// LED Pin
#define PIN_LED   2
#define PIN_LED_R 12
#define PIN_LED_G 13
#define PIN_LED_B 14

/** other pin **/
#define PIN_LDRSENSOR A0
#define PIN_DHT        5
#define PIN_IRSW       4
#define PIN_RELAY     16
#define PIN_BUTTON     0
#define PIN_BUZZER    15


/*************************** Sketch Code ************************************/

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

Adafruit_MQTT_Publish sensor_ldrsensor   = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_LDRSENSOR);
Adafruit_MQTT_Publish sensor_temperature = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_TEMPERATURE);
Adafruit_MQTT_Publish sensor_humidity    = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_HUMIDITY);
Adafruit_MQTT_Publish sensor_button      = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_BUTTON);
Adafruit_MQTT_Publish sensor_irsw        = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_IRSW);
Adafruit_MQTT_Publish sensor_response    = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_RESPONSE);
Adafruit_MQTT_Publish sensor_counter     = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC_COUNTER);

// Setup a feed
Adafruit_MQTT_Subscribe sub_buzzer  = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_BUZZER, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_relay   = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_RELAY, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_led     = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_LED, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_rgb     = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_RGB, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_light   = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_LIGHT, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_query   = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_QUERY, MQTT_QOS_1);
Adafruit_MQTT_Subscribe sub_counter_set = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TOPIC_COUNTER_SET, MQTT_QOS_1);

boolean is_mqtt_pub_button = false;
boolean is_mqtt_pub_irsw = false;
boolean is_mqtt_pub_counter = false;


// Variables about button
int buttonSwitchState = HIGH;  // button as switch
int buttonState;
int lastButtonState = HIGH;
int counter_button = 0;

// Variables about IRSW
int irswSwitchState = HIGH;  // IRSW as switch
int irswState;
int lastIrswState = HIGH;

/** DHT 11 **/
#include "DHT.h"
#define DHTTYPE DHT11

DHT dht(PIN_DHT, DHTTYPE);

// Variables about LED Lightshow
int lightShowType = 0; // Global variable
int showState = 0;
int light_param0;
int light_param1;
int light_param2;

// time control
unsigned long time_next_dht;
unsigned long time_next_ldrsensor;
unsigned long time_next_button;
unsigned long time_next_irsw;
unsigned long time_next_led;
unsigned long time_next_rgb;
unsigned long time_next_light;

unsigned long time_next_light_phase;

boolean time_check_rgb = false;
boolean time_check_light = false;

// Prototype
void MQTT_connect();


/****************************** Feeds ***************************************/

void sub_led_callback(unsigned int x) {
  Serial.print("Hey we're in a button value callback, the value is: ");
  Serial.println(x);
}


void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_IRSW, INPUT);

  dht.begin();
  
  time_next_ldrsensor = millis();
  time_next_dht    = millis();
  time_next_dht    = millis();
  time_next_led    = millis();
  time_next_rgb    = millis();
  time_next_light  = millis();

  // Serial port
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  
  sub_led.setCallback(sub_led_callback);
  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&sub_buzzer);
  mqtt.subscribe(&sub_relay);
  mqtt.subscribe(&sub_led);
  mqtt.subscribe(&sub_rgb);
  mqtt.subscribe(&sub_counter_set);
  mqtt.subscribe(&sub_light);
  mqtt.subscribe(&sub_query);
}


void loop()
{
  char *payload;
    
  // ldrsensor
  int photor = analogRead(PIN_LDRSENSOR);

  // dht
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  
  if (isnan(h) || isnan(t) || isnan(f)) {
    //Serial.println("Failed to read from DHT sensor!");
    //return;
    t = 0;
    h = 0; 
  }


  // Button (notice: Debounce)
  int reading = digitalRead(PIN_BUTTON);
  is_mqtt_pub_button = false;
  if (reading != lastButtonState) {
    time_next_button = millis() + time_delay_button;
  }

  if (millis() > time_next_button) {
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        buttonSwitchState = !buttonSwitchState;
        counter_button++;
        is_mqtt_pub_button = true;
      }
    }
  }
  lastButtonState = reading;

  
  // IRSW
  irswState = !digitalRead(PIN_IRSW);  // IRSW inverse
  is_mqtt_pub_irsw = false;
  if (irswState != lastIrswState) {
     irswSwitchState = !irswSwitchState;
     time_next_irsw = millis() + time_delay_irsw;
     is_mqtt_pub_irsw = true;
  }
  lastIrswState = irswState;

  // MQTT  
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  
  while ((subscription = mqtt.readSubscription(100))) {

    if (subscription == &sub_relay) {
      payload = (char *)sub_relay.lastread;
      Serial.print(F("Got (relay): "));
      Serial.print(payload);
      Serial.println();
      if(strcmp(payload, "1")==0) {
        digitalWrite(PIN_RELAY, 1);
      }
      else if(strcmp(payload, "0")==0) {
        digitalWrite(PIN_RELAY, 0);
      }
    }
    else if (subscription == &sub_led) {
      payload = (char *)sub_led.lastread;
      Serial.print(F("Got: (led)"));
      Serial.print(payload);
      Serial.println();
      if(strcmp(payload, "ON")==0) {
        digitalWrite(PIN_LED, 0);  // builtin LED inverse
      }
      else if(strcmp(payload, "OFF")==0) {
        digitalWrite(PIN_LED, 1);  // builtin LED inverse
      }
      else {
        if(payload[0]>='0' && payload[0]<='9') {  // check not digit
          int value = (int)(payload[0]) - 48;
          for(int i=0; i<value; i++) {  
            delay(300);          
            digitalWrite(PIN_LED, 0);  // builtin LED inverse
            delay(100);
            digitalWrite(PIN_LED, 1);  // builtin LED inverse
          }
        }
      }
      time_next_led = millis() + time_delay_led;
    }
    else if (subscription == &sub_rgb) {
      payload = (char *)sub_rgb.lastread;
      Serial.print(F("Got: (rgb)"));
      Serial.print(payload);
      Serial.println();
      rgb_do(payload);
      time_next_rgb = millis() + time_delay_rgb;
      time_check_rgb = true;
      time_check_light = false;
      lightShowType = 0;  // stop light show
    }
    else if (subscription == &sub_light) {
      payload = (char *)sub_light.lastread;
      Serial.println("Subscribe get LIGHT");
      Serial.print(payload);
      Serial.println();
      light_set(payload);
      time_next_light = millis() + time_delay_light;
      time_check_rgb = false;
      time_check_light = true;
    }
    else if (subscription == &sub_counter_set) {
      payload = (char *)sub_counter_set.lastread;
      Serial.print(F("Got (counter_set): "));
      Serial.print(payload);
      Serial.println();
      /*
      if(strcmp(payload, "0")==0) {
        counter_button = 0;
      }
      */
      String stringOne = String(payload);
      counter_button = stringOne.toInt();
      is_mqtt_pub_counter = true;
      Serial.print("Set Counter to ");
      Serial.print(counter_button);
      Serial.println();
    }
    else if (subscription == &sub_buzzer) {
      payload = (char *)sub_buzzer.lastread;
      Serial.println("Subscribe get BUZZER");
      play_music(1);
    }
    else if (subscription == &sub_query) {
      payload = (char *)sub_query.lastread;
      String ret = query_do(payload);
      Serial.print("publish (response):");
      Serial.println(ret);
      sensor_response.publish(ret.c_str());
    }
  }

  // MQTT Pub : ldrsensor
  if(millis() > time_next_ldrsensor) {
    time_next_ldrsensor = millis() + time_delay_ldrsensor;
    sensor_ldrsensor.publish(photor);
  }

  // MQTT Pub : dht
  if(millis() > time_next_dht) {
    Serial.print("publish (dht-t):");
    Serial.print(t);
    Serial.print("    (dht-h):");
    Serial.print(h);
    Serial.println();
    time_next_dht = millis() + time_delay_dht;
    sensor_temperature.publish(t);
    sensor_humidity.publish(h);
  }

  // MQTT Pub : button
  if(is_mqtt_pub_button) {    
    Serial.print("publish (button):");
    Serial.print(buttonSwitchState);
    Serial.println();
    /*
    if(buttonSwitchState==1) {
      sensor_button.publish("ON");
    }
    else {
      sensor_button.publish("OFF");
    }
    */
    sensor_button.publish(buttonSwitchState);
    sensor_counter.publish(counter_button);
  }

  if(is_mqtt_pub_counter) {
    sensor_counter.publish(counter_button);  // when counter_reset, publish counter
    is_mqtt_pub_counter = false;
  }

  // MQTT Pub : IRSW
  if(is_mqtt_pub_irsw) {    
    Serial.print("publish (IRSW):");
    Serial.print(irswSwitchState);
    Serial.println();
    sensor_irsw.publish(irswSwitchState);
  }

  // Light show
  light_run();

  // check if rgb timeout
  if(time_check_rgb && millis() > time_next_rgb) {
    digitalWrite(PIN_LED_R, 0);
    digitalWrite(PIN_LED_G, 0);
    digitalWrite(PIN_LED_B, 0);
  }
  
  // check if light timeout
  if(time_check_light && millis() > time_next_light) {
    lightShowType = 0;
    digitalWrite(PIN_LED_R, 0);
    digitalWrite(PIN_LED_G, 0);
    digitalWrite(PIN_LED_B, 0);
  }
}


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
  
  mqtt.subscribe(&sub_led);
  
  for(int i=0; i<5; i++)
  {
    digitalWrite(PIN_LED, LOW);  delay(100);
    digitalWrite(PIN_LED, HIGH);  delay(100);
  }
}


void rgb_do(char *p) {
  int len = strlen(p);
  int value = 0;
  char ch;

  if(len==7 && p[0]=='#') {
    // #RRGGBB ??????
    analogWrite(PIN_LED_R, 16*x2b(p[1])+x2b(p[2]));
    analogWrite(PIN_LED_G, 16*x2b(p[3])+x2b(p[4]));
    analogWrite(PIN_LED_B, 16*x2b(p[5])+x2b(p[6]));
  }
  else if(len>=2 && (p[1]>='0' && p[1]<='9')) {
    // Rn, Gn, Bn ??????
    if(p[0]=='R' || p[0]=='r') {
      value = 0;
      for(int i=1; i<len; i++) {
        value = value*10 + ((int)(p[i]) - 48);
      }
      analogWrite(PIN_LED_R, value);
    }
    else if(p[0]=='G' || p[0]=='g') {
      value = 0;
      for(int i=1; i<len; i++) {
        value = value*10 + ((int)(p[i]) - 48);
      }
      analogWrite(PIN_LED_G, value);
    }  
    else if(p[0]=='B' || p[0]=='b') {
      value = 0;
      for(int i=1; i<len; i++) {
        value = value*10 + ((int)(p[i]) - 48);
      }
      analogWrite(PIN_LED_B, value);
    }
  }
  else if ((p[0]>='0' && p[0]<='9')) {
    // number
    value = 0;
    for(int i=0; i<len; i++) {
      if (p[i] >= 48 && p[i] <= 57) {
        value = value * 10 + ((int)(p[i])-48);
      }
    }
    time_delay_rgb = value * 1000;
    Serial.print("delay...");
    Serial.println(value);
  }
  else {
    // RGBrgb ??????
    for(int i=0; i<len; i++) {
      ch = p[i];
      if(!(ch>='0' && ch<='9')) {  // check not digit
        if(ch=='R') {
          digitalWrite(PIN_LED_R, 1);
        }
        else if(ch=='G') {
          digitalWrite(PIN_LED_G, 1);
        }
        else if(ch=='B') {
          digitalWrite(PIN_LED_B, 1);
        }
        else if(ch=='r') {
          digitalWrite(PIN_LED_R, 0);
        }
        else if(ch=='g') {
          digitalWrite(PIN_LED_G, 0);
        }
        else if(ch=='b') {
          digitalWrite(PIN_LED_B, 0);
        }
        else if(ch=='-') {
          digitalWrite(PIN_LED_R, 0);
          digitalWrite(PIN_LED_G, 0);
          digitalWrite(PIN_LED_B, 0);
        }
      }
    }
  }
}


void light_set(char *p) {
  int len = strlen(p);
  int value = 0;
  
  if(p[0]=='0') {
    lightShowType = 0;
    Serial.print("listShowType: 0");
  }
  
  if(!(p[0]>='0' && p[0]<='9')) {  // check not digit
    if(p[0]=='A' || p[0]=='a') {
      Serial.println("lightShowType: A");
      lightShowType = 1;
      light_param0 = 200;
      light_param1 = 200;
      // set up speed (0~9: 50+100*i)
      if(len>=2) {
      value = 50 + (9-((int)p[1]-48)) * 100;
        Serial.print("Speed:");
        Serial.println(value);
        light_param0 = value;
        light_param1 = value;
      }
      showState = 0;
      time_next_light_phase = millis() + light_param0;
    }
    else if(p[0]=='B' || p[0]=='b') {
      Serial.println("lightShowType: B");
      lightShowType = 2;
      light_param0 = 1000;
      light_param1 = 1000;
      light_param2 = 1000;
      // set up speed (0~9: 50+300*i)
      if(len>=2) {
      value = 50 + (9-((int)p[1]-48)) * 300;
        Serial.print("Speed:");
        Serial.println(value);
        light_param0 = value;
        light_param1 = value;
        light_param2 = value;
      }
      showState = 0;
      time_next_light_phase = millis() + light_param0;
    }
    else if(p[0]=='C' || p[0]=='c') {
      Serial.println("lightShowType: B");
      lightShowType = 3;
      light_param0 = 1;  // timer (meanless)
      light_param1 = 20; // step
      // set up transition step (0~9: 1+8*i)
      if(len>=2) {
        value = 1 + ((int)p[1]-48)*10;
        Serial.print("Step:");
        Serial.println(value);
        light_param1 = value;  // step change
      }
      showState = 0;
      time_next_light_phase = millis() + light_param0;
    }
  }
  else {
    // delay time
    value = 0;
    for(int i=0; i<len; i++) {
      if (p[i] >= 48 && p[i] <= 57) {
        value = value * 10 + ((int)(p[i])-48);
      }
    }
    time_delay_light = value * 1000;
    Serial.print("delay...");
    Serial.println(value);
  }
}


String query_do(char *p) {
  int len = strlen(p);
  int value = 0;
  String ret;
  
  if(strcmp(p, "time")==0) {
    // get time
    int ts = millis() / 1000;
    int hh = ts / 3600;
    int mm = (ts % 3600) / 60;
    int ss = (ts % 3600) % 60;
    ret = "";
    if(hh<10) {ret += "0";}
    ret += String(hh) + ":";
    if(mm<10) {ret += "0";}
    ret += String(mm) + ":";
    if(ss<10) {ret += "0";}
    ret += String(ss);
    // ret = String(hh) + ":" + String(mm) + ":" + String(ss);
  }
  else if(strcmp(p, "wifi")==0) {
    ret = String(WLAN_SSID) + "/" + String(WLAN_PASS);
  }
  else if(strcmp(p, "rssi")==0) {
    ret = String(WiFi.RSSI());
  }
  else if(strcmp(p, "ip")==0) {
    ret = WiFi.localIP().toString();
  }
  else {
    ret = "????????????????????????";
  }

  return ret;
}


void light_run() {

  if(lightShowType==0) {
    // do nothing
  }
  
  else if(lightShowType==1) {
    if(showState==0) {
      if(millis() > time_next_light_phase) {
        time_next_light_phase = millis() + light_param1;
        showState = 1;
      }
      digitalWrite(PIN_LED_R, 1);
      digitalWrite(PIN_LED_G, 0);
      digitalWrite(PIN_LED_B, 0);
    }
    else if(showState==1) {
      if(millis() > time_next_light_phase) {
        time_next_light_phase = millis() + light_param0;
        showState = 0;
      }
      digitalWrite(PIN_LED_R, 0);
      digitalWrite(PIN_LED_G, 0);
      digitalWrite(PIN_LED_B, 1);
    }
  }
  
  else if(lightShowType==2) {
    if(showState==0) {
      if(millis() > time_next_light_phase) {
        time_next_light_phase = millis() + light_param1;
        showState = 1;
      }
      digitalWrite(PIN_LED_R, 1);
      digitalWrite(PIN_LED_G, 0);
      digitalWrite(PIN_LED_B, 0);
    }
    else if(showState==1) {
      if(millis() > time_next_light_phase) {
        time_next_light_phase = millis() + light_param2;
        showState = 2;
      }
      digitalWrite(PIN_LED_R, 0);
      digitalWrite(PIN_LED_G, 1);
      digitalWrite(PIN_LED_B, 0);
    }
    else if(showState==2) {
      if(millis() > time_next_light_phase) {
        time_next_light_phase = millis() + light_param0;
        showState = 0;
      }
      digitalWrite(PIN_LED_R, 0);
      digitalWrite(PIN_LED_G, 0);
      digitalWrite(PIN_LED_B, 1);
    }
  }
  
  else if(lightShowType==3) {
    if(millis() > time_next_light_phase) {
      time_next_light_phase = millis() + light_param0;
      showState = (showState+light_param1) % 1536;  // 256*6=1536
    }
    int section = showState / 256;
    int remain = showState % 256;
    //Serial.print(section);
    //Serial.print("--");
    //Serial.print(remain);
    //Serial.println();
    switch(section) {
      case 0: 
        analogWrite(PIN_LED_R, 255);
        analogWrite(PIN_LED_G, 0);
        analogWrite(PIN_LED_B, 255-remain);
        break;
      case 1: 
        analogWrite(PIN_LED_R, 255);
        analogWrite(PIN_LED_G, remain);
        analogWrite(PIN_LED_B, 0);
        break;
      case 2: 
        analogWrite(PIN_LED_R, 255-remain);
        analogWrite(PIN_LED_G, 255);
        analogWrite(PIN_LED_B, 0);
        break;
      case 3: 
        analogWrite(PIN_LED_R, 0);
        analogWrite(PIN_LED_G, 255);
        analogWrite(PIN_LED_B, remain);
        break;
      case 4: 
        analogWrite(PIN_LED_R, 0);
        analogWrite(PIN_LED_G, 255-remain);
        analogWrite(PIN_LED_B, 255);
        break;
      case 5: 
        analogWrite(PIN_LED_R, remain);
        analogWrite(PIN_LED_G, 0);
        analogWrite(PIN_LED_B, 255);
        break;
    }
  }
  else {
    // do nothing
  }
}


byte x2b( char c) {
  if (isdigit(c)) {  // 0 - 9
    return c - '0';
  } 
  else if (isxdigit(c)) { // A-F, a-f
    return (c & 0xF) + 9;
  }
}


void play_music(int code) {
  // buzzer tone
  //??????????????????
  #define C41 262  //1 Do
  #define D41 294  //2 Re
  #define E41 330  //3 Mi
  #define F41 349  //4 Fa
  #define G41 392  //5 Sol
  #define G42 415  //#5 #Sol
  #define A41 440  //6 La
  #define B41 494  //7 Si
  #define C51 523  //1 Do_h
  #define D51 587  //2 Re_h
  #define D52 622  //#2 #Re_h
  #define E51 659  //3 Mi_h
  #define F51 698  //4 Fa_h
  #define G51 784  //5 Sol_h
  #define A51 880  //6 La_h
  #define B51 988  //7 Si_h
  #define O1 000  //?????????
  //?????????????????????????????????:??????*2^(1/12)

  int scale[]={E51,D52,E51,D52,E51,B41,D51,C51,A41,O1,E41,A41,B41,E41,G42,B41,C51,E41,E51,D52,E51,D52,E51,B41,D51,C51,A41,O1,E41,A41,B41,E41,C51,B41,A41};  //??????????????????
  int times[]={1,1,1,1,1,1,1,1,3,0,1,1,3,1,1,1,3,1,1,1,1,1,1,1,1,1,3,0,1,1,3,1,1,1,3};    //????????????,????????????????????????
  int length;  
  
  length = sizeof(scale)/sizeof(scale[0]);  //???????????????????????????
  for(int i=0;i<length;i++) {
    tone(PIN_BUZZER, scale[i]);

    // ?????????????????????????????????0,??????????????????
    if(times[i]>0) {
      delay(times[i]*250);
    }
    else {
      noTone(PIN_BUZZER);
      delay(250);
    }
    noTone(PIN_BUZZER);
    delay(0);
  }
}
