// M9DES implemented on ESP8266
// Firmware for the Transmitter
// Distributed under the MIT License
// © Copyright Maxim Bortnikov 2021
// For more information please visit
// https://github.com/Northstrix/M9DES_ESP8266
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Hash.h>
#include <FS.h>
#include <DES.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(0, 2); // RX, TX
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
#include "GBUS.h"
DES des;
GBUS bus(&mySerial, 3, 25);
uint8_t broadcastAddress[] = {0x5C, 0xCF, 0x7F, 0xFD, 0x85, 0x1D};
char str[17];
char right_half[8];
int ch_num;
struct myStruct {
  char x;
  byte sc;
};

typedef struct struct_message {
  byte left_prt[8];
  byte rght_prt[8];
} struct_message;

// Create a struct_message called myData
struct_message myData;

uint8_t Forward_S_Box[16][16] = {  
    {0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76},  
    {0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0},  
    {0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15},  
    {0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75},  
    {0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84},  
    {0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF},  
    {0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8},  
    {0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2},  
    {0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73},  
    {0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB},  
    {0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79},  
    {0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08},  
    {0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A},  
    {0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E},  
    {0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF},  
    {0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16}  
};

void IV_incrementer(){
  // Read IVs
  String f = readFile(SPIFFS, "/FIV.txt");
  String s = readFile(SPIFFS, "/SIV.txt");
  String t = readFile(SPIFFS, "/TIV.txt");
  String foiv = readFile(SPIFFS, "/FOIV.txt");
  String fiiv = readFile(SPIFFS, "/FIIV.txt");
  // Convert IVs to the int
  unsigned int fir = f.toInt();
  unsigned int sec = s.toInt();
  unsigned int thi = t.toInt();
  unsigned int fou = foiv.toInt();
  unsigned int fif = fiiv.toInt();
  // Increment IVs
  fir++;
  sec++;
  thi++;
  fou++;
  fif++;
  // Convert IVs back to the strings
  f = String(fir);
  s = String(sec);
  t = String(thi);
  foiv = String(fou);
  fiiv = String(fif);
  // Save new IVs to the SPIFFS
  writeFile(SPIFFS, "/FIV.txt", f);
  writeFile(SPIFFS, "/SIV.txt", s);
  writeFile(SPIFFS, "/TIV.txt", t);
  writeFile(SPIFFS, "/FOIV.txt", foiv);
  writeFile(SPIFFS, "/FIIV.txt", fiiv);
}

void send_IV(){
  String fifth_iv = readFile(SPIFFS, "/FIIV.txt");
  byte in[8];
  for (int i = 0; i < 8; i++){
      char x = fifth_iv[i];
      in[i] = int(x);
  }
  byte out[8];
  byte key[] = { 
                  0x3D, 0x07, 0x7E, 0x7F, 0xAB, 0x63, 0xAB, 0xD6,
                  0x0E, 0x62, 0x05, 0x45, 0x90, 0xB8, 0xD2, 0x1F,
                  0xF6, 0x16, 0xCC, 0x66, 0xD5, 0x07, 0x80, 0x86,
                };
  des.tripleEncrypt(out, in, key);
  for(int i = 0; i<8; i++)
  myData.rght_prt[i] = out[i];
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
    lcd.setCursor(0, 0);
    lcd.print("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
    lcd.setCursor(0, 0);
    lcd.print("Delivery failed");
  }
}

void clear_str(){
  for (int i = 0; i<16; i++)
  str[i] = 32;
  str[16] = '\0';
}

String readFile(fs::FS &fs, const char * path){
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, String IV){
  //Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(IV)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

void Forw_S_Box(char first_eight[8], bool half_of_m){
  byte aft_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = first_eight[i];
  int sec = first_eight[j];
  if (fir < 16)
  strOne += 0;
  strOne +=  String(fir, HEX);
  if (sec < 16)
  strTwo += 0;
  strTwo +=  String(sec, HEX);  
  /*
  Serial.print(strOne);
  Serial.println("");
  Serial.print(strTwo);
  Serial.println("");
  */
  char chars_f[3];
  char chars_s[3];
  strOne.toCharArray(chars_f, 3);
  strTwo.toCharArray(chars_s, 3);
  chars_f[2] = '\0';
  chars_s[2] = '\0';
  /*
  Serial.print(chars_f[0]);
  Serial.print(chars_f[1]);
  Serial.println("");
  Serial.print(chars_s[0]);
  Serial.print(chars_s[1]);
  Serial.println("");
  */
  int flc = getNum(chars_f[0]);
  int frc = getNum(chars_f[1]);
  int slc = getNum(chars_s[0]);
  int src = getNum(chars_s[1]);
  /*
  Serial.printf("%x",Forward_S_Box[flc][frc]);
  Serial.print(" ");
  Serial.printf("%x",Forward_S_Box[slc][src]);
  Serial.print(" ");
  */
  aft_box[i] = ("%x",Forward_S_Box[flc][frc]);
  aft_box[j] = ("%x",Forward_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
  Serial.printf("%x",aft_box[i]);
  Serial.print(" ");
  }
  */
  first_xor(aft_box,half_of_m);
}

void first_xor(byte aft_forw_box[8], bool half_of_m){
  String stiv1 = readFile(SPIFFS, "/FIV.txt");
  int stiv1_len = stiv1.length() + 1;
  char iv1[stiv1_len];
  stiv1.toCharArray(iv1, stiv1_len);
  byte aft_xor[8];
  for(int i = 0; i<8; i++){
   aft_xor[i] = aft_forw_box[i] ^ iv1[i];
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.printf("%x", aft_xor[i]);
   Serial.print(" ");
  }
  */
  first_3des(aft_xor, half_of_m);
}

void first_3des(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0xA4, 0xBB, 0x5F, 0x5E, 0x51, 0x3D, 0xF0, 0x13,
                  0xCA, 0x8F, 0x2D, 0x5E, 0xE0, 0x2E, 0x94, 0x9D,
                  0xE4, 0x2E, 0x39, 0xF6, 0x0F, 0x70, 0xAE, 0x1F,
                };
  des.tripleEncrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x", aft_xor[i]);
   Serial.print(" ");
  }
  */
  Forw_S_Box_two(out, half_of_m);
}

void Forw_S_Box_two(byte first_eight[8], bool half_of_m){
  byte aft_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = first_eight[i];
  int sec = first_eight[j];
  if (fir < 16)
  strOne += 0;
  strOne +=  String(fir, HEX);
  if (sec < 16)
  strTwo += 0;
  strTwo +=  String(sec, HEX);  
  char chars_f[3];
  char chars_s[3];
  strOne.toCharArray(chars_f, 3);
  strTwo.toCharArray(chars_s, 3);
  chars_f[2] = '\0';
  chars_s[2] = '\0';
  int flc = getNum(chars_f[0]);
  int frc = getNum(chars_f[1]);
  int slc = getNum(chars_s[0]);
  int src = getNum(chars_s[1]);
  aft_box[i] = ("%x",Forward_S_Box[flc][frc]);
  aft_box[j] = ("%x",Forward_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
  Serial.printf("%x",aft_box[i]);
  Serial.print(" ");
  }
  */
  second_xor(aft_box, half_of_m);
}

void second_xor(byte aft_forw_box[8], bool half_of_m){
  String stiv2 = readFile(SPIFFS, "/SIV.txt");
  int stiv2_len = stiv2.length() + 1;
  char iv2[stiv2_len];
  stiv2.toCharArray(iv2, stiv2_len);
  byte aft_xor[8];
  for(int i = 0; i<8; i++){
   aft_xor[i] = aft_forw_box[i] ^ iv2[i];
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.printf("%x",aft_xor[i]);
   Serial.print(" ");
  }
  */
  second_3des(aft_xor, half_of_m);
}

void second_3des(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0x04, 0x65, 0x99, 0xB0, 0xD5, 0x8B, 0xC5, 0xFD,
                  0xF8, 0x55, 0x5D, 0x43, 0x23, 0x44, 0xCC, 0x2E,
                  0x1F, 0x9A, 0xF7, 0x51, 0xDC, 0x85, 0xA8, 0x4B,
                };
  des.tripleEncrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x",out[i]);
   Serial.print(" ");
  }
  */
  Forw_S_Box_three(out, half_of_m);
}

void Forw_S_Box_three(byte first_eight[8], bool half_of_m){
  byte aft_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = first_eight[i];
  int sec = first_eight[j];
  if (fir < 16)
  strOne += 0;
  strOne +=  String(fir, HEX);
  if (sec < 16)
  strTwo += 0;
  strTwo +=  String(sec, HEX);  
  char chars_f[3];
  char chars_s[3];
  strOne.toCharArray(chars_f, 3);
  strTwo.toCharArray(chars_s, 3);
  chars_f[2] = '\0';
  chars_s[2] = '\0';
  int flc = getNum(chars_f[0]);
  int frc = getNum(chars_f[1]);
  int slc = getNum(chars_s[0]);
  int src = getNum(chars_s[1]);
  aft_box[i] = ("%x",Forward_S_Box[flc][frc]);
  aft_box[j] = ("%x",Forward_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
  Serial.printf("%x",aft_box[i]);
  Serial.print(" ");
  }
  */
  third_xor(aft_box, half_of_m);
}

void third_xor(byte aft_forw_box[8], bool half_of_m){
  String stiv3 = readFile(SPIFFS, "/TIV.txt");
  int stiv3_len = stiv3.length() + 1;
  char iv3[stiv3_len];
  stiv3.toCharArray(iv3, stiv3_len);
  byte aft_xor[8];
  for(int i = 0; i<8; i++){
   aft_xor[i] = aft_forw_box[i] ^ iv3[i];
  }
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x", aft_xor[i]);
   Serial.print(" ");
  }
  */
  third_3des(aft_xor, half_of_m);
}

void third_3des(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0xBA, 0x33, 0x7B, 0x9F, 0x66, 0x38, 0xDE, 0xE7,
                  0x79, 0x0F, 0xBD, 0x98, 0xC3, 0xDE, 0x47, 0xFA,
                  0x3B, 0x5A, 0x4D, 0x7A, 0xDF, 0x09, 0x2F, 0x75,
                };
  des.tripleEncrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x", out[i]);
   Serial.print(" ");
  }
  */
  Forw_S_Box_four(out, half_of_m);
}

void Forw_S_Box_four(byte first_eight[8], bool half_of_m){
  byte aft_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = first_eight[i];
  int sec = first_eight[j];
  if (fir < 16)
  strOne += 0;
  strOne +=  String(fir, HEX);
  if (sec < 16)
  strTwo += 0;
  strTwo +=  String(sec, HEX);  
  char chars_f[3];
  char chars_s[3];
  strOne.toCharArray(chars_f, 3);
  strTwo.toCharArray(chars_s, 3);
  chars_f[2] = '\0';
  chars_s[2] = '\0';
  int flc = getNum(chars_f[0]);
  int frc = getNum(chars_f[1]);
  int slc = getNum(chars_s[0]);
  int src = getNum(chars_s[1]);
  aft_box[i] = ("%x",Forward_S_Box[flc][frc]);
  aft_box[j] = ("%x",Forward_S_Box[slc][src]);
  }
  fourth_xor(aft_box, half_of_m);
}

void fourth_xor(byte aft_forw_box[8], bool half_of_m){
  String stiv4 = readFile(SPIFFS, "/FOIV.txt");
  int stiv4_len = stiv4.length() + 1;
  char iv4[stiv4_len];
  stiv4.toCharArray(iv4, stiv4_len);
  byte aft_xor[8];
  for(int i = 0; i<8; i++){
   aft_xor[i] = aft_forw_box[i] ^ iv4[i];
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.print(aft_xor[i]);
   Serial.print(" ");
  }
  */
  if (half_of_m == false){
   for(int i = 0; i<8; i++)
    myData.left_prt[i] = aft_xor[i];
  }
  
  else{
   for(int i = 0; i<8; i++)
    myData.rght_prt[i] = aft_xor[i];
   esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
}

int getNum(char ch)
{
    int num=0;
    if(ch>='0' && ch<='9')
    {
        num=ch-0x30;
    }
    else
    {
        switch(ch)
        {
            case 'A': case 'a': num=10; break;
            case 'B': case 'b': num=11; break;
            case 'C': case 'c': num=12; break;
            case 'D': case 'd': num=13; break;
            case 'E': case 'e': num=14; break;
            case 'F': case 'f': num=15; break;
            default: num=0;
        }
    }
    return num;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initialization");
  WiFi.mode(WIFI_STA);
  ch_num = 0;
  if(!SPIFFS.begin()){
   Serial.println("An Error has occurred while mounting SPIFFS");
   return;
  }
    if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  lcd.clear();
  IV_incrementer();
  send_IV();
  IV_incrementer();
  
}

void loop() {
  bus.tick();
  if (bus.gotData()) {
    myStruct data;
    bus.readData(data);
    lcd.clear();
    if (data.sc != 0){
    if (data.sc == 3){ // Backspace
    if (ch_num > 0){
    int length = strlen(str);
    str[length-1] = '\0';
    ch_num--;
    lcd.setCursor(0, 1);
    lcd.print(str);
        }
      }
      
    if (data.sc == 2){ // Up arrow
      
    }
    
    if (data.sc == 1){ // Enter
       int i, k;
       char first_eight[8];
       for(i = 0; i<8; i++)
        first_eight[i] = str[i];
       first_eight[i] = '\0';
       for(i = 8, k = 0; i < 17; i++, k++)
        right_half[k]= str[i];
       /*Serial.println("");
       Serial.println(first_eight);
       Serial.println(right_half);
       */
       Forw_S_Box(first_eight,false);
       IV_incrementer();
       Forw_S_Box(right_half,true);
       IV_incrementer();
       ch_num = 0;
       clear_str();
    }
    
    }
    
    else{
    if (ch_num < 16){
    Serial.print(data.x);
    str[ch_num] = data.x;
    lcd.setCursor(0, 1);
    lcd.print(str);
    ch_num++;
      }
    else{
    lcd.setCursor(0, 1);
    lcd.print(str);
        }
    }
    
  }
}
