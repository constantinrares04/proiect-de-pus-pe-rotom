#ifndef ROTOM_H
#define ROTOM_H

#include <Arduino.h>
#include <string>
#include <LittleFS.h>
#include <TFT_eSPI.h>

class Ingredient{
	private:
  	int qnt;
		std::string name;
    std::string ing_id;
		Ingredient *next;
    Ingredient *prev;
	public:
		Ingredient(std::string name, std::string ing_id, int qnt){
      this->name=name;
      this->ing_id=ing_id;
      this->qnt=qnt;
      this->next=this;
      this->prev=this;
		}

    ~Ingredient(){
      if(this->next!=this){
        this->prev->next=this->next;
        this->next->prev=this->prev;
      }
    }

    void Add_Ing(Ingredient* &head, int n=-1){
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

    void Display(int n=1){
      Ingredient* current=this;
      if(current!=nullptr){
        do{
          //partea de afisare pe tft
          current=current->next;
          n--;
        }while(current!=nullptr && current!=this && n>0);
      }
    }

    void Use(int n=1){
      if(n>0){
        this->qnt-=n;
      }
    }

    void Add(int n=1){
      if(n>0){
        this->qnt+=n;
      }
    }

    void Remove(Ingredient* &head){
      if(this==head){
        if(this->next==this){
          head=nullptr;
        }else{
          head=this->next;
        }
      }
      delete this;
    }
};

void Get_List(Ingredient* &head, String file){
  String qnt;
  String name;
  String ing_id;
  LittleFS.begin();
  File data=LittleFS.open("/"+file);
  while(data.available()){
    name=data.readStringUntil(':');
    ing_id=data.readStringUntil(':');
    qnt=data.readStringUntil('\n');
    name.trim();
    ing_id.trim();
    qnt.trim();
    Ingredient *point=new Ingredient(name.c_str(),ing_id.c_str(),qnt.toInt());
    point.Add_Ing(head);
  }
  data.close();
  LittleFS.end();
}



#endif