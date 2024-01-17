#include <MKRWAN.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Définition des broches pour les capteurs DHT22
#define DHTPIN_sensor_ext A1
#define DHTPIN_sensor_int A2
#define DHTTYPE DHT22 // Type DHT22

//Definition de la photocell
#define PHOTOCELL_PIN A4

//constantes pour la lecture de la batterie
const int analogPin = A0;
const int numReadings = 100; // Nombre de lectures pour la moyenne

// Def des sensors DS
#define ONE_WIRE_BUS A3 // Broche de données pour les capteurs DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Initialisation des objets DHT
DHT DHT22_Temp_int(DHTPIN_sensor_int, DHTTYPE);
DHT DHT22_Temp_ext(DHTPIN_sensor_ext, DHTTYPE);

//TPL
#define TPL5110_DELAY_PIN  2
#define TPL5110_DONE_PIN  4

//Constantes Poids
#include "HX711.h"
#define LOADCELL_DOUT_PIN  6
#define LOADCELL_SCK_PIN  7
HX711 scale;
float calibration_factor = 13660; // Ce facteur doit être ajusté selon votre balance

// Définir la broche de connexion du buzz
#define buzzerPin 5 

//Constantes Lora
LoRaModem modem;
bool connected;
int err_count;
short con;
String appEui = "0000004500230069";                   //IDs connexions Lora
String appKey = "771F47D684AA60024F42582EB99140E9";



void setup() {
  //Initialisaiton du moniteur série
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("Init");
  
  //Initialisation du TPL

  pinMode(TPL5110_DONE_PIN, OUTPUT);
  pinMode(TPL5110_DELAY_PIN, OUTPUT);

  // Initialisation des capteurs DS18B20
  sensors.begin();

  //Initialisation des catpeurs DHT
  DHT22_Temp_int.begin();
  DHT22_Temp_ext.begin();

  //Initialisation du buzzer
  pinMode(buzzerPin, OUTPUT);
  playMelody(); // Jouer une mélodie au démarrage

  //Initsialisation capteur poids

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.println(zero_factor);

  //Initialisation pour LoRa
  modem.begin(EU868);
  delay(1000);
  connected = false;
  err_count = 0;
  con = 0;
}

void loop() {

  risingEdgeDelay();
  delay(1000);
  uint16_t Poids = round(getPoids() * 100);
  delay(100);
  uint16_t Temp_DHT22_int = round(getTempInt() * 100);
  delay(100);
  uint16_t Hum_DHT22_int = round(getHumInt() * 100);
  delay(100);
  uint16_t Temp_DHT22_ext = round(getTempExt() * 100);
  delay(100);
  uint16_t Hum_DHT22_ext = round(getHumExt() * 100);
  uint16_t temp_DS18B20_N1 = round(getTemp_DS18B20_N1() * 100);
  uint16_t temp_DS18B20_N2 = round(getTemp_DS18B20_N2() * 100);
  uint16_t batterie = round(getBatterie() * 100);
  uint16_t lum = round(getLum() * 100);




  // Tentative de connexion si non connecté
  if (!connected) {
    Serial.print("Join test : ");
    Serial.println(++con);
    if (modem.joinOTAA(appEui, appKey)) {
      connected = true;
      modem.minPollInterval(60);
      Serial.println("Connected");
    } else {
      Serial.println("Connection failed");
      delay(10000);  // Attendre 10 secondes avant de réessayer
      return;  // Sortir de la fonction loop pour réessayer
    }
  }

  // Envoi des données si connecté
  if (connected) {
    modem.dataRate(5);
    modem.beginPacket();
    modem.write(Temp_DHT22_ext);
    modem.write(Hum_DHT22_ext);
    modem.write(Temp_DHT22_int);
    modem.write(Hum_DHT22_int);
    modem.write(temp_DS18B20_N1);
    modem.write(temp_DS18B20_N2);
    modem.write(batterie);
    modem.write(Poids);
    modem.write(lum);
    

    // Affichage des valeurs sur le moniteur série
    Serial.print("Poids : ");
    Serial.println((float)Poids / 100.00);
    Serial.print("Temp int : ");
    Serial.println((float)Temp_DHT22_int / 100.00);
    Serial.print("Hum int : ");
    Serial.println((float)Hum_DHT22_int / 100.00);
    Serial.print("Temp ext : ");
    Serial.println((float)Temp_DHT22_ext / 100.00);
    Serial.print("Hum ext : ");
    Serial.println((float)Hum_DHT22_ext / 100.00);
    Serial.print("Temperature DS18B20 n°1 : ");
    Serial.println((float)temp_DS18B20_N1 / 100.0);
    Serial.print("Temperature DS18B20 n°2 : ");
    Serial.println((float)temp_DS18B20_N2 / 100.0);
    Serial.print("Batterie : ");
    Serial.println((float)batterie / 100.0);
    Serial.print("lum : ");
    Serial.println((float)lum / 100.0);

    if (modem.endPacket() <= 0) {
      Serial.println("Error sending message");
      err_count++;
      if (err_count > 50) {
        connected = false;
      }
      delay(1000); // Attendre 1 sec
    } else {
      err_count = 0;
      Serial.println("Message envoyé");
      delay(3000); // Attendre 3 secondes
    }
  }
    risingEdgeDone();
    delay(600000);        //delai de 10 min
}

float getPoids(){
  float weight = scale.get_units() * 0.45 - 3.28;
  return weight;
}

void playMelody() {
  int melody[] = {
    330, 330, 330, 262, 330, 330, 330, 262, // ta ta ta ta ta ta ta ta
    330, 392, 440, 349, 330, 262, 294, 262, 330, 360  // ta ta ta ta ta ta ta ta
  };

  int noteDurations[] = {
    200, 100, 100, 100, 100, 200, 200, 200,
    200, 200, 200, 100, 100, 100, 100, 200, 200, 2000
  };

  for (int thisNote = 0; thisNote < 16; thisNote++) {
    int noteDuration = noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);
    
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    
    noTone(buzzerPin);
  }
}

float getTempInt(){
  float temp = DHT22_Temp_int.readTemperature();
  return temp;
}

float getHumInt(){
  float hum = DHT22_Temp_int.readHumidity();
  return hum;
}

float getTempExt(){
  float temp = DHT22_Temp_ext.readTemperature();
  return temp;
}

float getHumExt(){
  float hum = DHT22_Temp_ext.readHumidity();
  return hum;
}

float getTemp_DS18B20_N1() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float getTemp_DS18B20_N2() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(1);
}

float getBatterie(){
  long total = 0; // Pour stocker la somme des lectures
  short value = 0;
  int averageAnalogValue = 0;
  for (int i = 0; i < numReadings; i++) {
    total += analogRead(analogPin);
    delay(10); // Petite pause entre les lectures
  }
  // Calcul de la moyenne
  averageAnalogValue = (total / numReadings);
  // Envoi de la valeur moyenne au moniteur série
  value = map(averageAnalogValue,786, 1023, 0, 100);
  return value;
}

float getLum() {
  float Lum = analogRead(PHOTOCELL_PIN);
  return Lum;
}

void risingEdgeDelay(){
  delay(50);
  digitalWrite(TPL5110_DELAY_PIN, HIGH);
  delay(50);
  digitalWrite(TPL5110_DELAY_PIN, LOW);
}

void risingEdgeDone(){
  delay(50);
  digitalWrite(TPL5110_DONE_PIN, HIGH);
  delay(50);
  digitalWrite(TPL5110_DONE_PIN, LOW);
}