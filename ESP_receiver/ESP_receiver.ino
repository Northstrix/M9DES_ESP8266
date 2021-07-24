// M9DES implemented on ESP8266
// Firmware for the Receiver
// Distributed under the MIT License
// © Copyright Maxim Bortnikov 2021
// For more information please visit
// https://github.com/Northstrix/M9DES_ESP8266
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Hash.h>
#include <FS.h>
#include <DES.h>
DES des;
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  byte left_prt[8];
  byte rght_prt[8];
} struct_message;
bool flag = true;
bool dec = true;
// Create a struct_message called myData
struct_message myData;
uint8_t Inv_S_Box[16][16] = {  
    {0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB},  
    {0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB},  
    {0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E},  
    {0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25},  
    {0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92},  
    {0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84},  
    {0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06},  
    {0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B},  
    {0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73},  
    {0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E},  
    {0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B},  
    {0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4},  
    {0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F},  
    {0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF},  
    {0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61},  
    {0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D}  
};

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
    //Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

void Adjust_IVs(){
  String fifth_iv = readFile(SPIFFS, "/FIIV.txt");
  unsigned int fif = fifth_iv.toInt();
  String rec_iv = "";
  byte in[8];
  for (int i = 0; i < 8; i++){
      char x = myData.rght_prt[i];
      in[i] = int(x);
  }
  byte out[8];
  byte key[] = { 
                  0x3D, 0x07, 0x7E, 0x7F, 0xAB, 0x63, 0xAB, 0xD6,
                  0x0E, 0x62, 0x05, 0x45, 0x90, 0xB8, 0xD2, 0x1F,
                  0xF6, 0x16, 0xCC, 0x66, 0xD5, 0x07, 0x80, 0x86,
                };
  des.tripleDecrypt(out, in, key);
  for(int i = 0; i<8; i++)
  rec_iv += char(out[i]);
  unsigned int r_iv = rec_iv.toInt();
  unsigned int diff = r_iv - fif;
  /*
  Serial.println("");
  Serial.println("Received IV");
  Serial.println(rec_iv);
  Serial.println("Stored IV");
  Serial.println(fif);
  */
  if(diff > 0 && diff < 50){
  IV_incrementer(diff+1);
   lcd.setCursor(0, 0);
   lcd.print("IVs adjusted!");
  }
  else{
   dec = false;
   lcd.setCursor(0, 0);
   lcd.print("Incorrect IV!");
   delay(24);
   lcd.noBacklight();
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

void IV_incrementer(int n){
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
  for(int i = 0; i<n; i++){
  fir++;
  sec++;
  thi++;
  fou++;
  fif++;
  }
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

void fourth_xor_dec(byte aft_forw_box[8], bool half_of_m){
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
  Inverse_S_Box_Four(aft_xor, half_of_m); // Decryption
}

void Inverse_S_Box_Four(byte aft_forw_box[8], bool half_of_m){
  byte aft_inv_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = aft_forw_box[i];
  int sec = aft_forw_box[j];
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
  Serial.printf("%x",Inv_S_Box[flc][frc]);
  Serial.print(" ");
  Serial.printf("%x",Inv_S_Box[slc][src]);
  Serial.print(" ");
  */
  aft_inv_box[i] = ("%x",Inv_S_Box[flc][frc]);
  aft_inv_box[j] = ("%x",Inv_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
  Serial.printf("%x",aft_inv_box[i]);
  Serial.print(" ");
  }
  */
  third_3des_dec(aft_inv_box, half_of_m);
}

void third_3des_dec(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0xBA, 0x33, 0x7B, 0x9F, 0x66, 0x38, 0xDE, 0xE7,
                  0x79, 0x0F, 0xBD, 0x98, 0xC3, 0xDE, 0x47, 0xFA,
                  0x3B, 0x5A, 0x4D, 0x7A, 0xDF, 0x09, 0x2F, 0x75,
                };
  des.tripleDecrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x", out[i]);
   Serial.print(" ");
  }
  */
  third_xor_dec(out, half_of_m);
}

void third_xor_dec(byte aft_forw_box[8], bool half_of_m){
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
  Inverse_S_Box_Three(aft_xor, half_of_m); // Decryption
}

void Inverse_S_Box_Three(byte aft_forw_box[8], bool half_of_m){
  byte aft_inv_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = aft_forw_box[i];
  int sec = aft_forw_box[j];
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
  Serial.printf("%x",Inv_S_Box[flc][frc]);
  Serial.print(" ");
  Serial.printf("%x",Inv_S_Box[slc][src]);
  Serial.print(" ");
  */
  aft_inv_box[i] = ("%x",Inv_S_Box[flc][frc]);
  aft_inv_box[j] = ("%x",Inv_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.printf("%x",aft_inv_box[i]);
   Serial.print(" ");
  }
  */
  second_3des_dec(aft_inv_box, half_of_m);
}

void second_3des_dec(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0x04, 0x65, 0x99, 0xB0, 0xD5, 0x8B, 0xC5, 0xFD,
                  0xF8, 0x55, 0x5D, 0x43, 0x23, 0x44, 0xCC, 0x2E,
                  0x1F, 0x9A, 0xF7, 0x51, 0xDC, 0x85, 0xA8, 0x4B,
                };
  des.tripleDecrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.printf("%x",out[i]);
   Serial.print(" ");
  }
  */
  second_xor(out, half_of_m);
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
  Inverse_S_Box_Two(aft_xor, half_of_m);
}

void Inverse_S_Box_Two(byte aft_forw_box[8], bool half_of_m){
  byte aft_inv_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = aft_forw_box[i];
  int sec = aft_forw_box[j];
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
  Serial.printf("%x",Inv_S_Box[flc][frc]);
  Serial.print(" ");
  Serial.printf("%x",Inv_S_Box[slc][src]);
  Serial.print(" ");
  */
  aft_inv_box[i] = ("%x",Inv_S_Box[flc][frc]);
  aft_inv_box[j] = ("%x",Inv_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.printf("%x",aft_inv_box[i]);
   Serial.print(" ");
  }
  */
  first_3des_dec(aft_inv_box, half_of_m);
}

void first_3des_dec(byte in[8], bool half_of_m){
  byte out[8];
  byte key[] = { 
                  0xA4, 0xBB, 0x5F, 0x5E, 0x51, 0x3D, 0xF0, 0x13,
                  0xCA, 0x8F, 0x2D, 0x5E, 0xE0, 0x2E, 0x94, 0x9D,
                  0xE4, 0x2E, 0x39, 0xF6, 0x0F, 0x70, 0xAE, 0x1F,
                };
  des.tripleDecrypt(out, in, key);
  /*
  for(int i = 0; i<8; i++){
   Serial.print(out[i]);
   Serial.print(" ");
  }
  */
  first_xor(out, half_of_m);
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
   Serial.print(aft_xor[i]);
   Serial.print(" ");
  }
  */
  Inverse_S_Box_one(aft_xor, half_of_m);
}

void Inverse_S_Box_one(byte aft_forw_box[8], bool half_of_m){
  byte aft_inv_box[8];
  for (int count; count<4; count++){
  String strOne = "";
  String strTwo = "";
  int i = count * 2;
  int j = count * 2 + 1;
  int fir = aft_forw_box[i];
  int sec = aft_forw_box[j];
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
  Serial.printf("%x",Inv_S_Box[flc][frc]);
  Serial.print(" ");
  Serial.printf("%x",Inv_S_Box[slc][src]);
  Serial.print(" ");
  */
  aft_inv_box[i] = ("%x",Inv_S_Box[flc][frc]);
  aft_inv_box[j] = ("%x",Inv_S_Box[slc][src]);
  }
  /*
  Serial.println("");
  for(int i = 0; i<8; i++){
   Serial.printf("%x",aft_inv_box[i]);
   Serial.print(" ");
  }
  */
  if (half_of_m == false){
   for(int i = 0; i<8; i++){
    lcd.setCursor(i, 1);
    lcd.print(char(aft_inv_box[i]));
   }
  }
  else{
   for(int i = 8; i<16; i++){
    lcd.setCursor(i, 1);
    lcd.print(char(aft_inv_box[i-8]));
   }
  }
}

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  if(dec == true){
  if (flag == false){
   Adjust_IVs();
   flag = true;
  }
  else{
  lcd.clear();
  fourth_xor_dec(myData.left_prt, false);
  IV_incrementer(1);
  fourth_xor_dec(myData.rght_prt, true);
  IV_incrementer(1);
  }
 }
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  flag = false;
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  if(!SPIFFS.begin()){
   Serial.println("An Error has occurred while mounting SPIFFS");
   return;
  }
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  
}
