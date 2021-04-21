/*
 * Small Program to Simulate a Numpad using a 2.4" TFT Touchscreen
 * Program does not act as an USB HID Device
 * 
 * Note:
 * This version is complete with styling and numbers,
 * if you want the smaller version get the "numpad-layout" program
 * from https://github.com/williamtavares/Arduino-Uno-NumPad
 * 
 * Enjoy!
*/

#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <MCUFRIEND_kbv.h>

//wifi and MQTT necessities
#include <WiFi.h>
#include <PubSubClient.h> //pio lib install "knolleary/PubSubClient"

// OTA necessities
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

//define router and broker
#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"

#define MQTT_SERVER   "192.168.1.2" // could change if the setup is moved
#define MQTT_PORT     1883

bool connected;
bool wifi = true;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// OTA
AsyncWebServer server(80);

//define callback method
void callback(char *topic, byte *message, unsigned int length);

// function for establishing wifi connection, do not touch
void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


// Touch display defines
#define LCD_RD  2  //LED
#define LCD_WR  4
#define LCD_RS 15  //or LCD_CD - hard-wired to GPIO35
#define LCD_CS 33  //hard-wired to A3 (GPIO34)
#define LCD_RST 32 //hard-wired to A4 (GPIO36)

#define LCD_D0 12
#define LCD_D1 13
#define LCD_D2 26
#define LCD_D3 25
#define LCD_D4 17
#define LCD_D5 16
#define LCD_D6 27
#define LCD_D7 14

MCUFRIEND_kbv tft;

#define DO 23   // pin that opens the door to the key

#define YP LCD_CS  // LCD_CS
#define XM LCD_RS  // LCD_RS
#define YM LCD_D1  // LCD_D1
#define XP LCD_D0  // LCD_D0

#define TS_MINX 941
#define TS_MINY 896
#define TS_MAXX 111
#define TS_MAXY 24

//Color Definitons
#define BLACK     0x0000
#define BLUE      0x001F
#define GREY      0xCE79
#define LIGHTGREY 0xDEDB
#define NEONGREEN 0x07E0

#define MINPRESSURE 100
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
// Pins A2-A6

#include "TouchScreen_kbv.h"         //my hacked version
#define TouchScreen TouchScreen_kbv
#define TSPoint     TSPoint_kbv

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 364); 

//Size of key containers 60px
#define BOXSIZE 60

//text width
#define textsize 40

// color scheme
uint16_t tbgcolor = BLACK;
uint16_t boxcolor = NEONGREEN;

// input en correcte code
byte code[4] = {1, 2, 3, 4};
byte input[4] = {11, 11, 11, 11};

// positie van de input (0-3)
byte pos;

// aantal pogingen
byte attempts = 3;

// positie van de input display
//double DispCursorX;
double firstRowCursorY;

// extra variabele voor correcte werking
bool isAtEnd = false;
bool isTouching = false;

//2.4 = 240 x 320
//Height 319 to fit on screen

//MCUFRIEND_kbv tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

  //Container variables for touch coordinates
  int X, Y, Z;

  
  //----- Screen reference positions -----//
  //Screen height without hidden pixel
  double tHeight = tft.height()-1;
  //Centering the mid square
  double center = (tft.width()/2)-(BOXSIZE/2);
  //Space between squares
  double padding = 5;
  //Position of squares to the left and right of center
  double fromCenter = BOXSIZE + padding;
  //Second row Y-Axis position
  double secondRow = BOXSIZE + padding;
  //Third row Y-Axis position
  double thirdRow = secondRow + BOXSIZE + padding;
  //Fourth row Y-Axis position
  double fourthRow = thirdRow + BOXSIZE + padding;
  //Fifth row Y-Axis position
  double fifthRow = fourthRow + BOXSIZE + padding;
  //Y-Axis align for all squares
  double verticalAlign = (tHeight-((BOXSIZE * 5)+(padding * 4)))/2;
  //Left column starting x posision
  double leftColPositionX = center - fromCenter;
  //Mid column starting x posision
  double midColPositionX = center;
  //Right column starting x posision
  double rightColPositionX = center + fromCenter;

void createButtons(){
  //(initial x,initial y,width,height,color)
  double secondRowVertialAlign = secondRow + verticalAlign;
  double thirdRowVertialAlign = thirdRow + verticalAlign;
  double fourthRowVertialAlign = fourthRow + verticalAlign;
  double fifthRowVertialAlign = fifthRow + verticalAlign;

  /***Draw filled squares with specified dimensions and position***/
  //First Row
  //tft.fillRect(leftColPositionX, verticalAlign, BOXSIZE, BOXSIZE, GREY);
  tft.fillRect(leftColPositionX, verticalAlign, (3*BOXSIZE)+(2*padding), BOXSIZE, tbgcolor);
  //tft.fillRect(rightColPositionX, verticalAlign, BOXSIZE, BOXSIZE, GREY);
  
  //Second Row
  tft.fillRect(leftColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(midColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(rightColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);

  //Third Row
  tft.fillRect(leftColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(midColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(rightColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);

  //Fourth Row
  tft.fillRect(leftColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(midColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(rightColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);

  //Fifth Row
  tft.fillRect(leftColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(midColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);
  tft.fillRect(rightColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, tbgcolor);

  /***Draw Borders around squares***/
  //First Row
  //tft.drawRect(leftColPositionX, verticalAlign, BOXSIZE, BOXSIZE, BLACK);
  tft.drawRect(leftColPositionX, verticalAlign, (3*BOXSIZE)+(2*padding), BOXSIZE, boxcolor);
  //tft.drawRect(rightColPositionX, verticalAlign, BOXSIZE, BOXSIZE, BLACK);

  //Second Row
  tft.drawRect(leftColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(midColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(rightColPositionX, secondRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);

  //Third Row
  tft.drawRect(leftColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(midColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(rightColPositionX, thirdRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);

  //Fourth Row
  tft.drawRect(leftColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(midColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(rightColPositionX, fourthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);

  //Fifth Row
  tft.drawRect(leftColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(midColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
  tft.drawRect(rightColPositionX, fifthRowVertialAlign, BOXSIZE, BOXSIZE, boxcolor);
}

void insertNumbers(){
  //Centers text horizontally on all three columns
  double leftColCursorX   = leftColPositionX +(BOXSIZE/3);
  double midColCursorX    = midColPositionX  +(BOXSIZE/3);
  double rightColCursorX  = rightColPositionX+(BOXSIZE/3);
  //Centers text horizontally on all four rows
  double firstRowCursorY  = verticalAlign+(BOXSIZE/4);
  double secondRowCursorY = secondRow + firstRowCursorY;
  double thirdRowCursorY  = thirdRow  + firstRowCursorY;
  double fourthRowCursorY = fourthRow + firstRowCursorY;
  double fifthRowCursorY = fifthRow + firstRowCursorY;

  tft.setTextSize(4);
  tft.setTextColor(NEONGREEN);
  
  //Insert Number 1
  tft.setCursor(leftColCursorX,secondRowCursorY);
  tft.println("1");

  //Insert Number 2
  tft.setCursor(midColCursorX,secondRowCursorY);
  tft.println("2");

  //Insert Number 3
  tft.setCursor(rightColCursorX,secondRowCursorY);
  tft.println("3");

  //Insert Number 4
  tft.setCursor(leftColCursorX,thirdRowCursorY);
  tft.println("4");

  //Insert Number 5
  tft.setCursor(midColCursorX,thirdRowCursorY);
  tft.println("5");

  //Insert Number 6
  tft.setCursor(rightColCursorX,thirdRowCursorY);
  tft.println("6");

  //Insert Number 7
  tft.setCursor(leftColCursorX,fourthRowCursorY);
  tft.println("7");

  //Insert Number 8
  tft.setCursor(midColCursorX,fourthRowCursorY);
  tft.println("8");

  //Insert Number 9
  tft.setCursor(rightColCursorX,fourthRowCursorY);
  tft.println("9");

  //Insert DEL Character
  tft.setCursor(leftColCursorX-15,fifthRowCursorY+5);
  tft.setTextSize(3);
  tft.println("DEL");
  tft.setTextSize(4);

  //Insert Number 0
  tft.setCursor(midColCursorX,fifthRowCursorY);
  tft.println("0");

  //Insert OK Character
  tft.setCursor(rightColCursorX-5,fifthRowCursorY+5);
  tft.setTextSize(3);
  tft.println("OK");
  tft.setTextSize(4);
}

void retrieveTouch()
{
    digitalWrite(13, HIGH); 
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);

    //If sharing pins, you'll need to fix the directions of the touchscreen pins
    pinMode(XM, OUTPUT); 
    pinMode(YP, OUTPUT); 
  
    //Scale from 0->1023 to tft.width
    X = map(p.x, TS_MAXX, TS_MINX, 0, tft.width());
    Y = map(p.y, TS_MAXY, TS_MINY, 0, tft.height());
    Z = p.z;
}

void openDoor(){
  digitalWrite(DO, LOW);
  delay(2000);
  digitalWrite(DO, HIGH); 
}

void gameOver(){
    tft.setTextSize(8);
    tft.fillScreen(BLACK);
    tft.setCursor(leftColPositionX,secondRow + verticalAlign);
    tft.println("GAME");
    tft.setCursor(leftColPositionX,thirdRow + verticalAlign);
    tft.println("OVER");
    delay(1000);
    tft.fillScreen(BLACK);
    delay(1000);
    gameOver();
}

void fail(){
  if(attempts > 1){
    attempts--;
    tft.fillScreen(BLACK);
    tft.setCursor(leftColPositionX,secondRow + verticalAlign);
    tft.println(attempts);
    tft.setCursor(leftColPositionX,thirdRow + verticalAlign);
    if(attempts > 1) {tft.println("ATTEMPTS");} else {tft.println("ATTEMPT");}
    tft.setCursor(leftColPositionX,fourthRow + verticalAlign);
    tft.println("LEFT");
    
    delay(4000);
    
    tft.fillScreen(BLACK);
    tft.drawRect(leftColPositionX + 25 + textsize*pos-5, firstRowCursorY-5, (textsize/2)+10, textsize, boxcolor);
    createButtons();
    insertNumbers(); 
    pos = 0;
    firstRowCursorY  = verticalAlign+(BOXSIZE/4);
    tft.setCursor(leftColPositionX + 25 + textsize*pos,firstRowCursorY);
    Serial.println(F("Press any button on the TFT screen: "));
    
  } else {
    // GAME OVER
    openDoor();
    gameOver();
  }
}

void succes(){
  // SUCCESS 
  tft.fillScreen(BLACK);
  tft.setCursor(35,thirdRow + verticalAlign);
  tft.println("SUCCESS");
  openDoor();
  delay(6000);
  setup(); 
}

void codeCheck(){
  bool match = true;
  //input vergelijken met code
  for(int i=0; i<4; i++){
    Serial.print(input[i]);
    if(input[i] != code[i]){
      match = false;
    }
  }
  Serial.println();

  if(match){
    succes();
  } else {
    fail();  
  } 
}

void setup_disp(){
  tft.reset();  
  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  tft.setRotation(0);

  //Background color
  tft.fillScreen(BLACK);

  // cursor box
  tft.drawRect(leftColPositionX + 25 + textsize*pos-5, firstRowCursorY-5, (textsize/2)+10, textsize, boxcolor);
  
  createButtons();
  insertNumbers();

  //DispCursorX  = leftColPositionX + 25;
  pos = 0;
  firstRowCursorY  = verticalAlign+(BOXSIZE/4);
  tft.setCursor(leftColPositionX + 25 + textsize*pos,firstRowCursorY);
  Serial.println(F("Press any button on the TFT screen: "));
}

// callback function, only used when receiving messages
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // When receiving a message on "esp32/+/control" a check should be executed

  // If a message is received on the topic esp32/control, you check if the message is either "start" or "stop" (or "reset").
  // Changes the state according to the message
  if (String(topic) == "esp32/alohomora/control")
  {
    if(messageTemp.equals("open")){openDoor();}
    if(messageTemp.equals("AddAttempt")){attempts++;}
    if(messageTemp.equals("ready")){
      espClient.stop();
      delay(100);
      WiFi.disconnect(true, true);
      wifi = false;
      Serial.println("Disconnected");
      setup_disp();
    }
    if(messageTemp.equals("start")){setup_disp();}
  }
  if (String(topic) == "esp32/alohomora/code/1"){code[0] = (int) message[0];}
  if (String(topic) == "esp32/alohomora/code/2"){code[1] = (int) message[0];}
  if (String(topic) == "esp32/alohomora/code/3"){code[2] = (int) message[0];}
  if (String(topic) == "esp32/alohomora/code/4"){code[3] = (int) message[0];}
}

void setup() {

  //Setup gedeelte voor Wifi en MQTT
  connected = false;
  //setup wifi
  //Serial.begin(115200);
  Serial.begin(9600);
  
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  //setup voor OTA
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  
  //Setup gedeelte voor display
  pinMode(DO, OUTPUT); 
  digitalWrite(DO, HIGH); 
    
  //Serial.begin(9600);
  
  tft.reset();  
  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  tft.setRotation(0);

  //Background color
  tft.fillScreen(BLACK);

  // cursor box
  tft.drawRect(leftColPositionX + 25 + textsize*pos-5, firstRowCursorY-5, (textsize/2)+10, textsize, boxcolor);
  
  createButtons();
  insertNumbers();

  //DispCursorX  = leftColPositionX + 25;
  pos = 0;
  firstRowCursorY  = verticalAlign+(BOXSIZE/4);
  tft.setCursor(leftColPositionX + 25 + textsize*pos,firstRowCursorY);
  Serial.println(F("Press any button on the TFT screen: "));
}

// function to establish MQTT connection
void reconnect()
{
  delay(10);
  // Loop until we're reconnected
  while (!connected)
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("alohomora"))
    {
      Serial.println("connected");
      connected = true;
      // Publish
      client.publish("esp32/alohomora/control", "Touchlock gestart");
      // ... and resubscribe
      client.subscribe("esp32/alohomora/+");
      Serial.print("gelukt");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  //wifi gedeelte
  if(wifi){
    if (!connected)
      {
      reconnect();
      }
    client.loop();
    AsyncElegantOTA.loop();
  }
  //display gedeelte
  retrieveTouch();
  //int boxHeightRow1 = verticalAlign + BOXSIZE;
  int boxHeightRow2 = secondRow + BOXSIZE;
  int boxHeightRow3 = thirdRow + BOXSIZE;
  int boxHeightRow4 = fourthRow + BOXSIZE;
  int boxHeightRow5 = fifthRow + BOXSIZE;
  
  if(Z > MINPRESSURE && Z < MAXPRESSURE && !isTouching){
    // set touch status to active
    isTouching = true;
    // clear if already number at this position
    tft.fillRect(leftColPositionX + 25 + textsize*pos - 5, firstRowCursorY - 5, textsize, textsize, tbgcolor);
    //Check if element clicked is in left column
    if(X > leftColPositionX && X < (leftColPositionX+BOXSIZE)){
      //Check if element clicked is in row 2
      if(Y > verticalAlign){
        if(Y < boxHeightRow2){
             //Serial.println("1");
             tft.println("1");
             input[pos]=1;
             pos++;
             delay(150); 
        }
        //Check if element clicked is in row 3
        else if(Y < boxHeightRow3){
             //Serial.println("4");
             tft.println("4");
             input[pos]=4;
             pos++;
             delay(150); 
        }
        //Check if element clicked is in row 4
        else if(Y < boxHeightRow4){
             //Serial.println("7");
             tft.println("7");
             input[pos]=7;
             pos++;
             delay(150); 
        }
        //Check if element clicked is in row 5
        else if(Y < boxHeightRow5){
             //Serial.println("*");
             if(pos > 0 && !isAtEnd){pos--;}
             input[pos]=-1;
             // tft.fillRect(DispCursorX, firstRowCursorY, textsize, textsize, tbgcolor);
             delay(150); 
        }        
      }
       //Check if element clicked is in mid column
    } else if (X > midColPositionX && X < (midColPositionX+BOXSIZE)){
      //Check if element clicked is in row 2
        if(Y > verticalAlign){
          if(Y < boxHeightRow2){
               //Serial.println("2");
               tft.println("2");
               input[pos]=2;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 3
          else if(Y < boxHeightRow3){
               //Serial.println("5");
               tft.println("5");
               input[pos]=5;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 4
          else if(Y < boxHeightRow4){
               //Serial.println("8");
               tft.println("8");
               input[pos]=8;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 5
          else if(Y < boxHeightRow5){
               //Serial.println("0");
               tft.println("0");
               input[pos]=0;
               pos++;
               delay(150); 
          }      
        }
      //Check if element clicked is in third column
    } else if (X > rightColPositionX && X < (rightColPositionX+BOXSIZE)){
        if(Y > verticalAlign){
          //Check if element clicked is in row 2
          if(Y < boxHeightRow2){
               //Serial.println("3");
               tft.println("3");
               input[pos]=3;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 3
          else if(Y < boxHeightRow3){
               //Serial.println("6");
               tft.println("6");
               input[pos]=6;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 4
          else if(Y < boxHeightRow4){
               //Serial.println("9");
               tft.println("9");
               input[pos]=9;
               pos++;
               delay(150); 
          }
          //Check if element clicked is in row 5
          else if(Y < boxHeightRow5){
               //Serial.println("#");
               //check of code matcht
               codeCheck();
               delay(150); 
          }        
        }
    }
    if(pos > 3){
      pos = 3;
      isAtEnd = true;
    } else {
      isAtEnd = false;
      tft.fillRect(leftColPositionX + 25 + textsize*pos - 5, firstRowCursorY - 5, textsize, textsize, tbgcolor);
    }
    tft.setCursor(leftColPositionX + 25 + textsize*pos,firstRowCursorY);
    tft.drawRect(leftColPositionX + 25 + textsize*pos-5, firstRowCursorY-5, (textsize/2)+10, textsize, boxcolor);
  } else {isTouching = false;} 
}