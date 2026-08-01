#include "Rotom.h"
#include <LittleFS.h>
#include <string.h>

Ingredient::Ingredient(const char *name, const char *ing_id, int qnt){
  this->name=strdup(name);
  this->ing_id=strdup(ing_id);
  this->qnt=qnt;
  this->next=this;
  this->prev=this;
}

Ingredient::~Ingredient(){
  free(this->name);
  free(this->ing_id);
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

Ingredient* Ingredient::Next(){
  return this->next;
}

Ingredient* Ingredient::Prev(){
  return this->prev;
}

const char* Ingredient::Get_Name(){
  return this->name;
}

const char* Ingredient::Get_ID(){
  return this->ing_id;
}

int Ingredient::Get_Qnt(){
  return this->qnt;
}

void Get_List(Ingredient* &head, const char* file){
  File data=LittleFS.open(F(file),"r");
  char name[32];
  char ing_id[32];
  char qnt[12];
  while(data.available()){
    size_t l1=data.readBytesUntil(':',name,sizeof(name)-1);
    size_t l2=data.readBytesUntil(':',ing_id,sizeof(ing_id)-1);
    size_t l3=data.readBytesUntil('\n',qnt,sizeof(qnt)-1);
    name[l1]='\0';
    ing_id[l2]='\0';
    qnt[l3]='\0';
    if(l1>0){
      Ingredient *point=new Ingredient(name,ing_id,atoi(qnt));
      point->Add_Ing(head, -1);
    }
  }
  data.close();
}