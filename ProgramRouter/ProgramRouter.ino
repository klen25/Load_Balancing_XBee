#include <SoftwareSerial.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

namespace
{
  #define maxpaket 10
  
  #if maxpaket == 10
    #define MAXLENGTH 360
    #define waittime 13000
  #elif maxpaket == 20
    #define MAXLENGTH 900
    #define waittime 12000
  #elif maxpaket == 30
    #define MAXLENGTH 1055
  #endif
}

#define Routerr 1
#define pinbuzz 4
boolean Balancing = true;

#define coordH      "0013A200"
#define coordL      "414F8F0F"
#define broadcastH  "0"
#define broadcastL  "FFFF"

#define msglength 40

SoftwareSerial xbee (3, 2);

//Variabel variabel
bool started, ended = false;
char incomingByte ;
char msg[msglength];
byte bitPosition;

String datamasuk;
int lengthdata, paket, pengiriman = 0;
int EndDev = 1;
String value, StatusRouter = "";
bool OKE, StatRouter;
unsigned long previoustimeout, currenttimeout, timeout = 0;
bool flagBoleh, full;
unsigned long previousMillis = 0;
int timer1_counter, repeat = 0;
unsigned int toggle, count, buzzms = 0;

void setup()
{
  delay(2000);
  Serial.begin(9600);
  xbee.begin(9600);
  
// Set pin buzzer
  pinMode(pinbuzz, OUTPUT);
  digitalWrite(pinbuzz, HIGH);
  delay(500);
  digitalWrite(pinbuzz, LOW);

  //  Serial.println("Router Set Broadcast");
  //  SwitchRouter(broadcastH, broadcastL); delay(2000);

  if (Balancing == 1)
  {
    Serial.println("Balancing Mode");
  }
  else
  {
    Serial.println("No Balancing Mode");
  }

  char addd[10];
  sprintf(addd, "Router %d Ready", Routerr);
  Serial.println(addd);

// Jika Router 2, maka Router 2 akan standby dulu karena prioritas utama adalah Router 1.
// Jika Router 1, maka akan langsung request data ke End Device 1, dst
  if (Routerr == 2)
  {
    OKE = true;
    flagBoleh = false;
  }
  else
  { waiting(40, 2); }
  delay(2000);
}

/*
 *                               --- Sistem Global ---
 * Setiap request data, Router akan melakukan pengulangan request maksimal 3x apabila End Device
 * yang di request tidak ada respon.
 * Setiap data yang diterima Router akan dihitung 1 paket. Jika masih belum mencapai maksimum load
 * (10 paket), maka Router akan request data lagi ke End Device berikutnya.
 * Setelah mencapai maksimum load, maka Router akan mengirimkan warning bahwa Router telah full,
 * dilanjutkan mengirimkan 10 paket data ke Coordinator. Dan Router lainnya akan ambil alih untuk
 * request data ke End Device.
 * 
 * Jika Router 1, maka Router 1 akan langsung request data Suhu ke End Device. Dimulai dari
 * End Device 1, 2, 3.
 * Jika Router 2, pada awalnya akan menunggu selama 5 detik, jika Router 1 tidak ada proses
 * request data, maka Router 2 akan mengambil alih dan melakukan request data ke End Device.
 * 
 * Format Router :
 * 1. Request data ke ED :  <ED#R*NEXT>
 * # : ED yang direquest
 * * : Router yang merequest
 * 
 * 2. Warning jika Router FULL : <R#FULL>
 * # : Router yang FULL
 * 
 * 3. Warning jika Router lain sudah boleh request data : <R#Switch>
 * # : Router yang full dan sudah kirim data ke Coordinator
 */
 
void loop()
{
  if (xbee.available() > 0)
  {
    flagBoleh = false;
    OKE = false;
    timeout = 0;
    incomingByte = "";
    incomingByte = xbee.read(); delay(1);
    ReadData();
  }

  if (started && ended)
  {
    value = String (msg);
    CekXBEE();
  }

  if (flagBoleh == true) // Dipebolehkan Request data ke End Device
  {
    ReqED();
  }
  if (flagBoleh == false)
  {
    value = "";
    waiting(120, 1);
  }
}

void CekXBEE()
{
  if (value.indexOf("FULL") > 0) // Jika Router menerima respon FULL dari Router lain, maka Router lain akan ambil alih request data
  {
    Serial.print("Router "); Serial.print(value[1]); Serial.println(" FULL");
    full = true;
    value = "";
    memset(msg, 0, sizeof(msg));
  }

  if (value.indexOf("NEXT") > 0) // Jika Router menerima respon NEXT dari Router lain, maka akan disimpan ID End Device yang sedang direquest
  {
    OKE = false;
    flagBoleh = false;
    EndDev = int(value[2]) - 48;
  }

  if (value.indexOf("Switch") > 0) // Jika Router menerima respon Switch dari Router lain, maka akan diambil alih proses request data
  {
    Serial.print("Router "); Serial.print(value[1]); Serial.println(" Done");
    Serial.print("Last ED= "); Serial.println(EndDev);
    value = "";
    memset(msg, 0, sizeof(msg));
    started = false;
    ended = false;
    full = true;
    flagBoleh = true;
    OKE = true;
  }
}

void wait(int i)
{
  unsigned long currentMillis = millis();
  i = i * 100;
  if (currentMillis - previousMillis >= i)
  {
    previousMillis = currentMillis;
    return;
  }
}

/*
 * Fungsi untuk menunggu apakah ada komunikasi antara End Device dengan Router lain.
 * Jika waktu tunggu sudah lebih, maka akan diambil alih untuk proses request datanya
 */
void waiting(int timee, const int i)
{
  timeout = 0;
  while (xbee.available() == 0)
  {
    timeout = timeout + 1; delay(100);
    if (timeout >= timee)
    {
      switch (i)
      {
        // Jika waktu tunggu untuk menunggu bahwa Router lain OFF, maka akan ada warning jika proses request data diambil alih
        case 1:
          for (int n = 1; n <= 3; n++)
          {
            digitalWrite(pinbuzz, HIGH);
            delay(1500);
            digitalWrite(pinbuzz, LOW);
            delay(500);          
          }

          Serial.print("R"); Serial.print(Routerr); Serial.println(" Take Over");
          value = "";
          memset(msg, 0, sizeof(msg));
          started = false;
          ended = false;
          flagBoleh = true;
          repeat = 0;
          TIMSK2 = 0x00;
          break;
        // Jika waktu tunggu untuk menunggu bahwa Router lain OFF (diawal Router menyala), maka akan ada warning jika proses request data diambil alih
        case 2:
          flagBoleh = true;
          break;
        // Jika waktu tunggu untuk menunggu bahwa tidak ada respon dari End Device yang direquest, maka akan dilanjutkan untuk request End Device selanjutnya
        case 3:
          Serial.print("ED "); Serial.print(EndDev); Serial.println(" No Respon");
          repeat++;
          break;
      }
      break;
    }
  }
  return;
}

void ReqED()
{
  if (paket <= maxpaket - 1) // Jika paket yang disimpan belum mencapai maxload, maka masih boleh request
  {
    flagBoleh = true;
    if (repeat >= 3)
    {
      EndDev += 1;
      if (EndDev <= 0) { 
        EndDev = 1;
      }
      repeat = 0;
    }
    if ((EndDev <= 0) or (EndDev > 3)) {
      EndDev = 1;
    }

    memset(msg, 0, sizeof(msg)); value = "";
    started = false;
    ended = false;

    delay(1000);
    char next[11];
    sprintf(next, "<ED%dR%dNEXT>", EndDev, Routerr);
    xbee.print(next);
    Serial.print("R"); Serial.print(Routerr); Serial.print(" Request ED "); Serial.println(EndDev);
    timeout = 0;
    waiting(50, 3);

    while (xbee.available() > 0)
    {
      incomingByte = '\0';
      incomingByte = xbee.read(); delay(1);
      ReadData();
      timeout = 0;
      OKE = true;
    }
    full = false;

    if (started && ended)
    {
      value = String (msg);
      Serial.print("Data Masuk = "); Serial.println(value);

      if (value.indexOf("NEXT") > 0)
      {
        repeat = 0;
        if (int(value[value.length() - 6]) - 48 != Routerr)
        {
          value.remove(0);
          if (Routerr != 1)
          {
            flagBoleh = false;
          }
          else
          {
            flagBoleh = true;
          }
        }
      }

      if (int(value[6]) - 48 == Routerr) // Pembacaan input jika sesuai dengan ID Router saat ini atau tidak
      {
        // Jika ID Router yang dikirim ED sesuai dengan Router saat ini, maka data akan disimpan sebagai 1 paket
        if ((value.indexOf("Router") >= 0) and (value.indexOf("Device") >= 0))
        {
          if (int(value[20]) - 48 == EndDev)
          {
            repeat = 4;
            paket++;
            datamasuk += value + "|";
            lengthdata += value.length();
            Serial.print("Panjang Bytes = "); Serial.print(value.length()); Serial.println(" bytes");
            Serial.print("Paket Ke = "); Serial.println(paket);
            Serial.print("Total Bytes = "); Serial.print(lengthdata); Serial.println(" bytes");
          }
          else
          {
            EndDev -= 1;
          }
        }
      }

      bitPosition = 0;
      msg[bitPosition] = '\0';
      started = false;
      ended = false;
    }
  }
  else if ((paket >= maxpaket - 1)) // Jika paket sudah mencapai maxload
  {
    flagBoleh = false;
    if (Balancing)
    {
      char full[8];
      sprintf(full, "<R%dFULL>", Routerr); // Router akan mengirim warning jika sudah full
      xbee.println(full); delay(10);
    }
    wait(10);
    SendData(); // Kirim paket data to Coordinator
    delay(6000);
    char switchh[14];
    sprintf(switchh, "<R%dSwitch>", Routerr); // Router mengirim warning bahwa Router lain boleh ambil alih proses request data
    xbee.print(switchh); delay(10);
    datamasuk = "";
    lengthdata = 0;
    paket = 0;
  }
}

/*
 * Fungsi untuk membaca dan menyimpan inputan yang masuk
 */
void ReadData()
{
  if (incomingByte == '<')
  {
    started = true;
    bitPosition = 0;
    msg[bitPosition] = '\0';
  }
  else if (incomingByte == '>')
  {
    ended = true;
  }
  else
  {
    if (bitPosition < msglength)
    {
      msg[bitPosition] = incomingByte;
      bitPosition++;
      msg[bitPosition] = '\0';
    }
  }
}

/*
 * Fungsi untuk kirim paket data ke Coordinator
 * Format :
 * (Co=R#|DATA_SEBANYAK_MAXLOAD)
 * # : Router pengirim
 */
void SendData()
{
  digitalWrite(pinbuzz, HIGH);
  delay(1500);
  digitalWrite(pinbuzz, LOW);
  pengiriman += paket;
  Serial.println(); Serial.println("SEND DATA TO COORDINATOR :");
  char datakirim[MAXLENGTH];
  sprintf(datakirim, "(Co=R%d|%s)", Routerr, datamasuk.c_str()); delay(500);
  char buffer[40] = {0};
  size_t len = strlen(datakirim);
  size_t blen = sizeof(buffer) - 1;
  size_t i = 0;

  // Pengiriman data ke Coordinator dibagi menjadi beberapa bagian, tiap bagian sepanjang 40bytes
  for (i == 0; i <= len / blen; ++i)
  {
    memcpy(buffer, datakirim + (i * blen), blen); // Membagi paket menjadi masing-masing 40bytes
    Serial.println(buffer); delay(600);
    xbee.print(buffer);
  }
  if (len % blen)
  {
    memcpy(buffer, datakirim + (len - len % blen), blen); // Cek jika masih ada sisa dari pembagian paket
  }
  Serial.print("Panjang Bytes = "); Serial.print(len); Serial.println(" bytes");
  Serial.print("Jumlah Paket = "); Serial.println(paket);
  Serial.print("Total Paket Terkirim Ke Coordinator = "); Serial.println(pengiriman);
}

void SwitchRouter(String addrSH, String addrSL)
{
  timeout = 0;
  int n = 0;
  String atcom = "";
  Serial.println("AT Mode");

  xbee.print("+++");
  while (xbee.available() == 0) {
    timeout = timeout + 1; delay(100);
    if (timeout == 10) {
      xbee.print("+++");
    }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString(); Serial.println("+++ ");
  }
  delay(300);
  timeout = 0;
  Serial.println("Switch Address " + addrSH + addrSL);
  xbee.println("ATDH" + String(addrSH));
  while (xbee.available() == 0) {
    timeout = timeout + 1; delay(100);
    if (timeout == 20) {
      xbee.println("ATDH" + String(addrSH));
    }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString();
    Serial.print("ATDH ");  Serial.println(atcom);
  }

  delay(300);
  timeout = 0;
  xbee.println("ATDL" + String(addrSL));
  while (xbee.available() == 0) {
    timeout = timeout + 1; delay(100);
    if (timeout == 2000) {
      xbee.println("ATDL" + String(addrSL));
    }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString();
    Serial.print("ATDL ");  Serial.println(atcom); timeout = 0;
  }

  delay(300);
  timeout = 0;
  xbee.println("ATWR");
  while (xbee.available() == 0) {
    timeout = timeout + 1; delay(100);
    if (timeout == 2000) {
      xbee.println("ATWR");
    }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString();
    Serial.print("ATWR ");  Serial.println(atcom); timeout = 0;
  }

  delay(300);
  timeout = 0;
  xbee.println("ATCN");
  while (xbee.available() == 0) {
    timeout = timeout + 1; delay(100);
    if (timeout == 2000) {
      xbee.println("ATCN");
    }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString();
    Serial.print("ATCN ");  Serial.println(atcom); timeout = 0;
  }
}
