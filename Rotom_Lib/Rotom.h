#ifndef ROTOM_H
#define ROTOM_H

#include <Arduino.h>

class Ingredient{
	private:
  	int qnt;
		String name;
    String ing_id;
		Ingredient *next;
    Ingredient *prev;
  public:
		Ingredient(String name, String ing_id, int qnt);
    ~Ingredient();
    void Add_Ing(Ingredient* &head, int n=-1);
    void Display(int n=1);
    Ingredient* Next();
    Ingredient* Prev();
    void Use(int n=1);
    void Add(int n=1);
    void Remove(Ingredient* &head);
};

void Get_List(Ingredient* &head, String file);

#endif