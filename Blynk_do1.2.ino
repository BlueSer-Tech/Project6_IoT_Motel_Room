#define BLYNK_TEMPLATE_ID "TMPL-62PdqqC"
#define BLYNK_TEMPLATE_NAME "activitypulse"
#define BLYNK_AUTH_TOKEN "YF01LoMvPP7LKvefbj2XNnl3XZ674RPT"

#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <time.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP_Mail_Client.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

String GOOGLE_SCRIPT_ID = "AKfycbzj1UTEK4upUquf14juQ03vqpBaCZDJa1DYQFugUfXCQzaBjSG1c4AKhpmsk0MpLI9dHg";

TaskHandle_t Task1;
BlynkTimer timer;

#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17);
#else
PZEM004Tv30 pzem(Serial2);
#endif  // Software Serial pin 8 (RX) & 9 (TX)

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Mastermind";
char password[] = "2001zone";

#define relay1_pin 13
#define relay2_pin 12
#define relay3_pin 4

int relay1_state = HIGH;
int relay2_state = HIGH;
int relay3_state = HIGH;

#define button1_vpin V0
#define button2_vpin V1
#define button3_vpin V2
#define button4_vpin V3
#define door_pin 15


/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "noud.mlp6@gmail.com"
#define AUTHOR_PASSWORD "eehn xmod rkzg kziv"

/* Recipient's email*/
#define RECIPIENT_EMAIL "Thanhhanote@gmail.com"
#define RECIPIENT_NAME "Pham Thanh Ha"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;
//-----------------------------------RFID
#define SS_PIN 5
#define RST_PIN 0
MFRC522 mfrc522(SS_PIN, RST_PIN);
String UID_Tag = "";
//------------------------------------Keypad
LiquidCrystal_I2C lcd1(0x3F, 20, 4);
LiquidCrystal_I2C lcd2(0x26, 16, 2);

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 14, 27, 26, 25 };  // connect to the row pinouts of the keypad
byte colPins[COLS] = { 33, 32, 2 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String password_1 = "123456";        // change your password here
const String password_2 = "111111";  // change your password here
const String password_3 = "000000";  // change your password here
String inputPass = "";
String hiddenPass = "";
int Pass_lenght = 5;
int input = 6;
int temp = 0;
char SetTime[10] = "06:00:00";
char timeStringBuff[11];  //50 chars should be enough
char time_hStringBuff[10];
void Random_Pass();
void Door_Lock();
void Tinhtien();
void send_notification(float ene, float cost, int lev);
void energy_level();
void check_level();
int level;
int temp1;
float voltage;
float current;
float power;
float frequency;
float pf;
float energy;
float money;
//-----------------------------------------------


BLYNK_CONNECTED() {
  Blynk.syncVirtual(button1_vpin);
  Blynk.syncVirtual(button2_vpin);
  Blynk.syncVirtual(button3_vpin);
  Blynk.syncVirtual(button4_vpin);
}

BLYNK_WRITE(button1_vpin) {
  relay1_state = param.asInt();
  digitalWrite(relay1_pin, relay1_state);
}

BLYNK_WRITE(button2_vpin) {
  relay2_state = param.asInt();
  digitalWrite(relay2_pin, relay2_state);
}

BLYNK_WRITE(button3_vpin) {
  relay3_state = param.asInt();
  digitalWrite(relay3_pin, relay3_state);
}

void RFID_Read() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID của thẻ: ");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  UID_Tag = content.substring(1);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  Serial.println("");
}
void google_sheets(float vol, float curr, float fre, float p, float power_factor, float ene, float total_cost) {
  if (WiFi.status() == WL_CONNECTED) {
    static bool flag = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y", &timeinfo);
    strftime(time_hStringBuff, sizeof(time_hStringBuff), "%T", &timeinfo);
    Serial.print("Time:");
    Serial.println(String(time_hStringBuff));
    String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "time=" + time_hStringBuff + "&date=" + timeStringBuff + "&voltage=" + String(vol) + "&current=" + String(curr)
                      + "&frequency=" + String(fre) + "&power=" + String(p) + "&power_factor=" + String(power_factor) + "&energy_now=" + String(ene) + "&total_cost=" + String(total_cost);
    //Serial.print("POST data to spreadsheet:");
    //Serial.println(urlFinal);
    HTTPClient http;
    http.begin(urlFinal.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    //---------------------------------------------------------------------
    //getting response from google sheet
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      //Serial.println("Payload: "+payload);
    }
    //---------------------------------------------------------------------
    http.end();
  }
}

void send_notification(float ene, float cost, int lev) {
  smtp.debug(1);
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  /* Declare the message class */
  SMTP_Message message;
  message.sender.name = "ESP 32";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Warning Energy used";
  message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
  //Send HTML message
  String htmlMsg = "<div style=\"color:#000000;\">";
  htmlMsg += "<p>Điện năng tiêu thụ: ";
  htmlMsg += ene;
  htmlMsg += "</p>";

  htmlMsg += "<p>Tiền điện cần thanh toán: ";
  htmlMsg += cost;
  htmlMsg += "</p>";

  htmlMsg += "<p>Mức tiêu thụ: ";
  htmlMsg += lev;
  htmlMsg += "</p>";

  htmlMsg += "</div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  /* //Send simple text message
  String textMsg = "How are you doing";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/
  if (!smtp.connect(&session))
    return;
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void Tinhtien() {
  if (energy <= 50 && energy >= 0)
    money = energy * 1728;
  else if (energy <= 100)
    money = 86400 + (energy - 50) * 1786;
  else if (energy <= 200)
    money = 175700 + (energy - 100) * 2074;
  else if (energy <= 300)
    money = 383100 + (energy - 200) * 2612;
  else if (energy <= 400)
    money = 644300 + (energy - 300) * 2919;
  else
    money = 936200 + (energy - 400) * 3015;
}
void energy_level() {
  if (energy <= 50)
    level = 1;
  else if (energy <= 100) {
    level = 2;
  } else if (energy <= 200) {
    level = 3;
  } else if (energy <= 300) {
    level = 4;
  } else if (energy <= 400) {
    level = 5;
  } else {
    level = 6;
  }
}
void check_level() {
  if (level != temp1)
    send_notification(energy, money, level);
    temp1 = level;
}


void Random_Pass() {
  if (strncmp(SetTime, time_hStringBuff, 5) == 0 && temp == 0)  // compare first 2 character
  {
    password_1 = random(100000, 999999);
    send_email();
    temp = 1;
    Serial.println(" ");
    Serial.println(password_1);
  } else if (strncmp(SetTime, time_hStringBuff, 9) == 0) {
    temp = 0;
  }
}

void Door_Lock() {
  
  lcd2.setCursor(1, 0);
  lcd2.print("Enter Password");
  lcd2.setCursor(0, 1);
  lcd2.print("Pass:");
  //Random_Pass();
  char key = keypad.getKey();

  if (key) {
    if (input != 0) {
      if (key == '*') {

        Serial.println("");
        Serial.println("Reset Password");
        lcd2.setCursor(0, 0);
        lcd2.print("Reset Password");
        delay(3000);
        inputPass = "";  // reset the input password
        hiddenPass = "";
        Pass_lenght = 0;
        lcd2.clear();
        lcd2.setCursor(6, 1);
        lcd2.print("         ");
        Serial.print(key);
      } else if (key == '#') {
        if (inputPass == password_1 || inputPass == password_2 || inputPass == password_3) {
          Serial.println("The password is correct, unlocking the door");
          lcd2.setCursor(0, 0);  // Set the cursor at the 4th column and 1st row
          lcd2.print("Correct Password");
          lcd2.setCursor(0, 1);  // Set the cursor at the 4th column and 1st row
          lcd2.print(" Door Opened!");
          delay(1000);
          
          digitalWrite(door_pin, HIGH);
          delay(5000);
          digitalWrite(door_pin, LOW);
          lcd2.clear();
          
        } else {
          Serial.println("The password is incorrect, try again");
          input--;
          if (input != 0) {
            //Keypad_status=0;
            Serial.println("The password is incorrect, try again !");
            Serial.print("WARNING: ");
            Serial.print(input);
            Serial.println(" times left until system lock !");
            lcd2.clear();
            lcd2.setCursor(2, 0);  // Set the cursor at the 4th column and 1st row
            lcd2.print("Wrong Password");
            lcd2.setCursor(3, 1);  // Set the cursor at the 4th column and 1st row
            lcd2.print("Try Again!");
            delay(1000);
            lcd2.clear();
          } else {
            Serial.println("System Locked");
            lcd2.clear();
            lcd2.setCursor(1, 1);  // Set the cursor at the 4th column and 1st row
            lcd2.print("System Locked!");
            lcd2.clear();
          }
        }

        inputPass = "";  // reset the input password
        hiddenPass = "";
        Pass_lenght = 0;

      } else {
        if (Pass_lenght < 6) {
          Serial.print(key);
          Pass_lenght += 1;
          inputPass += key;
          hiddenPass += '*';
          lcd2.setCursor(6, 1);    // Set the cursor at the 4th column and 1st row
          lcd2.print(hiddenPass);  // append new character to input password string
        }
      }
    } else {
      Serial.println("System Locked");
      lcd2.clear();
      lcd2.setCursor(2, 1);  // Set the cursor at the 4th column and 1st row
      lcd2.print("System Locked!");
      lcd2.clear();
    }
  }
}

void send_email() {
  smtp.debug(1);
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */

  SMTP_Message message;
  message.sender.name = "ESP 32";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "KeyPassword to open door";
  message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);

  //Send HTML message

  String htmlMsg = "<div style=\"color:#000000;\">";
  htmlMsg += "<p>Password has changed at : ";
  htmlMsg += timeStringBuff;
  htmlMsg += " ";
  htmlMsg += time_hStringBuff;
  htmlMsg += "<br>";
  htmlMsg += "Password: ";
  htmlMsg += password_1;
  htmlMsg += "</p>";
  htmlMsg += "</div>";

  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session))
    return;
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}



void sendSensor() {

  voltage = pzem.voltage();
  if (voltage != NAN) {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    lcd1.setCursor(0, 0);
    lcd1.print("Voltage: ");
    lcd1.print(voltage);
    lcd1.print("V");
  } else {
    Serial.println("Error reading voltage");
  }


  current = pzem.current();
  if (current != NAN) {
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    lcd1.setCursor(0, 1);
    lcd1.print("Current: ");
    lcd1.print(current);
    lcd1.print("A");
  } else {
    Serial.println("Error reading current");
  }


  power = pzem.power();
  if (current != NAN) {
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    lcd1.setCursor(0, 2);
    lcd1.print("Power: ");
    lcd1.print(power);
    lcd1.print("W");
  } else {
    Serial.println("Error reading power");
  }

  energy = pzem.energy();
  if (current != NAN) {
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    lcd1.setCursor(0, 3);
    lcd1.print("Energy: ");
    lcd1.print(energy);
    lcd1.print("kWh");

  } else {
    Serial.println("Error reading energy");
  }

  frequency = pzem.frequency();
  if (current != NAN) {
    Serial.print("frequency: ");
    Serial.print(frequency, 3);
    Serial.println("Hz");

  } else {
    Serial.println("Error reading energy");
  }

  pf = pzem.pf();
  if (current != NAN) {
    Serial.print("pf: ");
    Serial.print(pf, 3);
    Serial.println("");
  } else {
    Serial.println("Error reading energy");
  }

  

  // if (energy <= 50) {
  //   // sum = energy * 1728;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }
  // if (energy > 50 && energy <= 100) {
  //   // sum = energy * 1786;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }
  // if (energy > 100 && energy <= 200) {
  //   // sum = energy * 2074;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }
  // if (energy > 200 && energy <= 300) {
  //   // sum = energy * 2612;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }
  // if (energy > 300 && energy <= 400) {
  //   // sum = energy * 2919;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }
  // if (energy > 400) {
  //   // sum = energy * 3015;
  //   // Serial.print("sum: ");
  //   // Serial.println(sum);
  //   // Serial.println();
  // }

  Blynk.virtualWrite(V4, voltage);
  Blynk.virtualWrite(V5, current);
  Blynk.virtualWrite(V6, power);
  Blynk.virtualWrite(V7, energy);
  Blynk.virtualWrite(V8, money);

  google_sheets(voltage, current, power, frequency, pf, energy, money);
}
void setup() {
  xTaskCreatePinnedToCore(
    Task1code, /* Function to implement the task */
    "Task1",   /* Name of the task */
    10000,     /* Stack size in words */
    NULL,      /* Task input parameter */
    1,         /* Priority of the task */
    &Task1,    /* Task handle. */
    0);        /* Core where the task should run */
  delay(500);
  Serial.begin(115200);
  lcd1.init();       // initializes the lcd
  lcd1.backlight();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(1000L, sendSensor);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  SPI.begin();
  delay(1000);
  mfrc522.PCD_Init();
  // Wire.begin(4, 5);

  pinMode(door_pin, OUTPUT);
  pinMode(relay1_pin, OUTPUT);
  pinMode(relay2_pin, OUTPUT);
  pinMode(relay3_pin, OUTPUT);

  digitalWrite(relay1_pin, LOW);
  digitalWrite(relay2_pin, LOW);
  digitalWrite(relay3_pin, LOW);
  digitalWrite(door_pin, LOW);

  Blynk.virtualWrite(button1_vpin, relay1_state);
  Blynk.virtualWrite(button2_vpin, relay2_state);
  Blynk.virtualWrite(button3_vpin, relay3_state);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  MailClient.networkReconnect(true);
  energy_level();
  temp1 = level;
}
void rfid_open_door()
{
    if (UID_Tag == "84 DE E6 BB" || UID_Tag == "CF 68 30 40") {
    lcd2.clear();
  // Set the cursor at the 4th column and 1st row
    lcd2.setCursor(0, 0);  // Set the cursor at the 4th column and 1st row
    lcd2.print("   Door Opened!");
    delay(1000);
    digitalWrite(door_pin, HIGH);
    delay(5000);
    digitalWrite(door_pin, LOW);
    lcd2.clear();
    UID_Tag = "";
  }
}
void Task1code(void* parameter) {
  lcd2.init();       // initializes the lcd
  lcd2.backlight();  // turns on the backlight

  lcd2.setCursor(0,0);
  lcd2.print("WelcomeToIoTRoom");
  delay(5000);
  lcd2.clear();
  for (;;) {

    RFID_Read();
    rfid_open_door();
    Door_Lock();
  }
}
void loop() {
  Blynk.run();
  timer.run();
  Tinhtien();
  energy_level();
  check_level();
  Random_Pass();
}