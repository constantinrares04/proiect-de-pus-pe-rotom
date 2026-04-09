#include <string.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
//#include <TJpg_decoder.h>

typedef struct ingrediente{
  String nume;
  int cantitate;
  struct ingrediente  *next,*prev;
}ingrediente;

ingrediente *lista_ingrediente=NULL;
TFT_eSPI tft = TFT_eSPI();

void pornire_wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin("Rares","123456789");
  while(WiFi.status()!= WL_CONNECTED){
    delay(500);
  }
  return;
}

void afisare_reteta(JSONVar reteta){
  String nume_reteta,ingrediente[20], cantitate[20];
  nume_reteta = String(reteta["strMeal"]);
  for(int i=0;i<20;i++){
    ingrediente[i]=String(reteta["strIngredient"+String(i+1)]);
    cantitate[i]=String(reteta["strMeasure"+String(i+1)]);
  }
  tft.fillScreen(TFT_BLACK);
  tft.println(nume_reteta+"\n");
  tft.setTextSize(1);
  for(int i=0;i<20;i++){
    tft.println(cantitate[i]+" "+ingrediente[i]);
  }
  while(!axaY()){
    delay(50);
  }
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0);
  tft.println(String(reteta["strInstructions"]));
  delay(250);
  tft.setTextSize(2);
  return;
}

void get_reteta_random(){
  tft.setCursor(0,0);
  tft.println("Fetching...");
  tft.setCursor(0, 0);
  if(WiFi.status()!= WL_CONNECTED){
    tft.drawString("Wifi neconectat!",0,0);
    return;
  }
  HTTPClient http;
  http.begin("https://www.themealdb.com/api/json/v1/1/random.php");
  int cod_raspuns = http.GET();
  if (cod_raspuns > 0) {
    JSONVar payload = JSON.parse(http.getString());
    JSONVar reteta = payload["meals"][0];
    afisare_reteta(reteta);
  }else{
    tft.fillScreen(TFT_BLACK);
    tft.println("Eroare HTTP_GET!");
  }
  http.end();
  while(!axaY()){
    delay(5);
  }
}

ingrediente *creare_ingredient(ingrediente *head, String nume, int cantitate){
  if(head==NULL){
    ingrediente *point=new ingrediente;
    point->nume=nume;
    point->cantitate=cantitate;
    point->next=point;
    point->prev=point;
    return point;
  }
  ingrediente *point_a=head;
  ingrediente *point_b=new ingrediente;
  while(point_a->next!=head){
    point_a=point_a->next;
  }
  point_b->nume=nume;
  point_b->cantitate=cantitate;
  point_b->next=head;
  point_b->prev=point_a;
  point_a->next=point_b;
  head->prev=point_b;
  return head;
}

void afisare_butoane(){
  tft.fillRect(0,290,80,35,TFT_RED);
  tft.fillRect(80,290,80,35,TFT_BLUE);
  tft.fillRect(160,290,80,35,TFT_GREEN);
}

void afisare_ingredient(ingrediente *curent){
  ingrediente *lista=curent;
  tft.fillScreen(TFT_BLACK);
  afisare_butoane();
  tft.setCursor(0, 0);
  tft.print(">");
  for(int i=0;i<=7;i++){
    tft.println((String(lista->nume)+" - "+String(lista->cantitate)));
    lista=lista->next;
  }
  delay(100);
}

void creare_stoc(){
  String nume;
  int cantitate;
  LittleFS.begin();
  File items=LittleFS.open("/items.txt");
  while(items.available()){
    nume=items.readStringUntil('-');
    nume.trim();
    cantitate = items.readStringUntil('\n').toInt();
    lista_ingrediente=creare_ingredient(lista_ingrediente,nume,cantitate);
  }
  items.close();
}

int axaY() {
  int y=0;
  pinMode(27, OUTPUT); digitalWrite(27, HIGH);
  pinMode(13, OUTPUT); digitalWrite(13, LOW);
  pinMode(14, INPUT);
  pinMode(26, INPUT);
  delay(5);
  for(int i=0;i<=20;i++){
    y+=analogRead(35);
    delay(2);
  }
  pinMode(26, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(13, OUTPUT);
  delay(5);
  return y/400; 
}

void setup() {
  creare_stoc();
  pornire_wifi();
  tft.init();
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  afisare_ingredient(lista_ingrediente);
}

void loop() {
  int y=axaY();
  if(y>44){
    lista_ingrediente=lista_ingrediente->prev;
    afisare_ingredient(lista_ingrediente);
  }else if(y>25){
    tft.fillScreen(TFT_BLACK);
    get_reteta_random();
  }else if(y>6){
    lista_ingrediente=lista_ingrediente->next;
    afisare_ingredient(lista_ingrediente);
  }
  delay(250);
}
