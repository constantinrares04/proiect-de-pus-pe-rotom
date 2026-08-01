#include "Rotom.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>

Ingredient::Ingredient(String name, String ing_id, int qnt){
  this->name=name;
  this->ing_id=ing_id;
  this->qnt=qnt;
  this->next=this;
  this->prev=this;
}

Ingredient::~Ingredient(){
  if(this->next!=this){
    this->prev->next=this->next;
    this->next->prev=this->prev;
  }
}

void Ingredient::Add_Ing(Ingredient* &head, int n){
  if(head==nullptr){
    head=this;
    return;
  }
  Ingredient *current=head;
  if(n==-1){
    current=head->prev;
    head->prev=this;
    current->next=this;
    this->prev=current;
    this->next=head;
  }else{
    for(int i=1;i<n && current->next!=head;i++){
      current=current->next;
    }
    this->prev=current;
    this->next=current->next;
    this->next->prev=this;
    current->next=this;
  }
}

void Ingredient::Display(int n){
  Ingredient* current=this;
  if(current!=nullptr){
    do{
      Serial.println(current->name + " " + current->qnt);
      current=current->next;
      n--;
    }while(current!=nullptr && current!=this && n>0);
  }
}

Ingredient* Ingredient::Next(){
  return this->next;
};

Ingredient* Ingredient::Prev(){
  return this->prev;
};

void Ingredient::Use(int n){
  if(n>0){
    this->qnt-=n;
  }
}

void Ingredient::Add(int n){
  if(n>0){
    this->qnt+=n;
  }
}

void Ingredient::Remove(Ingredient* &head){
  if(this==head){
    if(this->next==this){
      head=nullptr;
    }else{
      head=this->next;
    }
  }
  delete this;
}


void Get_List(Ingredient* &head, String file){
  String qnt;
  String name;
  String ing_id;
  File data=LittleFS.open(file.c_str(), "r");
  while(data.available()){
    name=data.readStringUntil(':');
    ing_id=data.readStringUntil(':');
    qnt=data.readStringUntil('\n');
    name.trim();
    ing_id.trim();
    qnt.trim();
    Ingredient *point=new Ingredient(name,ing_id,qnt.toInt());
    point->Add_Ing(head);
  }
  data.close();
}