char Rev_OILER[] = "GPS-OILER-2"; // Bezeichnung der Hardware
char firmware_Vers[] = "Rev.2.8"; // Aktuelle Firmwareversion

// EEPROM I2C Addresse vom Display
#define EEPROM_I2C_ADDRESS 0x50

#include <Arduino.h>                  //
#include <Adafruit_GFX.h>             //
#include <Adafruit_SSD1306.h>         // Für das OLED
#include <TinyGPS++.h>                // Für das GPS
#include <SoftwareSerial.h>           // Wird für den Serial für den GPS-Empfänger benötigt
#include <Fonts/FreeMonoBold12pt7b.h> // Schriftart für das OLED
#include <Fonts/FreeMono9pt7b.h>      // Schriftart für das OLED
#include <EEPROM.h>                   // Für das EEPROM
#include <Wire.h>                     // Für I2C
#include <ESP8266WiFi.h>              // Für WIFI
#include <ESP8266WebServer.h>         // Für WIFI

static const int RXPin = 14;          // Für Software Serial
static const uint32_t GPSBaud = 9600; // Baudrate für Software Serial

Adafruit_SSD1306 display(128, 64); // Definition für das OLED
TinyGPSPlus gps;                   //
SoftwareSerial GPS_serial(RXPin);  // The serial connection to the GPS device

const int oilPin = 0;       // Pin an dem die Pumpe angesteuert wird
const int LedPin = 2;       // LED auf der Platine
const int regenSensor = A0; // Pin für Regensensor
const int Wlan_Reset = 12;  // Taster zum Resetten des AP, Setzt ssid auf "OILER" und Passwort wird deaktiviert
const int U_Sensor = 15;    // Spannung für den Regensensor
const int U_VCC2 = 13;      // Prüft die Spannung an Klemme 15

// Variable für die Seriellen Eingabe
String inputString = ""; // a String to hold incoming data

// Variablen die über den Serial Monitor geändert werden können
String Vari_zeit_pumpe_ein = "Zeit Pumpdauer:";
String Vari_zeit_pumpe_pause = "Zeit Pumppause:";
String Vari_oilsymbol_Zeit = "Zeit Ölsymbol:";
String Vari_zeit_bis_notbetrieb = "Zeit bis Notbetrieb:";
String Vari_Not_geschwindigkeit = "Geschwindigkeit Notbetrieb:";
String Vari_pump_nach_Regen = "Pumpstoesse nach Regen:";
String Vari_Gefahrene_km = "Gefahrene Km:";
String Vari_Gefahrene_km_Ges = "Gesamt Km:";
String Vari_pumpDistanz = "Pumpdistanz:";
String Vari_anzahl_Pump = "Anzahl Pumpimpulse:";
String Vari_minGeschwindigkeit = "Min. Geschwindigkeit:";
String Vari_tankinhalt_max = "Max.Tankinhalt:";
String Vari_Start_disp_1 = "Startbildschirm 1:";
String Vari_Start_disp_2 = "Startbildschirm 2:";
String Vari_pump_anzahl_Ges = "Pumpimpulse Gesamt:";
String Vari_rainMulti = "Regen Multiplikator:";
String Vari_geschwindigkeit = "Geschwindigkeit:";

// Benutzerdifinierte Variablen
int anzahl_Pump = 1;                 // Anzahl der Pumpimpulse je Ölung
int minGeschwindigkeit = 20;         // Mindestgeschwindigkeit zum Ölen
int oilsymbol_Zeit = 6;              // Zeit in Sec. wie lange das OilSymbol erscheint
int geschwindigkeit_Notbetrieb = 80; // Geschwindigkeit die angenommen wird wenn kein Sat-Empfang ist (Notbetrieb)
int zeit_bis_notbetrieb = 180;       // Zeit bis Notbetrieb in Sekunden
int zeit_pumpe_ein = 50;             // Zeit in ms wie lage die Pumpe eineschaltet ist
int zeit_pumpe_pause = 500;          // Zeit zwischen den einzelnen Pumpimpulsen
int TimeAPout = 5;                   // Zeit in Minuten bis sich der AP wieder abschaltet
int pump_nach_Regen = 5;             // Pumpimpulse wenn der Regenmodus abgeschaltet wird
int Start_disp_1 = 10;               // Zeit für Startdisplay 1 in Sec.
int Start_disp_2 = 10;               // Zeit für Startdisplay 2 in Sec.
int sw_Regensensor_ein = 500;        // Schwellwert Regenmodus ein
int sw_Regensensor_aus = 600;        // Schwellwert Regenmodus aus
int pumpDistanz = 2000;              // Abstand in m zwischen den einzelnen Ölungen
int tankinhalt_ml = 150;             // Tankinhalt in ml
int pumps_ml = 50;                   // Anzahl der Pumpimpulse pro ml
int init_pump_anzahl = 0;            // Anzahl der Pumpimpulse für die Initialölung
float rainMulti = 2.0;               // Multiplikator für Regenmodus
char ssid_ap[20] = "GPS-OILER";      // Die SSID
char password_ap[20] = "";           // alternativ :  = "12345678";
char ssid_ap_set[20] = "GPS-Oiler";  // Wird bei WLAN RESET gesetzt
char password_ap_set[20] = "";       // Wird bei WLAN RESET gesetzt / alternativ :  = "12345678";

// Funktionelle Variablen
boolean Factory_init = 0;          // Wird für die erste Inbetriebnahme verwendet
boolean Spuelen = false;           // Variable für Spülen
boolean extraOilen = false;        // Variable für extraSpülen
boolean regenmodus = false;        // Bestimmt ob Regenmodus aktiv ist
boolean oilsymbol = false;         // Status für die Anzeige des Oilsymbol
boolean speed_show_toogle = true;  // Geschwindigkeit Blinkt wenn kein Satempfang
boolean sat_empfang = false;       // Satempfang und Serielle Daten vom GPS-Modul vorhanden
boolean notbetrieb = false;        // Wird bei Notbetrieb gesetzt
boolean oilen = false;             // Wird gesetzt wenn die nächste Ölung ansteht
boolean Not_Oil_Init = false;      // Variable für die Initialölung wenn der Notbetrieb beginnt
boolean invert_Display = true;     // Display Invertieren oder nicht
boolean TimeAPout_set = false;     // Variable wird gesetzt wenn AP aus ist
boolean oled_akt = false;          // wird für das Aktualisieren des OLD benötigt
boolean HTML_Page = true;          // Seite des Web Interface
boolean stop_write_eeprom = false; // verhindert das Beschreiben des EEPRON Speichers
boolean dir_Display = true;        // Wird für die Auswahl benötigt ob die Richtung oder der Mittelwet des ADC angezeigt werder soll
byte pump_anzahl_soll = 0;         // Aktuelle Anzahl der Pumpimpulse
byte tankinhalt_Anzeige = 0;       // Wert für die Anzeige des Balken (Von 0 - 128)
byte sateliten_anzahl = 0;         // Anzahl der empfangenen Sateliten
byte action;                       // Wird für HTML benutzt
int richtung = 0;                  // Wird für die Anzeige der Richtung benötigt
int tankinhalt_Aktuell = 0;        // Wert des Aktuellen Tankinhalt
int tankinhalt_max = 150;          // Anzahl der Pumpimpulse die benötigt werden um den Tank zu leeren. Wird später berechnet.
int regenValue = 0;                // Wert der am Regensensor ermittelt wird
int regensensorMittelwert = 0;     // Ermittelter Mittelwert des Regensensor
int notbetrieb_variable = 0;       // Variable zur ermittlung des Notbetrieb.
int Gefahrene_km_Temp = 0;         // wird für das Speichern der Km benötigt
int Gefahrene_km_display = 0;      // Gefahrene km für das Display
int tankinhalt_ml_akt = 0;         // Aktueller Tankinhalt
int oilsymbol_Aus = 0;             // Variable für die Berechnung für das Anzeigenzeit des Ölsymbol
float gefahrene_m = 0;             // Berechnete Strecke in Meter. Wird zum Berechnen der Ölabstände benötigt
float Gefahrene_km = 0;            // Für die Anzeige der gefahrenen Strecke
float geschwindigkeit = 0;         // Aktuelle Geschwindigkeit
float Next_Loop = 0;               // Wird im LOOP zur steuerung benötigt
float variable_pumpe_ein = 0;      // Variable zum steuern der Pumpe
float variable_pumpe_pause = 0;    // Variable zum steuern der Pumpe
float serial_data = 5000;          // Wird  zum erkenne der Seriellen Daten vom GPS benutzt
float pumpe_aus = 0;               // Variable zum steuern der Pumpe
float TimeAPoutmillis;             // Variable für die Abschaltung des AP
long Gefahrene_km_Ges = 10;        // Km Gesamt
long pump_anzahl_Ges = 0;          // Pumpimpulse Gesamt

// Speicheraddressen der Benutzerdifinierten Variablen
int addr_Factory_init = 0;                 // Wird für die Initiallisierung benötigt
int addr_anzahl_Pump = 50;                 // Adresse für Pumpanzahl
int addr_tankinhalt_ml = 100;              //
int addr_pumps_ml = 150;                   //
int addr_tankinhalt_ml_akt = 200;          //
int addr_pumpDistanz = 250;                // Abstand in m zwischen den einzelnen Ölungen
int addr_minGeschwindigkeit = 300;         // Mindestgeschwindigkeit zum Ölen
int addr_oilsymbol_Zeit = 350;             // Zeit des Ölsymbol in
int addr_tankinhalt_max = 400;             // Anzahl der Pumpimpulse die benötigt werden um den Tank zu leeren
int addr_geschwindigkeit_Notbetrieb = 450; // Geschwindigkeit die angenommen wird wenn kein Sat-Empfang ist (Notbetrieb)
int addr_zeit_bis_notbetrieb = 500;        // Zeit bis Notbetrieb in ms
int addr_zeit_pumpe_ein = 550;             // Zeit in ms wie lage die Pumpe eineschaltet ist
int addr_zeit_pumpe_pause = 600;           // Zeit zwischen den einzelnen Pumpimpulsen
int addr_sw_Regensensor_ein = 650;         //
int addr_sw_Regensensor_aus = 700;         //
int addr_regenmodus = 750;                 //
int addr_rainMulti = 800;                  //
int addr_TimeAPout = 850;                  // Zeit bis zum abschalter des AP
int addr_init_pump_anzahl = 900;           //
int addr_gefahrene_m = 1100;               // Wird zur Berechnung der Zurückgelegten entfernung benötigt
int addr_Start_disp_1 = 1150;              //
int addr_Start_disp_2 = 1200;              //
int addr_pump_nach_Regen = 1350;           // Pumpimpulse wenn der Regenmodus abgeschaltet wird
int addr_ssid_ap = 1500;                   // Name des AP
int addr_password_ap = 1800;               // Password AP
int addr_Gefahrene_km = 2000;              //
int addr_Gefahrene_km_Ges = 2100;          //
int addr_Gefahrene_km_Display = 2200;      //
int addr_tankinhalt_Aktuell = 2300;        //
int addr_pump_anzahl_Ges = 2400;           //
int addr_dir_Display = 2500;               //

// Variablen für die Umwandlung in char in void update_Display ()
char stringSpeed[10];                // Create a character array of 10 characters
char stringDir[10];                  // Create a character array of 10 characters
char stringGefahrene_km_display[10]; // Create a character array of 10 characters
char stringsateliten_anzahl[10];     // Create a character array of 10 characters
char StringFaktor[10];               // Create a character array of 10 characters
char stringMittelwert[10];           // Create a character array of 10 characters

// WiFi
byte my_WiFi_Mode = 2;              // WIFI_STA = 1 = Workstation  WIFI_AP = 2  = Accesspoint
IPAddress local_ip(192, 168, 4, 1); // Die Festgelegte IP Adresse des AP

WiFiServer server(80);
WiFiClient client;

#define MAX_PACKAGE_SIZE 2048
char HTML_String[5000];
char HTTP_Header[150];

#define ACTION_SET_WIFI 1
#define ACTION_SET_OILEN 2
#define ACTION_SET_TANK 3
#define ACTION_SET_PAGE 4
#define ACTION_SET_NOTBETRIEB 5
#define ACTION_SET_PUMPE 6
#define ACTION_SET_ANZEIGEZEIT_DISP 7
#define ACTION_SET_TANK_RESET 8
#define ACTION_LIES_AUSWAHL 9
#define ACTION_LIES_VOLUME 10
#define ACTION_SET_DATE_TIME 11
#define ACTION_SET_SPUELEN_EIN 12
#define ACTION_SET_SPUELEN_AUS 13
#define ACTION_SET_OILSYMBOL_Zeit 14
#define ACTION_SET_REGENMODUS 15
#define ACTION_SET_APaus 18
#define ACTION_SET_EXTRAOILEN_EIN 19
#define ACTION_SET_EXTRAOILEN_AUS 20
#define ACTION_SET_ACTION_SET_INIT_PUMP 21

char tmp_string[20];

unsigned long ulReqcount;

// Auflistung der Funktionen
int read_int(int address);
void update_km();
void update_Oelen();
void update_GPS_Data();
void update_Display();
void Oelen();
void update_Rain();
void start_Disp_1();
void start_Disp_2();
void set_Eeprom();
void reed_Eeprom();
void WiFi_Start_AP();
void write_char_array(int address, char *value, int length, int i2c_address);
void read_char_array(int address, int length, int i2c_address, char *rcvData);
void write_int(int address, unsigned int value);
void set_Faktory();
void AP_Set_Faktory();
void WiFI_Traffic();
void Status_Serial();
long read_long(int address);
void write_long(int address, long value);
void strcatf(char *tx, float i);
void oled_start();

// WIFI
int Pick_Parameter_Zahl(const char *par, char *str);
int Find_Start(const char *such, const char *str);
int Find_End(const char *such, const char *str);
int Pick_Dec(const char *tx, int idx);
int Pick_N_Zahl(const char *tx, char separator, byte n);
void set_colgroup1(int ww);
void make_HTML01();
void make_HTML02();
void send_not_found();
void strcati(char *tx, int i);
void send_HTML();
void exhibit(const char *tx, int v);
void Pick_Text(char *tx_ziel, char *tx_quelle, int max_ziel);
void set_colgroup(int w1, int w2, int w3, int w4, int w5);
void strcati2(char *tx, int i);
char HexChar_to_NumChar(char c);

// Hier ist das Symbol der Ölkanne
static const unsigned char PROGMEM oilcan[] =
    {
        B00110000,
        B00000000,
        B01001100,
        B00000000,
        B10011011,
        B00000000,
        B10110011,
        B00000000,
        B01100001,
        B10000000,
        B11000000,
        B11100000,
        B01100000,
        B00011100,
        B00110000,
        B00000110,
        B00011000,
        B01111110,
        B00001100,
        B11000000,
        B00000111,
        B10000000,
        B00000000,
        B00000100,
        B00000000,
        B00001110,
        B00000000,
        B00011111,
        B00000000,
        B00011111,
        B00000000,
        B00001100,
};
// Hier ist das Symbol für Satelitenempfang
static const unsigned char PROGMEM satelite[] =
    {
        B00000000,
        B01110000,
        B01000011,
        B10110000,
        B00101100,
        B00111000,
        B00011000,
        B00101000,
        B00011000,
        B01001000,
        B00100100,
        B01001000,
        B01000010,
        B10001000,
        B01000001,
        B00001000,
        B10000010,
        B00010000,
        B10000100,
        B00110000,
        B01110000,
        B00110000,
        B01100000,
        B01111000,
        B00011111,
        B11111100,
        B00000001,
        B11111110,
        B00000001,
        B11111111,
        B00000001,
        B11111111,

};
// Hier ist das Symbol für Regen
static const unsigned char PROGMEM raining[] =
    {
        B00000011,
        B10000000,
        B00001100,
        B01100000,
        B00111000,
        B00111100,
        B01000000,
        B01000010,
        B01000000,
        B10000010,
        B01000000,
        B00000110,
        B00100000,
        B00000100,
        B00011100,
        B00111000,
        B00000111,
        B11100000,
        B00010000,
        B10001000,
        B00001000,
        B01000100,
        B00000100,
        B00000010,
        B00000000,
        B00010000,
        B00001001,
        B00001000,
        B00000100,
        B10000100,
        B00000010,
        B01000010,
};

// Serielle Eingaben
void evaluate(String input)
{
  // Pumpzeit
  if (input.startsWith(Vari_zeit_pumpe_ein))
  {
    input.replace(Vari_zeit_pumpe_ein, "");
    int value = input.toInt();
    Serial.print(Vari_zeit_pumpe_ein);
    Serial.println(value);
    zeit_pumpe_ein = value;
    write_int(addr_zeit_pumpe_ein, zeit_pumpe_ein);
  }
  // Pumppause
  else if (input.startsWith(Vari_zeit_pumpe_pause))
  {
    input.replace(Vari_zeit_pumpe_pause, "");
    int value = input.toInt();
    Serial.print(Vari_zeit_pumpe_pause);
    Serial.println(value);
    zeit_pumpe_pause = value;
    write_int(addr_zeit_pumpe_pause, zeit_pumpe_pause);
  }
  // Ölsymbol Anzeigezeit
  else if (input.startsWith(Vari_oilsymbol_Zeit))
  {
    input.replace(Vari_oilsymbol_Zeit, "");
    int value = input.toInt();
    Serial.print(Vari_oilsymbol_Zeit);
    Serial.println(value);
    oilsymbol_Zeit = value;
    write_int(addr_oilsymbol_Zeit, oilsymbol_Zeit);
  }
  // Zeit bis Notbetrieb
  else if (input.startsWith(Vari_zeit_bis_notbetrieb))
  {
    input.replace(Vari_zeit_bis_notbetrieb, "");
    int value = input.toInt();
    Serial.print(Vari_zeit_bis_notbetrieb);
    Serial.println(value);
    zeit_bis_notbetrieb = value;
    write_int(addr_zeit_bis_notbetrieb, zeit_bis_notbetrieb);
  }
  // Geschwindigkeit Notbetrieb
  else if (input.startsWith(Vari_Not_geschwindigkeit))
  {
    input.replace(Vari_Not_geschwindigkeit, "");
    int value = input.toInt();
    Serial.print(Vari_Not_geschwindigkeit);
    Serial.println(value);
    geschwindigkeit_Notbetrieb = value;
    write_int(addr_geschwindigkeit_Notbetrieb, geschwindigkeit_Notbetrieb);
  }
  // Pumpimpulse nach Regenmodus
  else if (input.startsWith(Vari_pump_nach_Regen))
  {
    input.replace(Vari_pump_nach_Regen, "");
    int value = input.toInt();
    Serial.print(Vari_pump_nach_Regen);
    Serial.println(value);
    pump_nach_Regen = value;
    write_int(addr_pump_nach_Regen, pump_nach_Regen);
  }
  // Pumpimpulse
  else if (input.startsWith(Vari_anzahl_Pump))
  {
    input.replace(Vari_anzahl_Pump, "");
    int value = input.toInt();
    Serial.print(Vari_anzahl_Pump);
    Serial.println(value);
    anzahl_Pump = value;
    write_int(addr_anzahl_Pump, anzahl_Pump);
  }
  // Min Gescchwindigkeit
  else if (input.startsWith(Vari_minGeschwindigkeit))
  {
    input.replace(Vari_minGeschwindigkeit, "");
    int value = input.toInt();
    Serial.print(Vari_minGeschwindigkeit);
    Serial.println(value);
    minGeschwindigkeit = value;
    write_int(addr_minGeschwindigkeit, minGeschwindigkeit);
  }
  // Gefahrene Km
  else if (input.startsWith(Vari_Gefahrene_km))
  {
    input.replace(Vari_Gefahrene_km, "");
    long value = input.toInt();
    Serial.print(Vari_Gefahrene_km);
    Serial.println(value);
    Gefahrene_km = value;
    write_long(addr_Gefahrene_km, Gefahrene_km);
  }
  // Gesamt Km
  else if (input.startsWith(Vari_Gefahrene_km_Ges))
  {
    input.replace(Vari_Gefahrene_km_Ges, "");
    long value = input.toInt();
    Serial.print(Vari_Gefahrene_km_Ges);
    Serial.println(value);
    Gefahrene_km_Ges = value;
    write_long(addr_Gefahrene_km_Ges, Gefahrene_km_Ges);
  }
  // Pumpdistanz
  else if (input.startsWith(Vari_pumpDistanz))
  {
    input.replace(Vari_pumpDistanz, "");
    int value = input.toInt();
    Serial.print(Vari_pumpDistanz);
    Serial.println(value);
    pumpDistanz = value;
    write_int(addr_pumpDistanz, pumpDistanz);
  }
  // Maximaler Tankinhalt
  else if (input.startsWith(Vari_tankinhalt_max))
  {
    input.replace(Vari_tankinhalt_max, "");
    int value = input.toInt();
    Serial.print(Vari_tankinhalt_max);
    Serial.println(value);
    tankinhalt_max = value;
    write_int(addr_tankinhalt_max, tankinhalt_max);
  }
  // Startdisplay 1
  else if (input.startsWith(Vari_Start_disp_1))
  {
    input.replace(Vari_Start_disp_1, "");
    int value = input.toInt();
    Serial.print(Vari_Start_disp_1);
    Serial.println(value);
    Start_disp_1 = value;
    write_int(addr_Start_disp_1, Start_disp_1);
  }
  // Startdisplay 2
  else if (input.startsWith(Vari_Start_disp_2))
  {
    input.replace(Vari_Start_disp_2, "");
    int value = input.toInt();
    Serial.print(Vari_Start_disp_2);
    Serial.println(value);
    Start_disp_2 = value;
    write_int(addr_Start_disp_2, Start_disp_2);
  }
  // Pumpimpulse Gesamt
  else if (input.startsWith(Vari_pump_anzahl_Ges))
  {
    input.replace(Vari_pump_anzahl_Ges, "");
    int value = input.toInt();
    Serial.print(Vari_pump_anzahl_Ges);
    Serial.println(value);
    pump_anzahl_Ges = value;
    write_int(addr_pump_anzahl_Ges, pump_anzahl_Ges);
  }
  // Geschwindigkeit
  else if (input.startsWith(Vari_geschwindigkeit))
  {
    input.replace(Vari_geschwindigkeit, "");
    int value = input.toInt();
    Serial.print(Vari_geschwindigkeit);
    Serial.println(value);
    geschwindigkeit = value;
    // write_int(addr_pump_anzahl_Ges, pump_anzahl_Ges);
  }
  // RainMulti
  else if (input.startsWith(Vari_rainMulti))
  {
    input.replace(Vari_rainMulti, "");
    int value = input.toInt();
    Serial.print(Vari_rainMulti);
    if (value <= 1)
    {
      value = 1;
    }
    Serial.println(value);
    rainMulti = value;
    write_long(addr_rainMulti, rainMulti);
  }
  // Status Ausrufen
  else if (input.startsWith("Status"))
  {
    Status_Serial();
  }
  // Faktory Set
  else if (input.startsWith("Set_Factory"))
  {
    set_Faktory();
    Status_Serial();
  }
  // EEPROM INIT
  else if (input.startsWith("EEPROM_init"))
  {
    write_int(addr_Factory_init, 1);
    ESP.restart();
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n')
    {
      // Serial.println(inputString);
      evaluate(inputString);
      // clear the string:
      inputString = "";
    }
    else
    {
      // add it to the inputString:
      inputString += inChar;
    }
  }
}

// Serielle Ausgabe aller Benutzerdefinierten Einstellungen
void Status_Serial()
{
  Serial.println("");
  Serial.println("Um die Variable zu ändern z.B. Hell:100 eingeben");
  Serial.println("Um den Oiler in den Factory Zustand zu versetzen Set_Factory eingeben");
  Serial.println("Für Statusabfrage Status eingeben");
  Serial.println("Um den EEPROM neu zu Initialisieren: EEPROM_init");
  Serial.println("Im Anschluss muss 2x neu gestartet werden");
  Serial.println("");
  Serial.print(Vari_Gefahrene_km);              //
  Serial.println(Gefahrene_km);                 // Gefahrene Km
  Serial.print(Vari_Gefahrene_km_Ges);          //
  Serial.println(Gefahrene_km_Ges);             // Gefahrene Km Gesamt
  Serial.print(Vari_pump_anzahl_Ges);           //
  Serial.println(pump_anzahl_Ges);              // Gesamt Pumpimpulse
  Serial.print(Vari_zeit_pumpe_ein);            //
  Serial.println(zeit_pumpe_ein);               // Pumpzeit
  Serial.print(Vari_zeit_pumpe_pause);          //
  Serial.println(zeit_pumpe_pause);             // Pumppause
  Serial.print(Vari_oilsymbol_Zeit);            //
  Serial.println(oilsymbol_Zeit);               // Zeit Oilsymbol ein
  Serial.print(Vari_zeit_bis_notbetrieb);       //
  Serial.println(zeit_bis_notbetrieb);          // Zeit bis Notbetrieb
  Serial.print(Vari_Not_geschwindigkeit);       //
  Serial.println(geschwindigkeit_Notbetrieb);   // Geschwindigkeit Notbetrieb
  Serial.print(Vari_pumpDistanz);               //
  Serial.println(pumpDistanz);                  // Pumpdistanz
  Serial.print(Vari_pump_nach_Regen);           //
  Serial.println(pump_nach_Regen);              // Pumpimpulse nach Regenmodus
  Serial.print(Vari_minGeschwindigkeit);        //
  Serial.println(minGeschwindigkeit);           // Mindestgeschwindigkeit
  Serial.print(Vari_Start_disp_1);              //
  Serial.println(Start_disp_1);                 // Anzeigezeit Startdisplay 1
  Serial.print(Vari_Start_disp_2);              //
  Serial.println(Start_disp_2);                 // Anzeigezeit Startdisplay 2
  Serial.print(Vari_rainMulti);                 //
  Serial.println(rainMulti);                    //
  Serial.print(Vari_geschwindigkeit);           // Anzeige Geschwindigkeit
  Serial.println(geschwindigkeit);              //
  Serial.println();                             //
  Serial.println();                             //
  Serial.println("Nur Anzeige !!!");            //
  Serial.print(Vari_tankinhalt_max);            //
  Serial.println(tankinhalt_max);               // Maximaler Tankinhalt
  Serial.print("Tankinhalt Aktuell: ");         //
  Serial.println(tankinhalt_Aktuell);           //
  Serial.print("Speicherplatz km: ");           //
  Serial.println(addr_Gefahrene_km);            //
  Serial.print("Speicherplatz km Ges.: ");      //
  Serial.println(addr_Gefahrene_km_Ges);        //
  Serial.print("Speicherplatz Tankinhalt: ");   //
  Serial.println(addr_tankinhalt_Aktuell);      //
  Serial.print("Speicherplatz Impulse Ges.: "); //
  Serial.println(addr_pump_anzahl_Ges);         //
}

// WIFI AP Start
void WiFi_Start_AP()
{
  WiFi.mode(WIFI_AP); // Accesspoint
  WiFi.softAP(ssid_ap, password_ap);
  server.begin();
  my_WiFi_Mode = WIFI_AP;
}

// Auslesen des EEPROM
void reed_Eeprom()
{
  Gefahrene_km = read_long(addr_Gefahrene_km) / 100.0;
  Gefahrene_km_Ges = read_long(addr_Gefahrene_km_Ges);
  pump_anzahl_Ges = read_int(addr_pump_anzahl_Ges);
  tankinhalt_Aktuell = read_int(addr_tankinhalt_Aktuell);
  read_char_array(addr_ssid_ap, 20, EEPROM_I2C_ADDRESS, ssid_ap);
  read_char_array(addr_password_ap, 20, EEPROM_I2C_ADDRESS, password_ap);
  pumpDistanz = read_int(addr_pumpDistanz);
  anzahl_Pump = read_int(addr_anzahl_Pump);
  minGeschwindigkeit = read_int(addr_minGeschwindigkeit);
  tankinhalt_max = read_int(addr_tankinhalt_max);
  zeit_bis_notbetrieb = read_int(addr_zeit_bis_notbetrieb);
  geschwindigkeit_Notbetrieb = read_int(addr_geschwindigkeit_Notbetrieb);
  zeit_pumpe_ein = read_int(addr_zeit_pumpe_ein);
  zeit_pumpe_pause = read_int(addr_zeit_pumpe_pause);
  TimeAPout = read_int(addr_TimeAPout);
  gefahrene_m = read_int(addr_gefahrene_m);
  oilsymbol_Zeit = read_int(addr_oilsymbol_Zeit);
  pump_nach_Regen = read_int(addr_pump_nach_Regen);
  Start_disp_1 = read_int(addr_Start_disp_1);
  Start_disp_2 = read_int(addr_Start_disp_2);
  Factory_init = read_int(addr_Factory_init);
  regenmodus = read_int(addr_regenmodus);
  sw_Regensensor_ein = read_int(addr_sw_Regensensor_ein);
  sw_Regensensor_aus = read_int(addr_sw_Regensensor_aus);
  rainMulti = read_long(addr_rainMulti);
  pumps_ml = read_int(addr_pumps_ml);
  tankinhalt_ml = read_int(addr_tankinhalt_ml);
  tankinhalt_ml_akt = read_int(addr_tankinhalt_ml_akt);
  dir_Display = read_int(addr_dir_Display);
  init_pump_anzahl = read_int(addr_init_pump_anzahl);
}

// Hier wird ermittelt ob am AP Daten anliegen
void WiFI_Traffic()
{
  char my_char;
  int htmlPtr = 0;
  int myIndex;
  unsigned long my_timeout;

  // Check if a client has connected
  client = server.available();
  if (!client)
  {
    return;
  }

  my_timeout = millis() + 250L;
  while (!client.available() && (millis() < my_timeout))
    delay(10);
  delay(10);
  if (millis() > my_timeout)
  {
    return;
  }

  htmlPtr = 0;
  my_char = 0;
  while (client.available() && my_char != '\r')
  {
    my_char = client.read();
    HTML_String[htmlPtr++] = my_char;
  }
  client.flush();
  HTML_String[htmlPtr] = 0;

  if (Find_Start("/?", HTML_String) < 0 && Find_Start("GET / HTTP", HTML_String) < 0)
  {
    send_not_found();
    return;
  }

  //---------------------------------------------------------------------
  // Benutzereingaben einlesen und verarbeiten
  //---------------------------------------------------------------------
  action = Pick_Parameter_Zahl("ACTION=", HTML_String);

  // SSID und password_ap
  if (action == ACTION_SET_WIFI)
  {
    myIndex = Find_End("Name_AP=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(ssid_ap, &HTML_String[myIndex], 20);
      write_char_array(addr_ssid_ap, ssid_ap, 20, EEPROM_I2C_ADDRESS);
      read_char_array(addr_ssid_ap, 20, EEPROM_I2C_ADDRESS, ssid_ap);
    }

    myIndex = Find_End("Password_AP=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(password_ap, &HTML_String[myIndex], 20);
      write_char_array(addr_password_ap, password_ap, 20, EEPROM_I2C_ADDRESS);
      read_char_array(addr_password_ap, 20, EEPROM_I2C_ADDRESS, password_ap);
    }
  }

  // OILEN
  if (action == ACTION_SET_OILEN)
  {
    myIndex = Find_End("P_Distanz=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      pumpDistanz = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_pumpDistanz, pumpDistanz);
    }

    myIndex = Find_End("P_Impulse=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      anzahl_Pump = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_anzahl_Pump, anzahl_Pump);
    }

    myIndex = Find_End("MIN_SPEED=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      minGeschwindigkeit = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_minGeschwindigkeit, minGeschwindigkeit);
    }

    myIndex = Find_End("init_pump=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      init_pump_anzahl = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_init_pump_anzahl, init_pump_anzahl);
    }
  }

  // Spülen ein
  if (action == ACTION_SET_SPUELEN_EIN)
  {
    Spuelen = true;
  }

  // Spülen Aus
  if (action == ACTION_SET_SPUELEN_AUS)
  {
    Spuelen = false;
  }

  // TANK
  if (action == ACTION_SET_TANK)
  {
    myIndex = Find_End("TANKINHALT=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      tankinhalt_ml = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_tankinhalt_ml, tankinhalt_ml);
      tankinhalt_max = tankinhalt_ml * pumps_ml;
      write_int(addr_tankinhalt_max, tankinhalt_max);
    }
  }

  if (action == ACTION_SET_TANK_RESET || action == ACTION_SET_TANK)
  {
    {
      Gefahrene_km_display = 0;
      Gefahrene_km = 0;
      Gefahrene_km_Temp = 0;
      tankinhalt_Aktuell = tankinhalt_ml * pumps_ml;
      write_int(addr_tankinhalt_Aktuell, tankinhalt_Aktuell);
      gefahrene_m = 0;
      set_Eeprom();
    }
  }

  // Notbetrieb
  if (action == ACTION_SET_NOTBETRIEB)
  {
    myIndex = Find_End("Not_Zeit=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      zeit_bis_notbetrieb = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_zeit_bis_notbetrieb, zeit_bis_notbetrieb);
    }

    myIndex = Find_End("NOT_SPEED=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      geschwindigkeit_Notbetrieb = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_geschwindigkeit_Notbetrieb, geschwindigkeit_Notbetrieb);
    }
  }

  // Pumpe
  if (action == ACTION_SET_PUMPE)
  {

    myIndex = Find_End("PUMPE_D=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      zeit_pumpe_ein = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_zeit_pumpe_ein, zeit_pumpe_ein);
    }

    myIndex = Find_End("PUMPE_P=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      zeit_pumpe_pause = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_zeit_pumpe_pause, zeit_pumpe_pause);
    }

    myIndex = Find_End("NOT_SPEED=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      geschwindigkeit_Notbetrieb = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_geschwindigkeit_Notbetrieb, geschwindigkeit_Notbetrieb);
    }
    myIndex = Find_End("Pumps_ml=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      pumps_ml = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_pumps_ml, pumps_ml);
    }
  }

  // AP Aus
  if (action == ACTION_SET_APaus)
  {

    myIndex = Find_End("AP_AUS=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      TimeAPout = Pick_N_Zahl(tmp_string, ':', 1);
      if (TimeAPout <= 1)
      {
        TimeAPout = 1;
      }
      write_int(addr_TimeAPout, TimeAPout);
    }
  }

  // Anzeigezeit Ölsymbol
  if (action == ACTION_SET_OILSYMBOL_Zeit)
  {

    myIndex = Find_End("Oilsymbol_Zeit=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      oilsymbol_Zeit = Pick_N_Zahl(tmp_string, ':', 1);
      if (TimeAPout <= 1)
      {
        TimeAPout = 1;
      }
      write_int(addr_oilsymbol_Zeit, oilsymbol_Zeit);
    }
  }

  // Extraölen ein
  if (action == ACTION_SET_EXTRAOILEN_EIN)
  {
    extraOilen = true;
  }

  // Extraölen aus
  if (action == ACTION_SET_EXTRAOILEN_AUS)
  {
    extraOilen = false;
  }

  // Anzeigedauer Startdisplay
  if (action == ACTION_SET_ANZEIGEZEIT_DISP)
  {
    myIndex = Find_End("DISPLAY_1=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      Start_disp_1 = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_Start_disp_1, Start_disp_1);
    }

    myIndex = Find_End("DISPLAY_2=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      Start_disp_2 = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_Start_disp_2, Start_disp_2);
    }
  }

  // Regenmodus
  if (action == ACTION_SET_REGENMODUS)
  {
    myIndex = Find_End("SW_EIN=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      sw_Regensensor_ein = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_sw_Regensensor_ein, sw_Regensensor_ein);
    }

    myIndex = Find_End("SW_AUS=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      sw_Regensensor_aus = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_sw_Regensensor_aus, sw_Regensensor_aus);
    }
    myIndex = Find_End("DIR_MW=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      dir_Display = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_dir_Display, dir_Display);
    }
    myIndex = Find_End("RainMulti=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      rainMulti = Pick_N_Zahl(tmp_string, ':', 1);
      if (rainMulti <= 1)
      {
        rainMulti = 1;
      }
      write_long(addr_rainMulti, rainMulti);
    }
    myIndex = Find_End("Impulse_nach_Regen=", HTML_String);
    if (myIndex >= 0)
    {
      Pick_Text(tmp_string, &HTML_String[myIndex], 20);
      pump_nach_Regen = Pick_N_Zahl(tmp_string, ':', 1);
      write_int(addr_pump_nach_Regen, pump_nach_Regen);
    }
  }

  // PAGE
  if (action == ACTION_SET_PAGE)
  {
    HTML_Page = !HTML_Page;
  }

  // Antwortseite aufbauen
  if (HTML_Page == true)
  {
    make_HTML01();
  }
  else
  {
    make_HTML02();
  }

  // Header aufbauen
  strcpy(HTTP_Header, "HTTP/1.1 200 OK\r\n");
  strcat(HTTP_Header, "Content-Length: ");
  strcati(HTTP_Header, strlen(HTML_String));
  strcat(HTTP_Header, "\r\n");
  strcat(HTTP_Header, "Content-Type: text/html\r\n");
  strcat(HTTP_Header, "Connection: close\r\n");
  strcat(HTTP_Header, "\r\n");
  client.print(HTTP_Header);
  delay(20);
  send_HTML();
}

// HTML Seite 01 aufbauen
void make_HTML01()
{
  strcpy(HTML_String, "<!DOCTYPE html>");
  strcat(HTML_String, "<html>");
  strcat(HTML_String, "<head>");
  strcat(HTML_String, "<title>");
  strcat(HTML_String, Rev_OILER);
  strcat(HTML_String, "</title>");
  strcat(HTML_String, "</head>");
  strcat(HTML_String, "<body bgcolor=\"#D8D8D8\">");
  strcat(HTML_String, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
  strcat(HTML_String, "<h1>");
  strcat(HTML_String, Rev_OILER);
  strcat(HTML_String, "</h1>");

  // Firmwareversion
  strcat(HTML_String, "<h2>Firmware Version: ");
  strcat(HTML_String, firmware_Vers);
  strcat(HTML_String, "</h2>");

  // WIFI
  strcat(HTML_String, "<h2>WIFI</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>SSID</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Name_AP\" maxlength=\"20\" Value =\"");
  strcat(HTML_String, ssid_ap);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_WIFI);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Passwort</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Password_AP\" maxlength=\"20\" Value =\"");
  strcat(HTML_String, password_ap);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Mindestl&auml;nge beim Passwort ist 8 Stellen. Leer kein Passwort!!</p>");
  strcat(HTML_String, "<p>Die &Auml;nderungen am WIFI sind erst nach einem Neustart wirksam!</p>");

  // WIFI Aus
  strcat(HTML_String, "<h2>WIFI-Aus</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Minuten</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"AP_AUS\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, TimeAPout);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_APaus);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Zeit in Minuten bis zum abschalten des WIFI.</p>");
  strcat(HTML_String, "<p>Kann nicht kleiner als 1 Minute sein</p>");

  // Oilen
  strcat(HTML_String, "<h2>&Ouml;len</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Distanz</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"P_Distanz\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, pumpDistanz);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_OILEN);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Pumpimpulse</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"P_Impulse\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, anzahl_Pump);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Min. Geschwindigkeit</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"MIN_SPEED\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, minGeschwindigkeit);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Init Impulse</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"init_pump\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, init_pump_anzahl);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");

  strcat(HTML_String, "<p>Distanz bis zur n&auml;chsten &Ouml;lung in Meter, Impulse pro &Ouml;lung, Mindestgeschwindigkeit zum &Ouml;len. Pumpimpulse f&uuml;r die Initial&ouml;lung.</p>");

  // Extraölen
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 100, 160, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b> Extra &Ouml;len</b></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_EXTRAOILEN_EIN);
  strcat(HTML_String, "\">Ein</button></td>");

  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_EXTRAOILEN_AUS);
  strcat(HTML_String, "\">Aus</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Extra &Ouml;len: &Ouml;lt 1,5 mal so viel bis zum Abschalten der Z&uuml;ndung.</p>");

  // Spülen
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 100, 160, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Sp&uuml;len</b></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_SPUELEN_EIN);
  strcat(HTML_String, "\">Ein</button></td>");

  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_SPUELEN_AUS);
  strcat(HTML_String, "\">Aus</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Sp&uuml;len und Entl&uuml;ften der Leitung.</p>");

  // Tank
  strcat(HTML_String, "<h2>&Ouml;ltank</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Tankinhalt</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"TANKINHALT\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, tankinhalt_ml);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_TANK);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");
  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Tankinhalt Akt.</b></td>");
  strcat(HTML_String, "<td>");
  tankinhalt_ml_akt = tankinhalt_Aktuell / pumps_ml;
  strcati(HTML_String, tankinhalt_ml_akt);

  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_TANK_RESET);
  strcat(HTML_String, "\">Tankreset</button></td>");
  strcat(HTML_String, "</tr>");
  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");

  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Gefahrene Km</b></td>");
  strcat(HTML_String, "<td>");
  strcati(HTML_String, Gefahrene_km_display);
  strcat(HTML_String, "</tr>");
  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Tankinhalt in ml und Tankreset.</p>");
  strcat(HTML_String, "<p>Tankinhalt nur bei vollem Tank &aumlndern. (Tankreset).</p>");

  // Notbetrieb
  strcat(HTML_String, "<h2>Notbetrieb</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Zeit</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Not_Zeit\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, zeit_bis_notbetrieb);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_NOTBETRIEB);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Geschwindigkeit</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"NOT_SPEED\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, geschwindigkeit_Notbetrieb);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Geschwindigkeit bei Notbetrieb und Zeit bis zum Notbetrieb in Sekunden.</p>");

  // PAGE
  strcat(HTML_String, "<form>");

  strcat(HTML_String, "<button style= \"width:200px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_PAGE);
  strcat(HTML_String, "\">Weitere Einstellungen</button>");

  strcat(HTML_String, "</form>");

  strcat(HTML_String, "<BR>");

  TimeAPoutmillis = (millis() + (TimeAPout * 60000));
}

void make_HTML02()
{
  strcpy(HTML_String, "<!DOCTYPE html>");
  strcat(HTML_String, "<html>");
  strcat(HTML_String, "<head>");
  strcat(HTML_String, "<title>");
  strcat(HTML_String, Rev_OILER);
  strcat(HTML_String, "</title>");
  strcat(HTML_String, "</head>");
  strcat(HTML_String, "<body bgcolor=\"#D8D8D8\">");
  strcat(HTML_String, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
  strcat(HTML_String, "<h1>");
  strcat(HTML_String, Rev_OILER);
  strcat(HTML_String, "</h1>");

  // Pumpe
  strcat(HTML_String, "<h2>Pumpe</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Dauer in ms</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"PUMPE_D\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, zeit_pumpe_ein);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_PUMPE);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Pause in ms</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"PUMPE_P\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, zeit_pumpe_pause);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Pumpsimpulse pro ml</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Pumps_ml\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, pumps_ml);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Dauer der Pumpimpulse und der Pausenzeiten.</p>");
  strcat(HTML_String, "<p>Anzahl der Pumpsimpulse pro ml.</p>");

  // Ölsymbol Dauer
  strcat(HTML_String, "<h2>Dauer &Ouml;lsymbol</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);
  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Dauer &Ouml;lsymbol</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Oilsymbol_Zeit\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, oilsymbol_Zeit);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_OILSYMBOL_Zeit);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Anzeigezeit des &Oumllsymbol in Sec.</p>");

  // Anzeigezeit Startdisplays
  strcat(HTML_String, "<h2>Anzeigezeit Startbildschirm</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Startbildschirm 1</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"DISPLAY_1\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, Start_disp_1);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_ANZEIGEZEIT_DISP);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Startbildschirm 2</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"DISPLAY_2\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, Start_disp_2);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");
  strcat(HTML_String, "<p>Anzeigezeit der Startbildschirme 1 und 2 in Sec.</p>");

  // Regenmodus
  strcat(HTML_String, "<h2>Regenmodus</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Ein</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"SW_EIN\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, sw_Regensensor_ein);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_REGENMODUS);
  strcat(HTML_String, "\">&Uuml;bernehmen</button></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Aus</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"SW_AUS\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, sw_Regensensor_aus);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Richtung/Sensor</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"DIR_MW\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, dir_Display);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Sensormittelwert</b></td>");
  strcat(HTML_String, "<td>");
  strcati(HTML_String, regensensorMittelwert);
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Sensorwert</b></td>");
  strcat(HTML_String, "<td>");
  strcati(HTML_String, analogRead(regenSensor));
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Multiplikator</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"RainMulti\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, rainMulti);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Pumpimpulse nach Regenmodus</b></td>");
  strcat(HTML_String, "<td>");
  strcat(HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Impulse_nach_Regen\" maxlength=\"20\" Value =\"");
  strcati(HTML_String, pump_nach_Regen);
  strcat(HTML_String, "\"></td>");
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");

  strcat(HTML_String, "<p>Ein/Aus Schwelle f&uuml;r den Regenmodus</p>");
  strcat(HTML_String, "<p>Richtung/Sensor: 1 f&uuml;r Richtung oder 0 f&uuml;r Sensormittelwert im Display</p>");
  strcat(HTML_String, "<p>Multiplikator f&uumlr den Regenmodus. Kann nicht kleiner als 1 sein.</p>");
  strcat(HTML_String, "<p>Pumpimpulse nach Regenmodus.</p>");

  // Gesamt Km und Pumpimpulse
  strcat(HTML_String, "<h2>Gesamt Km und Pumpimpulse</h2>");
  strcat(HTML_String, "<form>");
  strcat(HTML_String, "<table>");
  set_colgroup(170, 270, 150, 0, 0);

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Gesamt Km</b></td>");
  strcat(HTML_String, "<td>");
  strcati(HTML_String, Gefahrene_km_Ges);
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "<tr>");
  strcat(HTML_String, "<td><b>Gesamt Pumpimpulse</b></td>");
  strcat(HTML_String, "<td>");
  strcati(HTML_String, pump_anzahl_Ges);
  strcat(HTML_String, "</tr>");

  strcat(HTML_String, "</table>");
  strcat(HTML_String, "</form>");

  // PAGE
  strcat(HTML_String, "<BR>");

  strcat(HTML_String, "<form>");

  strcat(HTML_String, "<button style= \"width:120px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_PAGE);
  strcat(HTML_String, "\">Startseite</button>");

  strcat(HTML_String, "</form>");
}

//--------------------------------------------------------------------------
void send_not_found()
{
  client.print("HTTP/1.1 404 Not Found\r\n\r\n");
  delay(20);
  client.stop();
}

//--------------------------------------------------------------------------
void send_HTML()
{
  char my_char;
  int my_len = strlen(HTML_String);
  int my_ptr = 0;
  int my_send = 0;

  //--------------------------------------------------------------------------
  // in Portionen senden
  while ((my_len - my_send) > 0)
  {
    my_send = my_ptr + MAX_PACKAGE_SIZE;
    if (my_send > my_len)
    {
      client.print(&HTML_String[my_ptr]);
      delay(20);
      my_send = my_len;
    }
    else
    {
      my_char = HTML_String[my_send];
      // Auf Anfang eines Tags positionieren
      while (my_char != '<')
        my_char = HTML_String[--my_send];
      HTML_String[my_send] = 0;
      client.print(&HTML_String[my_ptr]);
      delay(20);
      HTML_String[my_send] = my_char;
      my_ptr = my_send;
    }
  }
  client.stop();
}

//----------------------------------------------------------------------------------------------
void set_colgroup(int w1, int w2, int w3, int w4, int w5)
{
  strcat(HTML_String, "<colgroup>");
  set_colgroup1(w1);
  set_colgroup1(w2);
  set_colgroup1(w3);
  set_colgroup1(w4);
  set_colgroup1(w5);
  strcat(HTML_String, "</colgroup>");
}
//------------------------------------------------------------------------------------------
void set_colgroup1(int ww)
{
  if (ww == 0)
    return;
  strcat(HTML_String, "<col width=\"");
  strcati(HTML_String, ww);
  strcat(HTML_String, "\">");
}

//---------------------------------------------------------------------
void strcati(char *tx, int i)
{
  char tmp[8];

  itoa(i, tmp, 10);
  strcat(tx, tmp);
}

//---------------------------------------------------------------------
int Pick_Parameter_Zahl(const char *par, char *str)
{
  int myIdx = Find_End(par, str);

  if (myIdx >= 0)
    return Pick_Dec(str, myIdx);
  else
    return -1;
}
//---------------------------------------------------------------------
int Find_End(const char *such, const char *str)
{
  int tmp = Find_Start(such, str);
  if (tmp >= 0)
    tmp += strlen(such);
  return tmp;
}

//---------------------------------------------------------------------
int Find_Start(const char *such, const char *str)
{
  int tmp = -1;
  int ww = strlen(str) - strlen(such);
  int ll = strlen(such);

  for (int i = 0; i <= ww && tmp == -1; i++)
  {
    if (strncmp(such, &str[i], ll) == 0)
      tmp = i;
  }
  return tmp;
}
//---------------------------------------------------------------------
int Pick_Dec(const char *tx, int idx)
{
  int tmp = 0;

  for (int p = idx; p < idx + 5 && (tx[p] >= '0' && tx[p] <= '9'); p++)
  {
    tmp = 10 * tmp + tx[p] - '0';
  }
  return tmp;
}
//----------------------------------------------------------------------------
int Pick_N_Zahl(const char *tx, char separator, byte n)
{

  int ll = strlen(tx);
  // int tmp = -1;
  byte anz = 1;
  byte i = 0;
  while (i < ll && anz < n)
  {
    if (tx[i] == separator)
      anz++;
    i++;
  }
  if (i < ll)
    return Pick_Dec(tx, i);
  else
    return -1;
}

//---------------------------------------------------------------------
int Pick_Hex(const char *tx, int idx)
{
  int tmp = 0;

  for (int p = idx; p < idx + 5 && ((tx[p] >= '0' && tx[p] <= '9') || (tx[p] >= 'A' && tx[p] <= 'F')); p++)
  {
    if (tx[p] <= '9')
      tmp = 16 * tmp + tx[p] - '0';
    else
      tmp = 16 * tmp + tx[p] - 55;
  }

  return tmp;
}

//---------------------------------------------------------------------
void Pick_Text(char *tx_ziel, char *tx_quelle, int max_ziel)
{

  int p_ziel = 0;
  int p_quelle = 0;
  int len_quelle = strlen(tx_quelle);

  while (p_ziel < max_ziel && p_quelle < len_quelle && tx_quelle[p_quelle] && tx_quelle[p_quelle] != ' ' && tx_quelle[p_quelle] != '&')
  {
    if (tx_quelle[p_quelle] == '%')
    {
      tx_ziel[p_ziel] = (HexChar_to_NumChar(tx_quelle[p_quelle + 1]) << 4) + HexChar_to_NumChar(tx_quelle[p_quelle + 2]);
      p_quelle += 2;
    }
    else if (tx_quelle[p_quelle] == '+')
    {
      tx_ziel[p_ziel] = ' ';
    }
    else
    {
      tx_ziel[p_ziel] = tx_quelle[p_quelle];
    }
    p_ziel++;
    p_quelle++;
  }

  tx_ziel[p_ziel] = 0;
}
//---------------------------------------------------------------------
char HexChar_to_NumChar(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 55;
  return 0;
}

void update_Display()
{
  // Convert float to a string:
  // (<variable>,<amount of digits we are going to use>,<amount of decimal digits>,<string name>)
  dtostrf(richtung, 3, 0, stringDir);                              // Wandelt die richtung in ein char um
  dtostrf(sateliten_anzahl, 3, 0, stringsateliten_anzahl);         // Wandelt die richtung in ein char um
  dtostrf(Gefahrene_km_display, 5, 0, stringGefahrene_km_display); // Wandelt die Gefahrene_km_display in ein char um
  dtostrf(geschwindigkeit, 2, 0, stringSpeed);                     // Wandelt die Variable geschwindigkeit in ein char um
  dtostrf(regensensorMittelwert, 3, 0, stringMittelwert);          // Wandelt die Variable regensensorMittelwert in ein char um
  // Verhintert das ständige springen der Anzeige beim Stehen
  if (geschwindigkeit <= 3)
  {
    geschwindigkeit = 0;
    richtung = 0;
  }

  display.clearDisplay(); // Clear the display so we can refresh

  // Ändert die schriftart auf Bold 12pt
  display.setFont(&FreeMonoBold12pt7b); // Set a custom font

  // Zeichnet einen Rahmen für die gefahrenen km
  display.drawRoundRect(1, 27, 75, 25, 4, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)

  // Zeichnet den Rahmen für die Geschwindigkeit
  display.drawRoundRect(79, 0, 48, 25, 4, WHITE); // Draw rectangle (x,y,width,height,color)

  // Zeichnet den Rahmen für die Richtung
  display.drawRoundRect(79, 27, 48, 25, 4, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)

  // Zeigt die Geschwindigkeit an
  display.setCursor(83, 20); // (x,y)
  if (!sat_empfang && speed_show_toogle)
  {
    speed_show_toogle = false;
  }
  else
  {
    display.println(stringSpeed);
    speed_show_toogle = true; // Text or value to print
  }

  // Zeigt die entspechende Himmelsrichtung zur Gradzahl an
  display.setCursor(95, 46); // (x,y)
  if (dir_Display == true)
  {
    if (sat_empfang == true)
    {
      if (richtung > 337 || richtung <= 22)
      {
        display.setCursor(98, 46); // (x,y)
        display.print("N");
      }
      if (richtung > 22 && richtung < 67)
      {
        display.setCursor(90, 46); // (x,y)
        display.print("NO");
      }
      if (richtung > 67 && richtung < 112)
      {
        display.setCursor(98, 46); // (x,y)
        display.print("O");
      }
      if (richtung > 112 && richtung < 157)
      {
        display.setCursor(90, 46); // (x,y)
        display.print("SO");
      }
      if (richtung > 157 && richtung < 202)
      {
        display.setCursor(98, 46); // (x,y)
        display.print("S");
      }
      if (richtung > 202 && richtung < 247)
      {
        display.setCursor(90, 46); // (x,y)
        display.print("SW");
      }
      if (richtung > 247 && richtung < 292)
      {
        display.setCursor(98, 46); // (x,y)
        display.print("W");
      }
      if (richtung > 292 && richtung < 337)
      {
        display.setCursor(90, 46); // (x,y)
        display.print("NW");
      }
    }
  }
  else
  {
    {
      display.setCursor(82, 46); // (x,y)
      display.print(stringMittelwert);
    }
  }
  // Zeigt die gefahrenen Km mit der Tankfüllung an
  display.setCursor(5, 46);
  display.println(stringGefahrene_km_display);

  // Rahmen und Balken für die Anzeige des Tankinhalt
  display.drawRect(1, 55, 127, 9, 1); // Border of the bar chart

  // Hier wied der Balken für den Tankinhalt Berechnet und Angezeig
  tankinhalt_Anzeige = map(tankinhalt_Aktuell, 0, tankinhalt_max, 0, 127);

  display.fillRect(0, 55, tankinhalt_Anzeige, 9, WHITE); // Draws the bar depending on the sensor value
  display.drawRect(0, 55, 128, 9, BLACK);                // Draw rounded rectangle (x,y,width,height,radius,color)

  // Hier wird das Symbol für den Satempfang angezeigt
  if (sat_empfang == true)
  {
    if (sateliten_anzahl < 10)
    {
      display.drawBitmap(40, 6, satelite, 16, 16, 1);
    }
  }
  display.setCursor(32, 20); // (x,y)
  display.println(stringsateliten_anzahl);

  // Hier wird das Sympol für das Ölen angezeigt
  if (oilsymbol == true)
  {
    display.drawBitmap(1, 6, oilcan, 16, 16, 1);

    if (oilsymbol_Aus >= 1)
    {
      oilsymbol_Aus--;
      if (oilsymbol_Aus == 0)
      {
        oilsymbol = false;
        oled_akt = false;
      }
    }
  }

  // Hier wird der Regenmodus angezeigt
  if (regenmodus == true)
  {
    display.drawBitmap(20, 6, raining, 16, 16, 1);
  }

  display.display(); // Print everything we set previously
}

void update_GPS_Data()
{
  if (gps.speed.isValid())
  {
    float geschwindigkeit_temp = (gps.speed.kmph());
    if (geschwindigkeit != geschwindigkeit_temp)
    {
      geschwindigkeit = geschwindigkeit_temp;
      oled_akt = false;
    }
  }

  notbetrieb_variable++;
  if (notbetrieb_variable > zeit_bis_notbetrieb)
  {
    geschwindigkeit = geschwindigkeit_Notbetrieb;
    if (Not_Oil_Init == false)
    {
      Not_Oil_Init = true;
      gefahrene_m = 0;
      oilen = true;
      pump_anzahl_soll = 1;
      oilsymbol_Aus = oilsymbol_Zeit;
      oled_akt = false;
    }
  }

  if (gps.course.isValid())
  {
    int richtung_temp = (gps.course.deg());
    if (richtung != richtung_temp)
    {
      richtung = richtung_temp;
      oled_akt = false;
    }
  }

  if (gps.satellites.isValid())
  {
    byte sateliten_anzahl_temp = gps.satellites.value();
    if (sateliten_anzahl != sateliten_anzahl_temp)
    {
      sateliten_anzahl = sateliten_anzahl_temp;
      oled_akt = false;
    }
  }

  if (sateliten_anzahl >= 3 && serial_data > 0) //&& (GPS_serial.available() > 0)
  {
    sat_empfang = true;
    notbetrieb_variable = 0;
    Not_Oil_Init = false;
  }
  else
  {
    sat_empfang = false;
  }

  serial_data = 0;
}

void update_km()
{
  float Gefahrene_km_sec = geschwindigkeit / 3600;
  Gefahrene_km = (Gefahrene_km + Gefahrene_km_sec);

  Gefahrene_km_Temp = Gefahrene_km;
  if (Gefahrene_km_Temp > Gefahrene_km_display)
  {
    Gefahrene_km_display = Gefahrene_km_Temp;

    Gefahrene_km_Ges++;

    write_long(addr_Gefahrene_km, Gefahrene_km * 100);
    write_long(addr_Gefahrene_km_Ges, Gefahrene_km_Ges);
    write_int(addr_Gefahrene_km_Display, Gefahrene_km_display);
    oled_akt = false;
  }
}

void update_Oelen()
{
  float Gefahrene_m_sec = geschwindigkeit / 3.6;

  if (regenmodus)
  {
    Gefahrene_m_sec = Gefahrene_m_sec * rainMulti; // Erhöht bei Regen die berechnete zurückgelegte Strecke
  }

  if (extraOilen)
  {
    Gefahrene_m_sec = Gefahrene_m_sec * 1.5; // Verringert bei ExtraOilen die abstände zwischen den Oelungen
  }

  gefahrene_m = gefahrene_m + Gefahrene_m_sec;

  if (gefahrene_m >= pumpDistanz) // gefahrene_m wird mit pumpDistanz verglichen
  {
    gefahrene_m = gefahrene_m - pumpDistanz;
    oilen = true;
    pump_anzahl_soll = pump_anzahl_soll + anzahl_Pump;
    // oilsymbol_Aus = oilsymbol_Zeit;
    // oled_akt = false;
    write_int(addr_gefahrene_m, gefahrene_m);
  }
}

void Oelen()
{
  if ((pump_anzahl_soll > 0) && millis() >= variable_pumpe_ein)
  {
    digitalWrite(oilPin, LOW);
    digitalWrite(LedPin, LOW);
    oilsymbol = true;
    pump_anzahl_soll--;   // Hier werden die Pumpimpuse gezählt
    tankinhalt_Aktuell--; // Hier werden die Pumpimpulse vom Tankinhalt abgezogen
    if (tankinhalt_Aktuell <= 0)
    {
      tankinhalt_Aktuell = 0;
    }
    write_int(addr_tankinhalt_Aktuell, tankinhalt_Aktuell);

    variable_pumpe_ein = millis() + zeit_pumpe_ein + zeit_pumpe_pause;
    variable_pumpe_pause = millis() + zeit_pumpe_ein;
    pump_anzahl_Ges++;

    write_long(addr_pump_anzahl_Ges, pump_anzahl_Ges);
    oilsymbol_Aus = oilsymbol_Zeit;
    oled_akt = false;
  }

  if (millis() >= variable_pumpe_pause)
  {
    digitalWrite(oilPin, HIGH);
    digitalWrite(LedPin, HIGH);

    if (pump_anzahl_soll == 0)
    {
      oilen = false;
    }
  }
}

void update_Rain()
{
  // digitalWrite(U_Sensor, HIGH);
  // delay(5);

  regensensorMittelwert = 0.85 * regensensorMittelwert + 0.15 * analogRead(regenSensor);

  if (regensensorMittelwert <= sw_Regensensor_ein)
  {
    regenmodus = true;
    write_int(addr_regenmodus, regenmodus);
  }
  if (regensensorMittelwert >= sw_Regensensor_aus && regenmodus)
  {
    regenmodus = false;
    write_int(addr_regenmodus, regenmodus);
    pump_anzahl_soll = pump_nach_Regen;
    oilen = true;
  }

  // digitalWrite(U_Sensor, LOW);
}

// Anzeigen des Startbildschirm auf dem OLD-Display
void start_Disp_1()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize display with the I2C address of 0x3C
  display.clearDisplay();                    // Clear the buffer
  display.setTextColor(WHITE);               // Set color of the text
  display.setRotation(0);                    // Set orientation. Goes from 0, 1, 2 or 3
  display.setTextWrap(false);                // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  display.dim(0);                            // Set brightness (0 is maximun and 1 is a little dim)
  display.invertDisplay(invert_Display);     //
  display.setFont(&FreeMono9pt7b);           // Set a custom font
  display.setCursor(1, 20);                  //
  display.println("Access point");           //
  display.println(ssid_ap);                  //
  display.println("192.168.4.1");            //
  display.display();                         // Print everything we set previously
}

void start_Disp_2()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize display with the I2C address of 0x3C
  display.clearDisplay();                    // Clear the buffer
  display.setTextColor(WHITE);               // Set color of the text
  display.setRotation(0);                    // Set orientation. Goes from 0, 1, 2 or 3
  display.setTextWrap(false);                // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  display.dim(0);                            // Set brightness (0 is maximun and 1 is a little dim)
  display.invertDisplay(invert_Display);     //
  display.setFont(&FreeMono9pt7b);           // Set a custom font
  display.setCursor(1, 20);                  //
  display.println(Rev_OILER);                //
  display.println("Firmware");               //
  display.print(firmware_Vers);              //
  display.display();                         // Print everything we set previously
}

void read_char_array(int address, int length, int i2c_address, char *rcvData)
{

  for (int i = 0; i < length; i++)
  {
    // byte charByte;

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(i2c_address);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // End the transmission
    Wire.endTransmission();

    // Request one byte of data at current memory address
    Wire.requestFrom(i2c_address, 1);

    // Read the data and assign to variable
    rcvData[i] = Wire.read();

    address++;
  }
}

void write_char_array(int address, char *value, int length, int i2c_address)
{
  if (stop_write_eeprom == false)
  {
    for (int i = 0; i < length; i++)
    {
      byte charByte = (value[i]);

      // Begin transmission to I2C EEPROM
      Wire.beginTransmission(i2c_address);

      // Send memory address as two 8-bit bytes
      Wire.write((int)(address >> 8));   // MSB
      Wire.write((int)(address & 0xFF)); // LSB

      // Send data to be stored
      Wire.write(charByte);

      // End the transmission
      Wire.endTransmission();

      // Add 5ms delay for EEPROM
      delay(5);
      address++;
    }
  }
}

int read_int(int address)
{
  // Define byte for received data
  unsigned int rcvData = 0xFF;
  byte byte_1 = 0;
  byte byte_2 = 0;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_2 = Wire.read();

  address++;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_1 = Wire.read();

  rcvData = ((byte_2 << 0) & 0xFFFFFF) + ((byte_1 << 8) & 0xFFFFFFFF);

  return rcvData;
}

void write_int(int address, unsigned int value)
{
  if (stop_write_eeprom == false)
  {

    byte byte_2 = (value & 0xFF);
    byte byte_1 = ((value >> 8) & 0xFF);

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_2);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);

    address++;

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_1);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);
  }
}

long read_long(int address)
{
  // Define byte for received data
  long rcvData = 0xFFFF;
  byte byte_1 = 0;
  byte byte_2 = 0;
  byte byte_3 = 0;
  byte byte_4 = 0;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_4 = Wire.read();

  address++;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_3 = Wire.read();

  address++;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_2 = Wire.read();

  address++;

  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);

  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB

  // End the transmission
  Wire.endTransmission();

  // Request one byte of data at current memory address
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);

  // Read the data and assign to variable
  byte_1 = Wire.read();

  rcvData = ((byte_4 << 0) & 0xFFFFFF) + ((byte_3 << 8) & 0xFFFFFF) + ((byte_2 << 16) & 0xFFFFFF) + ((byte_1 << 24) & 0xFFFFFFFF);

  return rcvData;
}

void write_long(int address, long value)
{
  if (stop_write_eeprom == false)
  {
    byte byte_4 = (value & 0xFF);
    byte byte_3 = ((value >> 8) & 0xFF);
    byte byte_2 = ((value >> 16) & 0xFF);
    byte byte_1 = ((value >> 24) & 0xFF);

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_4);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);

    address++;

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_3);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);

    address++;

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_2);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);

    address++;

    // Begin transmission to I2C EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);

    // Send memory address as two 8-bit bytes
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB

    // Send data to be stored
    Wire.write(byte_1);

    // End the transmission
    Wire.endTransmission();

    // Add 5ms delay for EEPROM
    delay(5);
  }
}

// Schreiben der Daten ins EEPROM
void set_Eeprom()
{
  write_long(addr_Gefahrene_km, Gefahrene_km * 100);
  write_int(addr_tankinhalt_Aktuell, tankinhalt_Aktuell);
}

// Setzt die Daten des AP zurück
void AP_Set_Faktory()
{
  write_char_array(addr_ssid_ap, ssid_ap_set, 20, EEPROM_I2C_ADDRESS);
  write_char_array(addr_password_ap, password_ap_set, 20, EEPROM_I2C_ADDRESS);
}

// Stellt die Grundeinstellung her
void set_Faktory()
{
  Gefahrene_km_Ges = 0;
  write_long(addr_Gefahrene_km_Ges, Gefahrene_km_Ges);                     // Gefahrene_km_Ges wird auf 0
  pump_anzahl_Ges = 0;                                                     //
  write_int(addr_pump_anzahl_Ges, pump_anzahl_Ges);                        //
  pumpDistanz = 2000;                                                      // Abstand in m zwischen den einzelnen Ölungen
  write_int(addr_pumpDistanz, pumpDistanz);                                //
  anzahl_Pump = 1;                                                         // Anzahl der Pumpimpulse je Ölung
  write_int(addr_anzahl_Pump, anzahl_Pump);                                //
  minGeschwindigkeit = 30;                                                 // Mindestgeschwindigkeit zum Ölen
  write_int(addr_minGeschwindigkeit, minGeschwindigkeit);                  //
  tankinhalt_max = 7500;                                                   // Anzahl der Pumpimpulse die benötigt werden um den Tank zu leeren
  write_int(addr_tankinhalt_max, tankinhalt_max);                          //
  write_int(addr_tankinhalt_Aktuell, tankinhalt_max);                      //
  geschwindigkeit_Notbetrieb = 80;                                         // Geschwindigkeit die angenommen wird wenn kein Sat-Empfang ist (Notbetrieb)
  write_int(addr_geschwindigkeit_Notbetrieb, geschwindigkeit_Notbetrieb);  //
  zeit_bis_notbetrieb = 180;                                               // Zeit bis Notbetrieb in Sekunden
  write_int(addr_zeit_bis_notbetrieb, zeit_bis_notbetrieb);                //
  zeit_pumpe_ein = 50;                                                     // Zeit in ms wie lage die Pumpe eineschaltet ist
  write_int(addr_zeit_pumpe_ein, zeit_pumpe_ein);                          //
  zeit_pumpe_pause = 500;                                                  // Zeit zwischen den einzelnen Pumpimpulsen
  write_int(addr_zeit_pumpe_pause, zeit_pumpe_pause);                      //                     // Setzt Tankinhalt auf Max
  Gefahrene_km = 0.0;                                                      // Gefahrene Km wird auf 0 gesetzt
  write_long(addr_Gefahrene_km, Gefahrene_km);                             //
  char ssid_ap[20] = "GPS-OILER";                                          // ssid = OILER
  write_char_array(addr_ssid_ap, ssid_ap, 20, EEPROM_I2C_ADDRESS);         //
  char password_ap[20] = "";                                               // kein Password
  write_char_array(addr_password_ap, password_ap, 20, EEPROM_I2C_ADDRESS); //
  TimeAPout = 10;                                                          // Zeit bis AP aus abgeschaltet wird
  write_int(addr_TimeAPout, TimeAPout);                                    //
  pump_nach_Regen = 5;                                                     // Pumpimpulse nach Regenmodus
  write_int(addr_pump_nach_Regen, pump_nach_Regen);                        //
  Start_disp_1 = 2;                                                        //
  write_int(addr_Start_disp_1, Start_disp_1);                              // Zeit Display 1
  Start_disp_2 = 5;                                                        //
  write_int(addr_Start_disp_2, Start_disp_2);                              // Zeit Display 2
  oilsymbol_Zeit = 6;                                                      //
  write_int(addr_oilsymbol_Zeit, oilsymbol_Zeit);                          // Zeit Ölsymbol ein
  write_long(addr_pump_anzahl_Ges, 0);                                     // Gesamt Pumpimpulse auf "0"
  sw_Regensensor_ein = 600;                                                // Regenmodus ein
  write_int(addr_sw_Regensensor_ein, sw_Regensensor_ein);                  //
  sw_Regensensor_aus = 800;                                                // Regenmodus aus
  write_int(addr_sw_Regensensor_aus, sw_Regensensor_aus);                  //
  rainMulti = 2.0;                                                         // Multiplikator für Regenmodus
  write_long(addr_rainMulti, rainMulti);                                   //
  tankinhalt_ml = 150;                                                     // Tankinhalt in ml
  write_int(addr_tankinhalt_ml, tankinhalt_ml);                            //
  pumps_ml = 50;                                                           // Pumpimpulse pro ml
  write_int(addr_pumps_ml, pumps_ml);
  init_pump_anzahl = 0;
  write_int(addr_init_pump_anzahl, init_pump_anzahl); //
  dir_Display = true;                                 // Dir Display
  write_int(addr_dir_Display, dir_Display);           //
  ESP.restart();                                      // Reset wird durchgeführt
}

// wird durch den Interupt "Betriebsspannung" ausgelöst
IRAM_ATTR void ISR()
{
  // digitalWrite(LedPin, LOW); //Nur zu Testzwecken eingefügt
  stop_write_eeprom = true;
}

void oled_start()
{
  // Convert float to a string:
  // (<variable>,<amount of digits we are going to use>,<amount of decimal digits>,<string name>)
  // dtostrf(0, 3, 0, stringDir);                     // Wandelt die richtung in ein char um
  dtostrf(8, 3, 0, stringsateliten_anzahl);        // Wandelt die richtung in ein char um
  dtostrf(8888, 4, 0, stringGefahrene_km_display); // Wandelt die Gefahrene_km_display in ein char um
  dtostrf(888, 2, 0, stringSpeed);                 // Wandelt die Variable geschwindigkeit in ein char um

  display.clearDisplay(); // Clear the display so we can refresh

  // Ändert die schriftart auf Bold 12pt
  display.setFont(&FreeMonoBold12pt7b); // Set a custom font

  // Zeichnet einen Rahmen für die gefahrenen km
  display.drawRoundRect(1, 27, 75, 25, 4, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)

  // Zeichnet den Rahmen für die Geschwindigkeit
  display.drawRoundRect(79, 0, 48, 25, 4, WHITE); // Draw rectangle (x,y,width,height,color)

  // Zeichnet den Rahmen für die Richtung
  display.drawRoundRect(79, 27, 48, 25, 4, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)

  // Zeigt die Geschwindigkeit an
  display.setCursor(83, 20); // (x,y)
  display.println(stringSpeed);

  // Zeigt die entspechende Himmelsrichtung zur Gradzahl an
  display.setCursor(98, 46); // (x,y)
  display.print("N");

  // Zeigt die gefahrenen Km mit der Tankfüllung an
  display.setCursor(18, 46);
  display.println(stringGefahrene_km_display);

  // Rahmen und Balken für die Anzeige des Tankinhalt
  display.drawRect(1, 55, 127, 9, 1); // Border of the bar chart

  // Hier wied der Balken für den Tankinhalt Berechnet und Angezeig
  tankinhalt_Anzeige = map(tankinhalt_Aktuell, 0, tankinhalt_max, 0, 127);

  display.fillRect(0, 55, tankinhalt_Anzeige, 9, WHITE); // Draws the bar depending on the sensor value
  display.drawRect(0, 55, 128, 9, BLACK);                // Draw rounded rectangle (x,y,width,height,radius,color)

  // Hier wird das Symbol für den Satempfang angezeigt
  display.drawBitmap(40, 6, satelite, 16, 16, 1);

  display.setCursor(32, 20); // (x,y)
  display.println(stringsateliten_anzahl);

  // Hier wird das Sympol für das Ölen angezeigt
  display.drawBitmap(1, 6, oilcan, 16, 16, 1);

  // Hier wird der Regenmodus angezeigt
  display.drawBitmap(20, 6, raining, 16, 16, 1);

  display.display(); // Print everything we set previously
}

void setup()
{
  delay(300);
  Serial.begin(115200);

  inputString.reserve(200);

  GPS_serial.begin(GPSBaud);
  Wire.begin();
  delay(200);

  reed_Eeprom();
  delay(200);

  if (Factory_init > 0)
  {
    write_int(addr_Factory_init, 0);
    set_Faktory();
    delay(100);
  }

  if (Start_disp_1 >= 10)
  {
    Start_disp_1 = 10;
  }

  start_Disp_1();
  delay(Start_disp_1 * 1000);

  if (Start_disp_2 >= 10)
  {
    Start_disp_2 = 10;
  }

  start_Disp_2();
  delay(Start_disp_2 * 1000);

  TimeAPoutmillis = (millis() + (TimeAPout * 60000)); // Startzeit für die abschaltung des AP

  pinMode(regenSensor, INPUT);
  pinMode(U_VCC2, INPUT);
  pinMode(Wlan_Reset, INPUT);
  pinMode(oilPin, OUTPUT);
  pinMode(LedPin, OUTPUT);
  pinMode(U_Sensor, OUTPUT);
  digitalWrite(LedPin, HIGH);
  digitalWrite(oilPin, HIGH);
  // ISR
  attachInterrupt(digitalPinToInterrupt(U_VCC2), ISR, FALLING); // Respond to falling edges on the pin  //RISING , FALLING

  // Hier wird im Vorfeld schon mal der Sensormittelwert gebildet
  digitalWrite(U_Sensor, HIGH);
  delay(20);
  regensensorMittelwert = analogRead(regenSensor);
  for (int i = 0; i < 5; i++)
  {
    regensensorMittelwert = 0.85 * regensensorMittelwert + 0.15 * analogRead(regenSensor);
  }

  pump_anzahl_soll = init_pump_anzahl;
  if (pump_anzahl_soll >= 0)
  {
    oilen = true;
  }

  WiFi_Start_AP();
  Status_Serial();
}

void loop()
{
  if (GPS_serial.available() > 0)
  {
    serial_data++;
  }

  while (GPS_serial.available() > 0)
    gps.encode(GPS_serial.read());

  if (millis() >= Next_Loop)
  {
    Next_Loop = (millis() + 1000);
    update_GPS_Data();
    update_km();
    update_Rain();
    update_Oelen();

    if (oled_akt == false)
    {
      update_Display();
    }
  }

  if (oilen || Spuelen)
  {
    if (Spuelen)
    {
      pump_anzahl_soll = 1;
      Oelen();
    }

    if (geschwindigkeit >= minGeschwindigkeit)
    {
      Oelen();
    }
  }

  if (digitalRead(Wlan_Reset) == LOW)
  {
    AP_Set_Faktory();
  }

  if (millis() >= TimeAPoutmillis)
  {
    WiFi.mode(WIFI_OFF);
    TimeAPout_set = true;
  }

  if (TimeAPout_set == false)
  {
    WiFI_Traffic();
  }
}
