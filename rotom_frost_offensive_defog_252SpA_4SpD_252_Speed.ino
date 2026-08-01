#include "Rotom.h"
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(T_CS, T_IRQ);
#define DRAW_BUF_SIZE (240 * 10 * (LV_COLOR_DEPTH/8))
uint32_t draw_buf[DRAW_BUF_SIZE/4];

int x, y;
Ingredient *ing=nullptr;
lv_obj_t *lbl_name[6]={nullptr};
lv_obj_t *lbl_qnt[6]={nullptr};

//---------------------------------------------------------------------------

lv_obj_t * recipe_win = nullptr;
lv_obj_t * recipe_text_container = nullptr; // Pointer salvat pentru container
lv_obj_t * recipe_label = nullptr;          // Pointer salvat pentru text
JSONVar current_recipe;
bool viewing_instructions = false;

static void recipe_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        
        // Verificăm să existe label-ul și containerul în memorie ca să prevenim blocarea
        if(recipe_label == nullptr || recipe_text_container == nullptr) return;

        if(!viewing_instructions) {
            // Trecem la instrucțiuni în siguranță
            lv_label_set_text(recipe_label, (const char*)current_recipe["strInstructions"]);
            
            // Resetăm scroll-ul în sus ca utilizatorul să vadă începutul textului
            lv_obj_scroll_to_y(recipe_text_container, 0, LV_ANIM_OFF);
            viewing_instructions = true;
        } else {
            // Închidem fereastra și curățăm TOȚI pointerii ca să nu lăsăm gunoi în RAM
            lv_obj_del(recipe_win);
            recipe_win = nullptr;
            recipe_text_container = nullptr;
            recipe_label = nullptr;
            viewing_instructions = false;
        }
    }
}

void display_reteta(JSONVar reteta) {
    current_recipe = reteta;
    viewing_instructions = false;

    // Fundalul principal
    recipe_win = lv_obj_create(lv_screen_active());
    lv_obj_set_size(recipe_win, 240, 320);
    lv_obj_center(recipe_win);
    lv_obj_set_style_bg_color(recipe_win, lv_color_hex(0x020414), 0);
    lv_obj_set_style_border_width(recipe_win, 0, 0);
    lv_obj_set_style_radius(recipe_win, 0, 0);
    lv_obj_remove_flag(recipe_win, LV_OBJ_FLAG_SCROLLABLE);

    // Salvarea containerului în variabila globală
    recipe_text_container = lv_obj_create(recipe_win);
    lv_obj_set_size(recipe_text_container, 230, 245); 
    lv_obj_set_pos(recipe_text_container, 5, 10);
    lv_obj_set_style_bg_opa(recipe_text_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(recipe_text_container, 0, 0);
    lv_obj_set_style_pad_all(recipe_text_container, 5, 0);
    lv_obj_set_scrollbar_mode(recipe_text_container, LV_SCROLLBAR_MODE_AUTO);

    // Salvarea label-ului în variabila globală
    recipe_label = lv_label_create(recipe_text_container);
    lv_obj_set_width(recipe_label, 210); 
    lv_obj_set_style_text_color(recipe_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_long_mode(recipe_label, LV_LABEL_LONG_WRAP);

    // Construim string-ul pentru ingrediente
    String buffer = "";
    if (reteta.hasOwnProperty("strMeal")) {
        buffer += String((const char*)reteta["strMeal"]) + "\n\n";
    }

    for(int i = 1; i <= 20; i++) {
        String prop_ingr = "strIngredient" + String(i);
        String prop_meas = "strMeasure" + String(i);

        if (reteta.hasOwnProperty(prop_ingr.c_str()) && reteta.hasOwnProperty(prop_meas.c_str())) {
            const char* ingr_val = (const char*)reteta[prop_ingr.c_str()];
            const char* meas_val = (const char*)reteta[prop_meas.c_str()];

            if (ingr_val != nullptr && strlen(ingr_val) > 0 && 
                strcmp(ingr_val, "null") != 0 && strcmp(ingr_val, "") != 0) {
                
                String meas_str = (meas_val != nullptr) ? String(meas_val) : "";
                buffer += meas_str + " " + String(ingr_val) + "\n";
            }
        }
    }
    lv_label_set_text(recipe_label, buffer.c_str());

    // Linia decorativă (Y = 265)
    static lv_point_precise_t lin_p1[] = {{10, 265}, {230, 265}};
    lv_obj_t * line = lv_line_create(recipe_win);
    lv_line_set_points(line, lin_p1, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0x3B3B3B), 0);

    // Butonul de navigare sub linie
    lv_obj_t * btn = lv_button_create(recipe_win);
    lv_obj_set_size(btn, 150, 30);
    lv_obj_set_pos(btn, 45, 272);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4F46E5), 0);
    lv_obj_add_event_cb(btn, recipe_event_handler, LV_EVENT_CLICKED, NULL);

    // Textul ">" din interiorul butonului
    lv_obj_t * btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Urmatoarea Pagina");
    lv_obj_center(btn_lbl);
    lv_obj_set_style_text_color(btn_lbl,lv_color_hex(0xFFFFFF),0);
}

void get_reteta(String id) {
    // Curățăm eventualele spații accidentale din ID
    id.trim();
    if (id.length() == 0) {
        Serial.println("Eroare: ID-ul pentru lookup este gol!");
        return;
    }

    HTTPClient http;
    // Construim URL-ul folosind String pentru a evita erorile de buffer
    String url = "https://www.themealdb.com/api/json/v1/1/lookup.php?i=" + id;
    
    Serial.print("Pasul 2: Lansare lookup catre: ");
    Serial.println(url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JSONVar json_data = JSON.parse(payload);
        
        if (JSON.typeof(json_data) != "undefined" && json_data.hasOwnProperty("meals")) {
            display_reteta(json_data["meals"][0]);
        } else {
            Serial.println("Eroare: Structura JSON lookup invalida.");
        }
    } else {
        Serial.printf("Eroare HTTP Lookup esuata! Cod eroare: %d\n", httpCode);
        // Dacă primești cod negativ (ex: -1), înseamnă că ESP32 a pierdut conexiunea WiFi temporar
    }
    http.end();
}

void get_retete_lista() {
    if (ing == nullptr) {
        Serial.println("Eroare: Nu exista ingredient selectat.");
        return;
    }

    HTTPClient http;
    String url = "https://www.themealdb.com/api/json/v1/1/filter.php?i=" + String(ing->Get_ID());
    
    Serial.print("Pasul 1: Lansare filtru catre: ");
    Serial.println(url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    String id_gasit = ""; // Stocăm ID-ul local aici

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JSONVar json_data = JSON.parse(payload);
        
        if (JSON.typeof(json_data) != "undefined" && json_data.hasOwnProperty("meals") && json_data["meals"].length() > 0) {
            if (json_data["meals"][0].hasOwnProperty("idMeal")) {
                id_gasit = String((const char*)json_data["meals"][0]["idMeal"]);
                Serial.printf("ID extras cu succes: %s\n", id_gasit.c_str());
            }
        } else {
            Serial.println("Nu s-au gasit retete pentru acest ingredient.");
        }
    } else {
        Serial.printf("Eroare HTTP Filtrare esuata! Cod eroare: %d\n", httpCode);
    }
    
    http.end(); // Inchidem complet prima conexiune si eliberam memoria

    // Trimitem cererea de lookup DOAR dacă am extras un ID valid la pasul 1
    if (id_gasit.length() > 0) {
        delay(50); // Mică pauză necesară pentru ca stiva de rețea a ESP32 să respire
        get_reteta(id_gasit);
    }
}
//---------------------------------------------------------------------------

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data){
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    x = map(p.x, 200, 3700, 1, 240);
    y = map(p.y, 240, 3800, 1, 320);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void event_handler_btn(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    int i=(int)lv_event_get_user_data(e);
    char temp[12];
    if(i==0){
      ing = ing->Prev();
    }else if(i==1){
      ing = ing->Next();
    }else if(i==2){
      get_retete_lista();
      return;
    }
    Ingredient *point = ing;
    for(int i = 0; i < 6; i++){
      lv_label_set_text(lbl_name[i], point->Get_Name());
      lv_label_set_text(lbl_qnt[i], itoa(point->Get_Qnt(), temp, 10));
      point = point->Next();
    }
  }
}

void create_GUI(){
  lv_obj_t * lcd = lv_screen_active();
  lv_obj_set_style_bg_color(lcd,lv_color_hex(0x020414),0);
  const char* btn_labels[]={LV_SYMBOL_UP,LV_SYMBOL_DOWN,"Reteta"};
  for(int i=0;i<3;i++){
    lv_obj_t * btn = lv_button_create(lcd);
    lv_obj_set_size(btn, 60, 30);
    lv_obj_set_pos(btn, i*75+15, 280);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4F46E5), 0);
    lv_obj_add_event_cb(btn, event_handler_btn, LV_EVENT_CLICKED, (void*)i);

    lv_obj_t * lbl_btn=lv_label_create(btn);
    lv_label_set_text(lbl_btn, btn_labels[i]);
    lv_obj_center(lbl_btn);
    lv_obj_set_style_text_color(lbl_btn,lv_color_hex(0xFFFFFF),0);
  }

  char temp[12];
  Ingredient *point=ing;
  for(int i=0;i<6;i++){
    lbl_name[i]=lv_label_create(lcd);
    lv_obj_set_size(lbl_name[i], 150, 20);
    lv_obj_set_pos(lbl_name[i], 30, i*35+45);
    lv_label_set_long_mode(lbl_name[i], LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_color(lbl_name[i],lv_color_hex(0xFFFFFF),0);

    lbl_qnt[i]=lv_label_create(lcd);
    lv_obj_set_size(lbl_qnt[i], 30, 20);
    lv_obj_set_pos(lbl_qnt[i], 200, i*35+45);
    lv_obj_set_style_text_color(lbl_qnt[i],lv_color_hex(0xFFFFFF),0);

    lv_label_set_text(lbl_name[i], point->Get_Name());
    lv_label_set_text(lbl_qnt[i], itoa(point->Get_Qnt(), temp, 10));

    point=point->Next();
  }

  lv_obj_t *lbl_point=lv_label_create(lcd);
  lv_obj_set_size(lbl_point, 25, 25);
  lv_obj_set_pos(lbl_point, 10, 44);
  lv_label_set_text(lbl_point,LV_SYMBOL_RIGHT);

  static lv_point_precise_t lin_p1[]={{10,265},{230,265}};
  static lv_point_precise_t lin_p2[]={{10,17},{230,17}};

  lv_obj_t *lin_1=lv_line_create(lcd);
  lv_obj_t *lin_2=lv_line_create(lcd);
  lv_line_set_points(lin_1,lin_p1,2);
  lv_line_set_points(lin_2,lin_p2,2);
  lv_obj_set_style_line_width(lin_1, 2, 0);
  lv_obj_set_style_line_width(lin_2, 2, 0);
  lv_obj_set_style_line_color(lin_1,lv_color_hex(0xFFFFFF),0);
  lv_obj_set_style_line_color(lin_2,lv_color_hex(0xFFFFFF),0);
}

void pornire_wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin("Rares","123456789");
  while(WiFi.status()!= WL_CONNECTED){
    delay(500);
  }
  //WiFi.setTxPower(WiFi_POWER_19_5dBm);// multumim contului espboards, daca e activat consum de 3 orei mai multa putere, dar obtine ~4,5dBm
  return;
}

void setup(){
Serial.begin(115200);

  LittleFS.begin();
  Get_List(ing,"/data.txt");
  LittleFS.end();

  lv_init();
  touchscreenSPI.begin(T_CLK, T_DO, T_DIN, T_CS);
  touchscreen.begin(touchscreenSPI);

  lv_display_t *disp=lv_tft_espi_create(240, 320, draw_buf, sizeof(draw_buf));
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  pornire_wifi();
  create_GUI();
}

void loop(){
  lv_task_handler();
  lv_tick_inc(20);
  delay(20); 
}