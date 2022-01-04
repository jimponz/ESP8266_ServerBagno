#include <espnow.h>
#include <CTBot.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>

#define IP_SERVER_PRINCIPALE "192.168.1.251"

//typedef struct struct_message {
//    String isAlive;
//    bool statusAntifurto;
//    bool statusLuceBagno;
//    bool statusLuceStanza;
//    bool statusScaldabagno;
//    
//    int ultmovgiostanza;
//    int ultmovorastanza;
//    int ultmovminstanza;
//    int ultmovsecstanza;
//    int epomovsta;
//
//    int ultmovgiobagno;
//    int ultmovorabagno;
//    int ultmovminbagno;
//    int ultmovsecbagno;
//    int epomovbag;
//} struct_message;
//
//struct_message myData;

typedef struct struct_message_sent {
    
    bool stateLuceBagno;
    bool stateLuceStanza;
    bool stateScaldabagno;

} struct_message_sent;
struct_message_sent infoServerBagno;

unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;

const char* ssid = "";  // Insert Wi-Fi SSID (The wifi's name)
const char* password = ""; // Insert Wi-Fi password

#define BotToken "" // Insert Telegram Bot Token
int64_t CHAT_ID = ; // Insert Chat ID

CTBot myBot;

String IPServerPrincipale = IP_SERVER_PRINCIPALE;

ESP8266WebServer server(80);
Servo servo;
Servo servoLuceBagno;
Servo servoLuceStanza;

const int servoPin = 5;  //D1
const int servoLuceBagnoPin = 4 ;//D2
const int servoLuceStanzaPin = 2 ;//D4
// potrei spostare il pir su 13 D7
const int pirStanza = 12; // D6
const int pirBagno = 14 ;//D5

bool stateLuceStanza = false;
bool stateLuceBagno = false;
bool stateScaldabagno =false;
String ora="";

bool AutoStanza = true;
bool AutoBagno = true;

bool stateAntifurto = false;
int controllaLuce = -1;
int controllaLuceBagno = -1;
int controllaacc = -1;
int controllaspe = -1;
int oraimp=-1;
int minuimp =-1;
int oraimpspe=-1;
int minuimpspe =-1;
int mostra = -1;
int mostraspe = -1;

int statePirStanza = LOW;
int statePirBagno = LOW;            
int val = 0;   

String ultmovstanza = "";
String fineultmovstanza = "";
String ultmovbagno = "";
String fineultmovbagno = "";

long timestamp =0;

long elapsedTime = 0;

const long utcOffsetInSeconds = 7200;
String daysOfTheWeek[] = {"Domenica", "Lunedì", "Martedì", 
                  "Mercoledì", "Giovedì", "Venerdì", "Sabato"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
HTTPClient http;

// MAC SERVER PRINCIPALE CC:50:E3:63:B1:F2
uint8_t broadcastAddressPrincipale[] = {0xCC, 0x50, 0xE3, 0x63, 0xB1, 0xF2};

String IPPub = "";
String oraServerOnline = "";

//void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
//  Serial.print("Last Packet Send Status: ");
//  if (sendStatus == 0){
//    Serial.println("Delivery success");
//  }
//  else{
//    Serial.println("Delivery fail");
//  }
//}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

//  String newHostname = "ESP_Bagno";
//  WiFi.hostname(newHostname.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    
  }

  oraServerOnline = "Server online da: " + getGiornoAndOra();
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.setHostname("Esp8266_Bagno");
  
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  myBot.setTelegramToken(BotToken);
  
  if (myBot.testConnection()){
    Serial.println("\ntestConnection OK");
    myBot.sendMessage(CHAT_ID, "Server stanza avviato e bot stanza attivo"); }
  else{
    Serial.println("\ntestConnection NOK"); }
    
  timeClient.begin();
  servo.attach(servoPin);
  delay(100);
  servo.write(90);
  delay(500);  
  
  servoLuceBagno.attach(servoLuceBagnoPin);
  delay(100);
  servoLuceBagno.write(90);
  delay(500);
  
  servoLuceStanza.attach(servoLuceStanzaPin);
  delay(100);
  servoLuceStanza.write(90);
  delay(500);    

  pinMode(pirStanza, INPUT);    
  digitalWrite(pirStanza,LOW);

  pinMode(pirBagno, INPUT);   
  digitalWrite(pirBagno,LOW);
  
  
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
//  esp_now_register_send_cb(OnDataSent);

  esp_now_add_peer(broadcastAddressPrincipale, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  server.on("/", handle_OnConnect);
  
  server.on("/AccendiLuceBagno",AccendiLuceBagno);
  server.on("/SpegniLuceBagno",SpegniLuceBagno);
  server.on("/AbilitaAutoBagno",AbilitaAutoBagno);
  server.on("/DisabilitaAutoBagno",DisabilitaAutoBagno);
  server.on("/AccendiScaldaBagno",AccendiScaldaBagno);
  server.on("/SpegniScaldaBagno",SpegniScaldaBagno);
  server.on("/AccendiLuceStanza",AccendiLuceStanza);
  server.on("/AbilitaAutoStanza",AbilitaAutoStanza);
  server.on("/DisabilitaAutoStanza",DisabilitaAutoStanza);
  server.on("/SpegniLuceStanza",SpegniLuceStanza);
  server.on("/Imposta",Imposta);
  server.on("/ImpostaOff",ImpostaOff);
  server.on("/Annulla", Annulla);
  server.on("/AnnullaOff", AnnullaOff);
  server.on("/ImpostaOraAccensione", ImpostaOraAccensione);
  server.on("/ImpostaOraSpegnimento", ImpostaOraSpegnimento);
  server.on("/AttivaAntifurto", AttivaAntifurto);
  server.on("/DisattivaAntifurto", DisattivaAntifurto);
  server.on("/Princ",Princ);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  while (IPPub == ""){
     WiFiClient client;
     HTTPClient http;
     http.begin(client, "http://api.ipify.org");
   
    int httpCode = http.GET();  
    if (httpCode > 0) {IPPub = http.getString();}
    else { Serial.println("Error on HTTP request");}
    http.end();
  }
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
//  if ((millis() - lastTime) > timerDelay) {
//    myData.isAlive = "BagnoAlive";
//    myData.statusAntifurto = stateAntifurto;
//    myData.statusLuceBagno = stateLuceBagno;
//    myData.statusLuceStanza = stateLuceStanza;
//    myData.statusScaldabagno = stateScaldabagno;
//    
//    myData.ultmovgiostanza = ultmovgiostanza;
//    
//    myData.ultmovorastanza = ultmovorastanza;
//    myData.ultmovminstanza = ultmovminstanza;
//    myData.ultmovsecstanza = ultmovsecstanza;
//    myData.epomovsta = epomovsta;
//    
//    myData.ultmovgiobagno = ultmovgiobagno;
//    myData.ultmovorabagno = ultmovorabagno;
//    myData.ultmovminbagno = ultmovminbagno;
//    myData.ultmovsecbagno = ultmovsecbagno;
//    myData.epomovbag = epomovbag;
//    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
//    lastTime = millis();
//  }

   // OGNI SECONDO
  if ((millis() - lastTime) > timerDelay) {

    infoServerBagno.stateLuceBagno = stateLuceBagno;
    infoServerBagno.stateLuceStanza = stateLuceStanza;
    infoServerBagno.stateScaldabagno = stateScaldabagno;
    
    esp_now_send(broadcastAddressPrincipale, (uint8_t *) &infoServerBagno, sizeof(infoServerBagno));
    lastTime = millis();
  }

  if (AutoStanza){  
    val = digitalRead(pirStanza);   
    if (val == HIGH) {           
      if (statePirStanza == LOW) {
        timeClient.update();
        ultmovstanza =daysOfTheWeek[timeClient.getDay()]+", "+
                       timeClient.getHours() +":"+timeClient.getMinutes()+
                       ":"+timeClient.getSeconds();
//        ultmovgiostanza = timeClient.getDay();
//        ultmovorastanza = timeClient.getHours();
//        ultmovminstanza = timeClient.getMinutes();
//        ultmovsecstanza = timeClient.getSeconds(); 
//        epomovsta = timeClient.getEpochTime();              
        timestamp = millis();
        controllaLuce = 1;
        Serial.println("Rilevato movimento: ");                
        Serial.println(ultmovstanza);
        if (stateAntifurto){
          String strBot = "Rilevato movimento nella STANZETTA alle ore ";
          strBot += timeClient.getFormattedTime();
          strBot += "";
          myBot.sendMessage(CHAT_ID,strBot); 
        }         
        if (stateLuceStanza == false){AccendiLuceStanza();}
        statePirStanza = HIGH; 
      }
    } 
    else {
        if (statePirStanza == HIGH){
          timeClient.update();
          fineultmovstanza =daysOfTheWeek[timeClient.getDay()]+", "+
                       timeClient.getHours() +":"+timeClient.getMinutes()+
                       ":"+timeClient.getSeconds();
          Serial.println("Fine movimento alle: ");                
          Serial.println(fineultmovstanza);
          statePirStanza = LOW;
      }
    }
  }
  
  if (AutoBagno){
    val = digitalRead(pirBagno);   
    if (val == HIGH) {           
      if (statePirBagno == LOW) {
        timeClient.update();
        ultmovbagno =daysOfTheWeek[timeClient.getDay()]+", "+
                       timeClient.getHours() +":"+timeClient.getMinutes()+
                       ":"+timeClient.getSeconds();
//        ultmovgiobagno = timeClient.getDay();
//        ultmovorabagno = timeClient.getHours();
//        ultmovminbagno = timeClient.getMinutes();
//        ultmovsecbagno = timeClient.getSeconds();
//        epomovbag = timeClient.getEpochTime();                 
        timestamp = millis();
        controllaLuceBagno = 1;
        Serial.println("Rilevato movimento: ");                
        Serial.println(ultmovbagno);
        if (stateAntifurto){
          String strBot = "Rilevato movimento in BAGNO alle ore ";
          strBot += timeClient.getFormattedTime();
          strBot += "";
          myBot.sendMessage(CHAT_ID,strBot); 
        }         
        if (stateLuceBagno == false){AccendiLuceBagno();}
        statePirBagno = HIGH; 
      }
    } 
    else {
        if (statePirBagno == HIGH){
          timeClient.update();
          fineultmovbagno =daysOfTheWeek[timeClient.getDay()]+", "+
                       timeClient.getHours() +":"+timeClient.getMinutes()+
                       ":"+timeClient.getSeconds();
          Serial.println("Fine movimento alle: ");                
          Serial.println(fineultmovbagno);
          statePirBagno = LOW;
      }
    }
  }
  
  if ((millis() - elapsedTime) > 60000) {
        verificaOra();
        elapsedTime = millis();
    }
}

String getGiornoAndOra(){
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  String dayName = daysOfTheWeek[timeClient.getDay()];
  String currentHour = String(timeClient.getHours());
  currentHour += ":";
  currentHour += String(timeClient.getMinutes());
  currentHour += ":";
  currentHour += String(timeClient.getSeconds());
  currentHour += " ";
  
  String ret = dayName;
  ret+=", ";
  ret+=currentHour;
  return ret;
}

void AbilitaAutoBagno(){
  AutoBagno = true;
  server.send(200,"text/html",SendHTML());
}

void DisabilitaAutoBagno(){
  AutoBagno = false;
  server.send(200,"text/html",SendHTML());
}

void AccendiLuceBagno(){ 
  
  servoLuceBagno.write(180);
  delay(300);
  servoLuceBagno.write(90);
  delay(100);
  Serial.println("Luce del bagno accesa");
  stateLuceBagno =true;
  server.send(200, "text/html", SendHTML()); 
}
void SpegniLuceBagno(){ 
  
  servo.write(0);
  delay(300);
  servo.write(90);
  delay(100);
  Serial.println("Luce spenta");
  stateLuceBagno =false;
  server.send(200, "text/html", SendHTML());   
}

void AccendiLuceStanza(){ 
  servoLuceStanza.write(180);
  delay(300);
  servoLuceStanza.write(90);
  delay(100);
  Serial.println("Luce della stanza accesa");
  stateLuceStanza =true;
  server.send(200, "text/html", SendHTML());   
}

void SpegniLuceStanza(){ 
  servoLuceStanza.write(0);
  delay(300);
  servoLuceStanza.write(90);
  delay(100);
  Serial.println("Luce della stanza spenta");
  stateLuceStanza =false;
  server.send(200, "text/html", SendHTML());  
}

void AbilitaAutoStanza(){
  AutoStanza = true;
  server.send(200,"text/html",SendHTML());
}

void DisabilitaAutoStanza(){
  AutoStanza = false;
  server.send(200,"text/html",SendHTML());
}

void AttivaAntifurto(){
  stateAntifurto = true;
  Serial.println("Antifurto attivato!");
  server.send(200,"text/html",SendHTML());
}

void DisattivaAntifurto(){
  stateAntifurto = false;
  Serial.println("Antifurto disattivato!");
  server.send(200,"text/html",SendHTML());
}

void verificaOra(){
  timeClient.update();
  int ora = timeClient.getHours();
  int minu = timeClient.getMinutes();
  //Serial.println("Richiedo orario");
  if(controllaacc == 1){
    if (ora == oraimp){
       Serial.println("Controllo ora");
      if(minu == minuimp){
        Serial.println("Controllo minuti");
        
//        http.begin("http://"+IPPub+"/AnnullaTemp");
//        if (http.GET()>0){Serial.println("Controllo automatico disattivato");}
//        http.end();
        
        AccendiScaldaBagno();
        Serial.println("E' arrivata l'ora di accendere lo scaldabagno!");
        controllaacc = 0;
        oraimp = -1;
        minuimp = -1;
        server.send(200, "text/html", SendHTML());             
      }
    } 
  }

  if(controllaspe == 1){
    if (ora == oraimpspe){
       Serial.println("Controllo ora");
      if(minu == minuimpspe){
        Serial.println("Controllo minuti");
        SpegniScaldaBagno();
        Serial.println("E' arrivata l'ora di spegnere lo scaldabagno!");
        controllaspe = 0;
        oraimpspe = -1;
        minuimpspe = -1;
        server.send(200, "text/html", SendHTML());             
      }
    } 
  }
  
  if (controllaLuce == 1 && stateLuceStanza){
    if (millis()-timestamp >60000){
      SpegniLuceStanza();
      controllaLuce = 0;
    }
  }

  if (controllaLuceBagno == 1 && stateLuceBagno){
    if (millis()-timestamp >60000){
      SpegniLuceBagno();
      controllaLuceBagno = 0;
    }
  }
  
}
void AccendiScaldaBagno(){
    
  servo.write(180);
  delay(300);
  servo.write(90);
  delay(100);
  Serial.println("Scaldabagno acceso");
  stateScaldabagno =true;
  server.send(200, "text/html", SendHTML()); 
}
void SpegniScaldaBagno(){
  
  servoLuceBagno.write(0);
  delay(300);
  servoLuceBagno.write(90);
  delay(100);
  Serial.println("Scaldabagno spento");
  stateScaldabagno =false;
  server.send(200, "text/html", SendHTML()); 

}
void Annulla(){
  controllaacc = 0;
  oraimp = -1;
  minuimp = -1;
  Serial.println("Annullato");
  server.send(200,"text/html", SendHTML());
}
void AnnullaOff(){
  controllaspe = 0;
  oraimpspe = -1;
  minuimpspe = -1;
  Serial.println("Annullato");
  server.send(200,"text/html", SendHTML());
}
void ImpostaOraAccensione() {
 
 oraimp = server.arg("ora").toInt(); 
 minuimp = server.arg("minuti").toInt(); 
 
 Serial.print("Ora impostata:");
 Serial.println(oraimp);
 Serial.print("Minuti:");
 Serial.println(minuimp);
 controllaacc = 1;
 Serial.print("Controlla vale: ");
 Serial.println(controllaacc);
 mostra = 0;

 server.sendHeader("Location", "/");
 server.send(302, "text/plain", "Updated– Press Back Button");
}
void ImpostaOraSpegnimento(){
  
 oraimpspe = server.arg("ora").toInt(); 
 minuimpspe = server.arg("minuti").toInt(); 
 
 Serial.print("Ora impostata spegnimento:");
 Serial.println(oraimpspe);
 Serial.print("Minuti:");
 Serial.println(minuimpspe);
 controllaspe = 1;
 Serial.print("Controlla vale: ");
 Serial.println(controllaspe);
 mostraspe = 0;

 server.sendHeader("Location", "/");
 server.send(302, "text/plain", "Updated– Press Back Button");
}

void Imposta(){
  mostra = 1;
  server.send(200, "text/html", SendHTML()); 
}
void ImpostaOff(){
  mostraspe = 1;
  server.send(200, "text/html", SendHTML()); 
}
void handle_OnConnect() {
   server.send(200, "text/html", SendHTML()); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
void Princ(){
  server.send(200,"text/html",SendPrinc());
}
String SendPrinc(){
 String ptr = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Bagno</title><meta http-equiv = \"refresh\" content = \"0; url = http://"+IPServerPrincipale+"\"/></head></html>";
 return ptr;
}
String SendHTML(){  
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Bagno</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 40px;background-color:#18699e;} h1 {color: #FFFFFF;margin: 20px auto 20px;} h2 {color: #D6D6D6;margin: 30px auto 30px;}h3 {color: #FFFFFF;margin:30px auto 30px;}h4 {color: #59b36d;margin: 30px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}info {color: #FFFFFF;margin: 30px auto 30px;font-size:1.25em}luo{color: #D6D6D6;margin: 30px auto 30px;font-size:1.25em}risc {color: #e37507;margin: 30px auto 30px;font-size:1.25em}acc {color: #ab0e0e;margin: 30px auto 30px;font-size:1.25em}spe {color: #3ca607;margin: 30px auto 30px;font-size:1.25em}\n";
  ptr +="button{background-color:#8C9992 ; color:white; font-family:inherit; margin:10px; font-size:1.5em;border-radius: 40%;transition-duration:0.4s;}\n";
  ptr +="button:hover {font-size: 1.75em;}\n";
  ptr +="</style>\n";

  ptr +="<script>\n";
  ptr +="setInterval(loadDoc,10000);\n";
  ptr +="function loadDoc() {\n";
  ptr +="var xhttp = new XMLHttpRequest();\n";
  ptr +="xhttp.onreadystatechange = function() {\n";
  ptr +="if (this.readyState == 4 && this.status == 200) {\n";
  ptr +="document.getElementById(\"webpage\").innerHTML =this.responseText}\n";
  ptr +="};\n";
  ptr +="xhttp.open(\"GET\", \"/\", true);\n";
  ptr +="xhttp.send();\n";
  ptr +="}\n";
  ptr +="</script>\n";
  
  ptr +="</head>\n";

  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  
  if (!stateAntifurto){ ptr+= "<a href=\"AttivaAntifurto\"><button>Attiva antifurto</button></a>\n";}
  if (stateAntifurto){ ptr+= "<a href=\"DisattivaAntifurto\"><button>Disattiva antifurto</button></a>\n";}
  
  ptr+= "<a href=\"Princ\"><button>Server principale</button></a>\n";

  ptr += "<button type=\"button\" onclick =\"\" class=\"btn rounded-pill btn-success text-white button-border-white \"><strong>";
  ptr += oraServerOnline;
  ptr += "</strong></button>\n";
  
  ptr +="<h1>Bagno</h1>\n";
  ptr+="<luo>Luce del bagno</luo>\n";
  if(stateLuceBagno == true){ptr+= "<acc>accesa</acc>\n";}
  if(stateLuceBagno == false){ptr+= "<spe>spenta</spe>\n";}
  ptr+= "<br><br>\n";
  if (!AutoBagno){ ptr+= "<a href=\"AbilitaAutoBagno\"><button>Abilita controllo automatico</button></a>\n";}
  if (AutoBagno){ ptr+= "<a href=\"DisabilitaAutoBagno\"><button>Disabilita controllo automatico</button></a>\n";}  
  ptr+= "<a href=\"AccendiLuceBagno\"><button>Accendi la luce</button></a>\n";
  ptr+= "<a href=\"SpegniLuceBagno\"><button>Spegni la luce</button></a>\n";
  ptr+= "<br>\n";
  ptr+= "<br><luo>Scaldabagno</luo><br>";
  ptr+= "<br><risc>Scaldabagno </risc>\n";
  if(stateScaldabagno == true){ptr+= "<acc>acceso</acc>\n";}
  if(stateScaldabagno == false){ptr+= "<spe>spento</spe>\n";}
  ptr+= "<a href=\"AccendiScaldaBagno\"><button>Accendi</button></a>\n";
  ptr+= "<a href=\"SpegniScaldaBagno\"><button>Spegni</button></a>\n";
  if (mostra == 1){
    ptr+= "<form action=\"/ImpostaOraAccensione\">\n";
    ptr+="<info>Inserisci l'ora a cui vuoi accendere lo scaldabagno:</info><br>\n";
    ptr+="<br><input type=\"text\" name=\"ora\" ><br>\n";
    ptr+="<info>Inserisci i minuti:</info><br>\n";
    ptr+="<br><input type=\"text\" name=\"minuti\" >\n";
    ptr+="<br><br>\n";
    ptr+="<input type=\"submit\" value=\"Programma accensione\">\n";
    ptr+="</form> ";
  }
  ptr+="<br><risc>Ora accensione scaldabagno: </risc>\n";
  ptr+="<spe>\n";
  if (oraimp == -1){ptr+="";
                    ptr+="<spe>nessuna</spe>\n";
                    ptr+="<a href=\"Imposta\"><button>Imposta</button></a>\n";}
  else { ptr+=oraimp;
          ptr+=":\n";}
 
  if (minuimp == -1){ptr+="";}
  else { ptr+=minuimp;}
  ptr+= "</spe>\n";
  if (oraimp != -1)ptr+= "<a href=\"Annulla\"><button>Annulla</button><a/>";
  ptr+= "<br>\n";

  if (mostraspe == 1){
    ptr+= "<form action=\"/ImpostaOraSpegnimento\">\n";
    ptr+="<info>Inserisci l'ora a cui vuoi spegnere lo scaldabagno:</info><br>\n";
    ptr+="<br><input type=\"text\" name=\"ora\" ><br>\n";
    ptr+="<info>Inserisci i minuti:</info><br>\n";
    ptr+="<br><input type=\"text\" name=\"minuti\" >\n";
    ptr+="<br><br>\n";
    ptr+="<input type=\"submit\" value=\"Programma accensione\">\n";
    ptr+="</form> ";
  }
  ptr+="<risc>Ora spegnimento scaldabagno: </risc>\n";
  ptr+="<spe>\n";
  if (oraimpspe == -1){ptr+="";
                    ptr+="<spe>nessuna</spe>\n";
                    ptr+="<a href=\"ImpostaOff\"><button>Imposta</button></a>\n";}
  else { ptr+=oraimpspe;
          ptr+=":\n";}
 
  if (minuimpspe == -1){ptr+="";}
  else { ptr+=minuimpspe;}
  ptr+= "</spe>\n";
  if (oraimpspe != -1)ptr+= "<a href=\"AnnullaOff\"><button>Annulla</button><a/>";
  ptr+= "<br><br>\n";
  
  ptr+="<luo>Luce stanza </luo>\n";
  if(stateLuceStanza == true){ptr+= "<acc>accesa</acc>\n";}
  if(stateLuceStanza == false){ptr+= "<spe>spenta</spe>\n";}
  ptr+= "<br><br>\n";
  if (!AutoStanza){ ptr+= "<a href=\"AbilitaAutoStanza\"><button>Abilita controllo automatico</button></a>\n";}
  if (AutoStanza){ ptr+= "<a href=\"DisabilitaAutoStanza\"><button>Disabilita controllo automatico</button></a>\n";}
  ptr+= "<a href=\"AccendiLuceStanza\"><button>Accendi la luce</button></a>\n";
  ptr+= "<a href=\"SpegniLuceStanza\"><button>Spegni la luce</button></a>\n";
  ptr+= "<br>\n";
  ptr+= "<spe>Ultimo movimento rilevato in STANZA: \n";
  ptr+= ultmovstanza;
  ptr+= "</spe>";
  ptr+= "<br>\n";
  ptr+= "<spe>Ultimo movimento rilevato in BAGNO: \n";
  ptr+= ultmovbagno;
  ptr+= "</spe>";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
} 
