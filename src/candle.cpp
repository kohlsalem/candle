
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <WiFiUdp.h>
#include <ArduinoOTA.h>    //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

// SSID/Password
//const char* ssid = "";
//const char* password = "";

// base colour distribution for a "nice candle yellow!"
int base_red    =  100;
int base_green  =  70;
int base_blue   =  12;

// how many candles get animated?
#define NR_CANDLES  3

// Animation programs
#define PRG_FIRE    1
#define PRG_ON      2
#define PRG_BURN    3
#define PRG_FLICKER 4
#define PRG_FLUTTER 5
#define PRG_OFF     6


#define NR_FADEOUT_FLICKS  5


// General brightness of the candles, used to dim or to swicht off
byte brightnes = 0;

// Animation programm running per candle
byte program[ NR_CANDLES ] = { PRG_ON, PRG_ON, PRG_ON};

// current dim value per candle
float candle_dim[ NR_CANDLES ] = { 100, 100, 100}; // selective brightness of candle

// acceleration per cycle of dim value per candle
float acceleration[ NR_CANDLES ];

// lower boundery for dim per candle
float dim_limit[ NR_CANDLES ];

// how often shall the current animation repeat
int repeat[NR_CANDLES];

//how many cycles to do nothing
int waitcnt[NR_CANDLES] = {0, 0, 0};

//waits to use in current animation.
int waitBetweenFlicks[NR_CANDLES] = {0, 0, 0};

//waits at the end of flick should be longer to look naturally
int flickFadeoutFactor[NR_CANDLES] = {1, 1, 1};


WiFiUDP listener;
ESP8266WebServer server(80);

/*
void doFireCandle(int candle, int seconds); //forward declaration
void doFlickerCandle(int candle, int seconds);//forward declaration
void set_rgb(int candle, int r, float g, int b);//forward declaration
void set_candle(int candle, float dim);//forward declaration
*/


// set one of the candles to specified RGB considering central brightness
void set_rgb(int candle, int r, float g, int b) {
 switch (candle) {
   case 0:
   analogWrite(D1, r*brightnes*PWMRANGE/10000);
   analogWrite(D2, b*brightnes*PWMRANGE/10000);
   analogWrite(D3, g*brightnes*PWMRANGE/10000);
   break;
   case 1:
   analogWrite(D1, r*brightnes*PWMRANGE/10000);
   analogWrite(D5, b*brightnes*PWMRANGE/10000);
   analogWrite(D6, g*brightnes*PWMRANGE/10000);
   break;
   case 2:
   analogWrite(D1, r*brightnes*PWMRANGE/10000);
   analogWrite(D7, b*brightnes*PWMRANGE/10000);
   analogWrite(D8, g*brightnes*PWMRANGE/10000);
   break;
 }
}
void set_rgb_all(int r, float g, int b){
  for(int cnd=0;cnd<NR_CANDLES;cnd++){
    set_rgb(cnd,r,g,b);
  }
}
void set_candle(int candle, float dim){
 // this formula's try to dim candles nicely
 // red dimming improved the look of my animations,
 // but should be done carefully
 // if red is shared among the candles.
 set_rgb(candle, 	base_red * (0.3+0.7*(dim/100)),
 base_green*dim/100,
 base_blue * (0.5+0.5*(dim/100)));
}


// start a fire animation
void doFireCandle(int candle, int seconds){
 program[candle] = PRG_FIRE;
 candle_dim[candle] = 100;
 dim_limit[candle] = 89;
 acceleration[candle]=-0.06;
 waitBetweenFlicks[candle] = random(30)+30;
 flickFadeoutFactor[candle] = 1;
 // repeat = duration / one complete cycle of down, up and wait.
 repeat[candle] = (1000*seconds)/(2*((100-dim_limit[candle])/abs(acceleration[candle]))+waitBetweenFlicks[candle]);
}

// start a fire animation
void doFlickerCandle(int candle, int seconds){
 program[candle] = PRG_FLICKER;
 candle_dim[candle] = 100;
 dim_limit[candle] = 75;
 acceleration[candle]= -0.5;
 waitBetweenFlicks[candle] = random(100)+100;
 flickFadeoutFactor[candle] = 1.5;
 // repeat = duration / one complete cycle of down, up and wait.
 repeat[candle] = (1000*seconds)/(2*((100-dim_limit[candle])/abs(acceleration[candle]))+waitBetweenFlicks[candle]);
}

// start a fire animation
void doFlutterCandle(int candle, int seconds){
 program[candle] = PRG_FLUTTER;
 candle_dim[candle] = 100;
 dim_limit[candle] = 50;
 acceleration[candle]= -1;
 waitBetweenFlicks[candle] = random(400)+300;
 flickFadeoutFactor[candle] = 2;
 // repeat = duration / one complete cycle of down, up and wait.
 repeat[candle] = (1000*seconds)/(2*((100-dim_limit[candle])/abs(acceleration[candle]))+waitBetweenFlicks[candle]);
}

// start a "on" == constant loght animation
void doOnCandle(int candle, int seconds){
 program[candle] = PRG_ON;
 candle_dim[candle] = 100;
 dim_limit[candle] = 100;
 acceleration[candle]= 0;
 repeat[candle] = 0;
 waitcnt[candle] = 1000*seconds;
}

void doSwitchOn(){
  for(int cnd=0;cnd<NR_CANDLES;cnd++){
    doFlickerCandle(cnd,5);
    candle_dim[cnd] = 0;
    waitcnt[cnd] = 0;
    acceleration[cnd]= -0.5;
  }
}

void doSwitchOff(){


}

void handleCandle(int candle){

 // first, if we have to wait, do not touch anything
 if(waitcnt[candle]>0){
   waitcnt[candle]--;
   set_candle(candle,candle_dim[candle]); //otherwise changes in brightnes show to late
   return;
 }


 // so, no wait cycle. bummer, some work is needed.

 // if we have a acceleration !=0, we need to change the dim a little bit
 if(acceleration[candle]<0){

   // dim a little bit
   candle_dim[candle] += acceleration[candle];

   //if the candle is dark enough, reverse direction
   if(candle_dim[candle]<=dim_limit[candle]){
     acceleration[candle]*=-1;
   }

   // light up a little bit
 } else if(acceleration[candle]>0){
   candle_dim[candle] += acceleration[candle];

   // now we are back to 100%, so one dim cycle ended
   if(candle_dim[candle]>=100){
     acceleration[candle]*=-1;  // reverse accelaration again
     waitcnt[candle] = waitBetweenFlicks[candle];
     repeat[candle]--; // decrease counter
     if(repeat[candle]>0 && repeat[candle]<NR_FADEOUT_FLICKS ){
        waitcnt[candle] *= flickFadeoutFactor[candle] * (NR_FADEOUT_FLICKS - repeat[candle]);
     }
   }
 }


 switch (program[candle]) {
   case PRG_FIRE:

   // last cycle of a fire animation reached.
   if(repeat[candle]==0){
     if(random(100)>90){
       doFlickerCandle(candle,random(15));
     }else{
       doOnCandle(candle,random(10));
     }
   }

   break;
   case PRG_FLICKER:
   // last cycle of a fire animation reached.
   if(repeat[candle]==0){
     if(random(100)>90){
       doFlutterCandle(candle,random(10));
     }else{
       doOnCandle(candle,random(10));
     }
   }

   break;
   case PRG_FLUTTER:
   // last cycle of a fire animation reached.
   if(repeat[candle]==0){
     if(random(100)>90){
       doFlickerCandle(candle,random(10));
     }else{
       doOnCandle(candle,random(10));
     }
   }

   break;
   case PRG_ON:

   if(repeat[candle]==0){
     doFireCandle(candle,random(30));
   }
   break;
 }

 set_candle(candle,candle_dim[candle]);

}
// the loop function runs over and over again forever
int handlecnt = 0;

void loop() {

 if(brightnes>0){ // if we actually light up the candle

   if(handlecnt<=0){ // handle wlan every 0.5 seconds
     ArduinoOTA.handle();
     server.handleClient();
     handlecnt=500;
   }
   handlecnt--;

   // animate next step
   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     handleCandle(cnd);
   }

   delay(1);

 }else{ // if we are dark anyway, no need to wast energy, take a longer nap

   ArduinoOTA.handle();
   server.handleClient();

   // ensure everythin is switched off
   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     set_rgb(cnd,0,0,0);
   }

   delay(500);
 }
}


void setupWebServer(){
 // listen to brightnes changes
 server.on("/", []()
 {
   if (server.arg("b") != "")
   { int b = constrain((byte) server.arg("b").toInt(),0,100);

     if(b>50 && brightnes == 0) doSwitchOn();
     if(b==0  && brightnes > 0) doSwitchOff();

     brightnes = b;
   }
   String s = "{\n   \"b\":";
   s += brightnes;
   s += "\n}";

   server.send(200, "text/plain", s);
 });
 server.on("/colour", []()
 {
   if (server.arg("r") != "")
   {
     base_red = (byte) server.arg("r").toInt();
     if (base_red > 100) base_red = 100;
     if (base_red < 0) base_red = 0;
   }
   if (server.arg("g") != "")
   {
     base_green = (byte) server.arg("g").toInt();
     if (base_green > 100) base_green = 100;
     if (base_green < 0) base_green = 0;
   }
   if (server.arg("b") != "")
   {
     base_blue = (byte) server.arg("b").toInt();
     if (base_blue > 100) base_blue = 100;
     if (base_blue < 0) base_blue = 0;
   }

   server.send(200, "text/plain", "set");
 });
 server.on("/fire", []()
 {
   int seconds;
   if (server.arg("s") != "")
   {
     seconds = (byte) server.arg("s").toInt();
     if (seconds > 100) seconds = 100;
     if (seconds < 0) seconds = 0;
   }else{
     seconds=10;
   }
   String s = "{\n   \"s\":";
   s += seconds;
   s += "\n}";

   server.send(200, "text/plain", s);

   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     doFireCandle(cnd, seconds);
   }

 });
 server.on("/flicker", []()
 {
   int seconds;
   if (server.arg("s") != "")
   {
     seconds = (byte) server.arg("s").toInt();
     if (seconds > 100) seconds = 100;
     if (seconds < 0) seconds = 0;
   }else{
     seconds=10;
   }
   String s = "{\n   \"s\":";
   s += seconds;
   s += "\n}";

   server.send(200, "text/plain", s);

   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     doFlickerCandle(cnd, seconds);
   }

 });

 server.on("/flutter", []()
 {
   int seconds;
   if (server.arg("s") != "")
   {
     seconds = (byte) server.arg("s").toInt();
     if (seconds > 100) seconds = 100;
     if (seconds < 0) seconds = 0;
   }else{
     seconds=10;
   }
   String s = "{\n   \"s\":";
   s += seconds;
   s += "\n}";

   server.send(200, "text/plain", s);

   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     doFlutterCandle(cnd, seconds);
   }

 });

 server.on("/signal", []()
 {
   byte br = brightnes;
   brightnes = 100;
   for(int i=0; i<40; i++){
     for(int cnd=0; cnd<NR_CANDLES; cnd++){
       int r=0,g=0,b=0;
       while(r+b+g==0){
         if(random(100)>90)r=100; else r=0;
         if(random(100)>66)g=100; else g=0;
         if(random(100)>66)b=100; else b=0;
       }
       set_rgb(cnd, r,g,b );
     }
     delay(200);
   }
   for(int cnd=0; cnd<NR_CANDLES; cnd++){
     set_candle(cnd,candle_dim[cnd]);
   }
   brightnes = br;
   server.send(200, "text/plain", "done");

 });

 // start the webserver
 server.begin();

}

// the setup function runs once when you press reset or power the board
void setup() {
WiFiManager wifiManager;

 // after some tries, this seems to be the loweset frequency w/o flicker
 analogWriteFreq(1500);

 // switch radio on
 set_rgb_all( 0, 100, 0);

 //fetches ssid and pass from eeprom and tries to connect
 //if it does not connect it starts an access point with the specified name
 //here  "Candle"
 //and goes into a blocking loop awaiting configuration
 wifiManager.autoConnect("Candle");
 //WiFi.begin(ssid, password);

 // if wlan fails we sit and wait the watchdog ;-)
 while (WiFi.status() != WL_CONNECTED) {
   set_rgb_all( 0, 50, 0);
   delay(250);
   set_rgb_all( 0, 100, 0);
   delay(250);
 }
 set_rgb_all( 0, 0, 100);
 // the pins get initiated for output and swithced off
 pinMode(D1, OUTPUT);    // All Red
 digitalWrite(D1, LOW);
 pinMode(D2, OUTPUT);    // 1 Blue
 digitalWrite(D2, LOW);
 pinMode(D3, OUTPUT);    // 1 Green
 digitalWrite(D3, LOW);
 pinMode(D5, OUTPUT);    // 2 Blue
 digitalWrite(D5, LOW);
 pinMode(D6, OUTPUT);    // 2 Green
 digitalWrite(D6, LOW);
 pinMode(D7, OUTPUT);    // 3 Blue
 digitalWrite(D7, LOW);
 pinMode(D8, OUTPUT);    // 3 Green
 digitalWrite(D8, LOW);


 // initialize OTA update
 // in case, debug output can be placed in the stubs
 ArduinoOTA.setHostname("Candle");
 ArduinoOTA.onStart([](){});
 ArduinoOTA.onEnd([](){});
 ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){});
 ArduinoOTA.onError([](ota_error_t error){});
 ArduinoOTA.begin();

 setupWebServer();

}
