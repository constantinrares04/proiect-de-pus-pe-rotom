#ifndef ROTOM_H
#define ROTOM_H

class Ingredient{
	private:
  	int qnt;
		char* name;
    char* ing_id;
		Ingredient *next;
    Ingredient *prev;
	public:
		Ingredient(const char* name, const char* ing_id, int qnt);
    ~Ingredient();
    Ingredient* Next();
    Ingredient* Prev();
    const char* Get_Name();
    const char* Get_ID();
    int Get_Qnt();
    void Add_Ing(Ingredient* &head, int n=-1);
    void Use(int n=1);
    void Add(int n=1);
    void Remove(Ingredient* &head);
};

void Get_List(Ingredient* &head, const char* file);

#endif