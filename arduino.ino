#include <WiFi.h>
#include <DNSServer.h>

// webserver init
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WiFiServer server(80);

// sensor variables + state
int sensorPin = 35;
int sensorValue = 0;
int calibration;
int arr[256];
int plus_ancien = 0;
int mm = 0;
float accel_glob = 0.5;
float accel_max = 0.7;
int flecheg = 0;
int fleched = 0;
boolean etat = true;
int ledstat = 0;

// timer init
hw_timer_t * timer = NULL;

// html var
String responseHTML = "";
String json;


void setup() {
  //init serial debug
  Serial.begin(115200);

  // init pin
  pinMode(14, INPUT_PULLUP); // btn1
  pinMode(15, INPUT_PULLUP); // btn2
  pinMode(LED_BUILTIN, OUTPUT); // led builtin
  pinMode(16, OUTPUT); // barre du milieu
  pinMode(17, OUTPUT); // fleche gauche
  pinMode(18, OUTPUT); // flèche droite

  // init sound meter history
  for (int i = 0; i < 256; i = i + 1) {
    arr[i] = 0;
  }
  // calibrating sound meter
  calibration = analogRead(sensorPin);
  Serial.println("Calibration " + String(calibration));

  // init wifi hospot + dns spoofer
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Projet Karting");
  dnsServer.start(DNS_PORT, "*", apIP);
  server.begin();

  // timer clignotant init
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  Serial.println("timer started");

  //button interrupt
  attachInterrupt(digitalPinToInterrupt(14), onButtonPress1, RISING);
  attachInterrupt(digitalPinToInterrupt(15), onButtonPress2, RISING);
}

// fonction blinker update
void onTimer() {
  etat = !etat;
  //Serial.println(etat);
  if (etat == true) {
    if (fleched == 1) {
      //turn on right arrow
      digitalWrite(16, HIGH);
      digitalWrite(17, HIGH);
      digitalWrite(18, LOW);
    } if (flecheg == 1) {
      //turn on left arrow
      digitalWrite(16, HIGH);
      digitalWrite(18, HIGH);
      digitalWrite(17, LOW);
    }
  }
  if (etat == false) {
    //turn of all led
    digitalWrite(16, LOW);
    digitalWrite(17, LOW);
    digitalWrite(18, LOW);
  }

}

// fonction button interrupt
void onButtonPress1() {
  noInterrupts();
  flecheg = 0;
  Serial.println("button pressed 1");
  if (fleched == 1) {
    fleched = 0;
  } else {
    fleched = 1;
  }
  delay(500);
  interrupts();
}
// fonction button interrupt
void onButtonPress2() {
  noInterrupts();
  fleched = 0;
  Serial.println("button pressed 2");
  if (flecheg == 1) {
    flecheg = 0;
  } else {
    flecheg = 1;
  }
  delay(500);
  interrupts();
}


void loop() {

  // sound meter read
  sensorValue = analogRead(sensorPin);
  mm = mm + abs(sensorValue - calibration) - arr[plus_ancien];
  arr[plus_ancien] = abs(sensorValue - calibration);
  plus_ancien = plus_ancien + 1;
  if (plus_ancien == 256) {
    plus_ancien = 0;
  }

  // webserver
  dnsServer.processNextRequest();
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // wait end of request \n
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // respond html code and type of content to client webbrother
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // generate json array for the graph
            json = "[";
            for (int i = 0; i < 256; i++)
            {
              json = json + String(arr[i]) + ",";
            }
            json = json + "]";

            // generate html response
            responseHTML = ""
                           "<!DOCTYPE html><html><head><title>Projet Karting</title>"
                           "<meta charset='UTF-8'>"
                           "<meta http-equiv='refresh' content='2; url=/'>"
                           "</head><body>"
                           "<h1>Site du projet Karting</h1>"
                           "<p>Ceci est un portail captif</p>"
                           "<a href='/H'> Allumer l'avertisseur lumineux </a>"
                           "<a style='margin-left: 30px;' href='/L'> Eteindre l'avertissement lumineux</a>"
                           "<h3>Sonomètre</h3>"
                           + String(abs(sensorValue - calibration)) +
                           "<br/>" +
                           "<canvas id='canvas' height='255'></canvas>"
                           "<script>"
                           "x=" + json +
                           "\n"
                           "var canvas = document.getElementById('canvas');"
                           "var ctx = canvas.getContext('2d');"
                           "ctx.fillStyle = 'green';"
                           "for (const [i, value] of x.entries()) {ctx.fillRect(i,(3300 - value)/15, 5, 5);}"
                           "\n"
                           "</script>"
                           "<h3> Mouvement</h3>"
                           "<p>Accélération :</p>"
                           + String(accel_glob) +
                           "<p>Accélération maximale  :</p>"
                           + String(accel_max) +
                           "<br/>"
                           "<a href='/G'> Allumer la fleche gauche </a>"
                           "<a style='margin-left: 30px;' href='/D'> Allumer la fleche droit </a>"
                           "<br/>"
                           "<a style='margin-left: 150px;' href='/E'> Eteindre les fleches </a>"
                           "</body></html>";
            // send html to client
            client.print(responseHTML);
            break;

            // append client request to currentLine
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // builtin Led - test /H & /L route
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);
        }

        //arrow /D /G /E route
        if (currentLine.endsWith("GET /D")) {
          //Serial.println("fleche D");
          fleched = 1;
          flecheg = 0;
        }
        if (currentLine.endsWith("GET /G")) {
          //Serial.println("fleche G");
          flecheg = 1;
          fleched = 0;
        }
        if (currentLine.endsWith("GET /E")) {
          //Serial.println("fleche low");
          flecheg = 0;
          fleched = 0;
        }
      }
    }
    client.stop();
  }

  // small delay to keep cpu level low
  delay(50);
}
