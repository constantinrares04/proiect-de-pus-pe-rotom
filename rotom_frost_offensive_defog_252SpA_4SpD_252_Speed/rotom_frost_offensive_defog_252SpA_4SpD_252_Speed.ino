#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

typedef struct pantry_list {
  String nume;
  int cantitate;
  struct pantry_list  *next, *prev;
}pantry_list;

pantry_list *pantry=NULL;
pantry_list *point_pantry=NULL;
int pin15;
int pin19;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

pantry_list  * pantry_restock(pantry_list * head, String nume, int cantitate){
  if(head==NULL){
    pantry_list  *point=new pantry_list;
    point->nume=nume;
    point->cantitate=cantitate;
    point->next=point;
    point->prev=point;
    return point;
  }
  pantry_list *point_a=head;
  pantry_list *point_b=new pantry_list;
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

// void pantry_check(pantry_list  *head){
//   pantry_list  *point=head;
//     while(point!=NULL || point->next!=head){
//     Serial.print(point->nume); 
//     Serial.print(" ");
//     Serial.println(point->cantitate);
//     point=point->next;
//   }
//    delay(250);
// }

void display_ingredient(pantry_list *point_pantry){
  display.clearDisplay();
  display.setCursor(1, 1);
  display.println(point_pantry->nume);
  display.setCursor(1, 40);
  display.print("Qty: ");
  display.println(point_pantry->cantitate);
  //display.println(analogRead(15));
  display.display();
  delay(10);
  return;
}

void merg_la_LIDL(){
 String nume;
 int cantitate;
 LittleFS.begin();
 File items=LittleFS.open("/items.txt");
 while(items.available()){
  nume=items.readStringUntil('-');
  nume.trim();
  cantitate = items.readStringUntil('\n').toInt();
  pantry=pantry_restock(pantry,nume,cantitate);
 }
 items.close();
}

void setup() {
 pinMode(15,INPUT);
 //Serial.begin(230400);
 merg_la_LIDL();
 point_pantry=pantry;
 display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 display.clearDisplay();
 display.setTextSize(2);
 display.setTextColor(WHITE);

}

void loop() {
 display_ingredient(point_pantry);
 if(analogRead(15)>3000){
  point_pantry=point_pantry->next;
  delay(250);
 }else if(analogRead(15)>2000){
  point_pantry=point_pantry->prev;
  delay(250);
 }

}
