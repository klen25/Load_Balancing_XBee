#include <SoftwareSerial.h>
#include "DHT.h"
#include <stdlib.h>

SoftwareSerial xbee (3, 2); // Set Pin 3 dan 2 sebagai RX, TX untuk XBee

// ------------------- KOnfigurasi DH DL XBee
#define router1H      "0013A200"
#define router1L      "4107043D"
#define router2H      "0013A200"
#define router2L      "417CE257"
#define broadcastH    "0"
#define broadcastL    "FFFF"
//--------------------

#define pinDHT    A0 // Pin Sensor DHT
#define DHTType DHT22
DHT dht(pinDHT, DHTType); // Inisialisasi pin sensor DHT

// -- Konfigurasi ID Arduino XBee
namespace
{
  #define ED 1
  
  #if ED == 1
    #define ED 1
    #define antri 1000
  #elif ED == 2
    #define ED 2
    #define antri 2000
  #elif ED == 3
    #define ED 3
    #define antri 3000
  #endif
}

#define MAXDATA 40
#define MAXPAKET 100
#define tunggu 400

// TIMER
int MENIT, JAM, count, DETIK;
unsigned long previousMillis = 0;
const long interval = 1000;
int timeout,jeda,jedanow = 0;

// VARIABEL GLOBAL
int paketsend, datasend = 0;
String Router1 = "";
String Router2 = "";
int readData;
String incomingByte;
float Temp;

char buffTemp[5] = "0";
char DataKirim [MAXDATA];

String outTemp = "";

bool Route = false;
bool Status = false;
int value;
bool SerialOK = false;
unsigned int Rout;

template<typename T, size_t N> size_t ArraySize (T (&) [N]){return N; }

void setup()
{
  Serial.begin(9600);
  xbee.begin(9600);
  dht.begin();
  delay(2000);

  Serial.println("End Device Set Broadcast");
  SwitchRouter(broadcastH, broadcastL); delay(2000); // Set XBee ke mode Broadcast
  Status = false;
  timeout = 0;
  Serial.print("End Device ");
  Serial.print(ED);
  Serial.println(" Ready");
}

/*
 * Selama ON, End Device dalam kondisi standby.
 * Jika tidak ada request data, maka End Device akan diam.
 * Ketika ada request data dari Router, maka End Device akan mengirimkan data suhu dari
 * pembacaan sensor DHT ke Router.
 * Setelah End Device mengirim data ke Router, kemudian End Device akan kembali standby sampai
 * ada Router yang request data.
 * 
 * Format pengiriman dari End Device ke Router :
 * <Router# : End Device* : DATA_SUHU : PAKET_KE>
 * # : Router yang request data SUHU
 * * : End Device pengirim data SUHU
 */
void loop()
{
  if(SerialOK == true) // Jika serial sudah OK (diperbolehkan), maka End Device akan mengirim data.
  { SendData(); SerialOK = false; }
  
  if(xbee.available() > 2) // Jika Serial menerima inputan, maka akan disimpan dan dibaca untuk dieksekusi
  {
    incomingByte = "";
    incomingByte = xbee.readString();
    Status = true; timeout = 0;
    CekInput(); // Cek inputan yang masuk ke End Device
  }
}

/*
 * Fungsi untuk mengirim data dari End Device ke Router
 */
void SendData()
{
  if(paketsend == MAXPAKET) { paketsend = 0; } // Jika paket yang terkirim sudah 100 paket, maka counter akan kembali ke 0
  paketsend++;
  Temp = dht.readTemperature(); // Pembacaan sensor suhu DHT
  outTemp = dtostrf(Temp, 2, 2, buffTemp); // Merubah data sensor bertype float menjadi string
  sprintf(DataKirim, "<Router%d : End Device%d : %s : %d>", Rout, ED, outTemp.c_str(), paketsend);
  delay(200);
  xbee.print(DataKirim);
  String datanya = DataKirim; datasend =+ datanya.length();
  Serial.print(DataKirim); Serial.println();
  Serial.print("Panjang Byte: "); Serial.print(datasend-2);  Serial.println(" bytes");
}

/*
 * Fungsi untuk membaca input yang diterima End Device
 * Jika inputan terdapat kata "FULL", maka End Device akan menampilkan output bahwa Router tersebut telah full
 * Jika inputan terdapat kata "NEXT", maka End Device akan melihat apakah yang di request ID End Device tersebut
 * atau bukan, jika iya maka End Device yang direquest akan mengirim data suhu, jika bukan maka End Device akan
 * standby
 */
void CekInput()
{
  if((incomingByte.indexOf("FULL") >= 0))
  {
    int pos = incomingByte.indexOf("FULL");
    Serial.print("Router "); Serial.print(pos-1); Serial.println(" FULL");
  }
  else if(incomingByte.indexOf("NEXT") >= 0)
  {
    if(int(incomingByte[3])-48 == ED) // Mencocokkan apakah yang direquest IDnya atau bukan
    { Status = true; SerialOK = true; Rout = int(incomingByte[5])-48; }
  }
  else
  { incomingByte = ""; }
  incomingByte = ""; timeout = 0;
}

void SwitchRouter(String addrSH, String addrSL)
{
  timeout = 0;
  int n = 0;
  String atcom = "";
  Serial.println("AT Mode");
  
  xbee.print("+++");
  while (xbee.available() == 0){
    timeout = timeout + 1; delay(100);
    if(timeout == 10) { xbee.print("+++"); }
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
    timeout=timeout+1; delay(100);
    if(timeout == 20) { xbee.println("ATDH" + String(addrSH));}
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
    timeout=timeout+1; delay(100);
    if(timeout == 2000) { xbee.println("ATDL" + String(addrSL)); }
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
    timeout=timeout+1; delay(100);
    if(timeout == 2000) { xbee.println("ATWR"); }
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
    timeout=timeout+1; delay(100);
    if(timeout == 2000) { xbee.println("ATCN"); }
  }
  while (xbee.available() > 0) {
    atcom = "";
    atcom = xbee.readString();
    Serial.print("ATCN ");  Serial.println(atcom); timeout = 0;
  }
}
