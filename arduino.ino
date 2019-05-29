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
float accel_glob = 0.0;
float accel_max = 0.0;
int flecheg = 0;
int fleched = 0;
boolean etat = true;
int ledstat = 0;
int indiclum = 0;
const int axeX = 32 ;
const int axeY = 33 ;
const int axeZ = 34 ;
float calib_value = 0.0033; //Constante de calibration
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
  pinMode(4, INPUT_PULLUP); // btn2
  pinMode(15, INPUT_PULLUP); // btn3
  pinMode(25, OUTPUT); //avert
  //pinMode(LED_BUILTIN, OUTPUT); // led builtin
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

}

// fonction blinker update
void onTimer() {
  etat = !etat;
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
    //turn off all led
    digitalWrite(16, LOW);
    digitalWrite(17, LOW);
    digitalWrite(18, LOW);
  }
}


void loop() {

  //Serial.println(String(digitalRead(15)));
  // sound meter read
  sensorValue = analogRead(sensorPin);
  //mm = mm + abs(sensorValue - calibration) - arr[plus_ancien];
  // put value into array
  arr[plus_ancien] = abs(sensorValue - calibration);
  plus_ancien = plus_ancien + 1;
  if (plus_ancien == 256) {
    plus_ancien = 0;
  }


  //partie accéléromètre
  
  //float accel_max = 0; //Variable où l'on stock la valeur maximale enregistrée
  float accel_x = calib_value * analogRead(axeX) - 1; //Assignation de l'axe X à l'entrée A2 de l'Arduino et stockage de la valeur dans la variable "accel_x"
  float accel_y = calib_value * analogRead(axeY) - 1; //Assignation de l'axe Y à l'entrée A1 de l'Arduino et stockage de la valeur dans la variable "accel_y"
  float accel_z = calib_value * analogRead(axeZ) - 1; //Assignation de l'axe Z à l'entrée A0 de l'Arduino et stockage de la valeur dans la variable "accel_z"
  float accel_x_abs = abs(accel_x / calib_value);
  float accel_y_abs = abs(accel_y / calib_value);
  //float accel_glob = 0;

  //On calcule la norme de l'acélération sur X et Y
  accel_glob = (sqrt(sq(accel_x_abs) + sq(accel_y_abs)));
  //Serial.println(accel_glob);

  if (accel_glob >= accel_max) {
    accel_max = accel_glob;
  } //On compare les valeurs de l'axe X avec la dernière valeur max stocké dans la variable "accel_xmax"

  //buttons
  if (digitalRead(14) == 0) {
    // button right arrow
    Serial.println("button pressed-right");
    // turn off left arrow
    flecheg = 0;
    // turn off on if led is on
    if (fleched == 1) {
      fleched = 0;
    }
    // turn on on if led is off
    else {
      fleched = 1;
    }
    delay(400);
  }
  // same with the left arrow
  if (digitalRead(4) == 0) {
    Serial.println("button pressed-left");
    // turn off right arrow
    fleched = 0;
    if (flecheg == 1) {
      flecheg = 0;
    } else {
      flecheg = 1;
    }
    delay(400);
  }

  if (digitalRead(15) == 0) {
    if (indiclum == 0) {
      digitalWrite(25, HIGH);
      indiclum = 1;
    } else {
      digitalWrite(25, LOW);
      indiclum = 0;
    }
    delay(400);
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
                           "<a style='margin-left: 30px;' href='/D'> Allumer la fleche droite</a>"
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
          //digitalWrite(LED_BUILTIN, HIGH);
          indiclum = 1;
          digitalWrite(25, HIGH);
        }
        if (currentLine.endsWith("GET /L")) {
          //digitalWrite(LED_BUILTIN, LOW);
          indiclum = 0;
          digitalWrite(25, LOW);
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
