/*
 * PIN XBEE - ARDUINO MEGA
 *      5V  - 5V
 *     GND  - GND
 *      RX  - 19
 *      TX  - 18
 */

#include <stdio.h>
#include <string.h>

//Variables
String dataIn = "";
String dt[20] = "";
int lengthIn = 0;
bool started = false;
bool ended = false;
bool statuss = false;
bool databaru = false;
String incomestring;
int k, pengiriman = 0;
int i, j, m, n = 0;

void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);
  delay(2000);
  Serial.println("Coordinator Ready");
  statuss = true;
}

/*
 * Ketika Coordinator menerima input, Coordinator akan menyeleksi inputan yang boleh disimpan
 * dan yang tidak boleh disimpan.
 * Inputan yang tidak boleh disimpan seperti :
 * 1. Inputan yang diawali dengan tanda "<" dan diakhiri dengan ">"
 * 2. Inputan yang terdapat kata "NEXT"
 * 
 * Inputan yang boleh disimpan adalah inputan yang diawali dengan tanda "(" dan diakhiri dengan
 * tanda ")". Inputan ini adalah inputan yang dikirim oleh Router berupa paket-paket data
 * berjumlah 'MAXLOAD' dan kemudian akan diolah dan ditampilkan oleh Coordinator.
 * Jika Inputan diawali dengan "(" dan diakhiri dengan ")", maka selanjutnya Coordinator akan
 * memilah parsingan data dengan index karakter "|". Setelah Coordinator memilah paket-paket
 * data inputan, selanjutnya Coordinator akan menampilkan hasil parsingan ke Serial Monitor
 * beserta dengan panjang karakter (bytes) dan jumlah paket data.
 * 
 * Format data yang diterima Coordinator :
 * (Co=R#|DATA_YANG_DISIMPAN|), # : Router Pengirim
 */
void loop()
{
  if(Serial3.available()) // Jika XBee menerima Input
  {
    incomestring = Serial3.readString(); // Inputan disimpan di variabel incomestring
    
// ------------------------------------------------- Input yang tidak boleh disimpan
    if(incomestring.indexOf("<") >= 0 ) // Input yang diawali dengan "<"
    {
      incomestring.remove(0); // Menghapus inputan yang tidak disimpan.
      databaru = true;
    }
    if((incomestring.indexOf("NEXT") >= 0 ))
    {
      incomestring.remove(0); // Menghapus inputan yang tidak disimpan.
      databaru = false;
    }

// ------------------------------------------------- Input yang boleh disimpan
      if (incomestring[0] == '(')
      { dataIn =""; databaru = false; started = true; }
      if (incomestring[incomestring.length() - 1] == ')')
      { databaru = false; ended = true; }

      if (started) // Inputan diawali dengan "(", maka akan disimpan
      {
        dataIn += incomestring;
        if(ended) // Inputan diakhiri dengan ")", maka akan mulai parsing inputan
        {
          dataIn.remove(dataIn.length() - 2); // 2 karakter terakhir akan dihapus ("|" dan ")")
          parsingData();
        }
        if (databaru)
        {
          parsingData();
        }
      }
  }
  else
  { incomestring = ""; } 
}

/*
 * Proses parsing data diawali dengan menghapus 4 karakter awal dari inputan yang diterima,
 * yakni karakter "(", "C", "o", dan "=".
 * Kemudian memulai proses parsing data berdasarkan karakter "|"
 * Setelah selesai parsing sesuai panjang karakter inputan yang diterima, kemudian ditampilkan
 * di serial monitor beserta panjang inputan dan paket data yang diterima
 */
void parsingData()
{
// Variabel variabel untuk proses parsing inputan  
   i = 0;
   j = 0;
   m = 0;
   
  dataIn.remove(0, 4); // Menghapus 4 karakter awal, karena tidak dimasukkan dalam proses parsing
  Serial.print(F("Data: ")); Serial.println(dataIn);
    for(i = 3; i <= dataIn.length(); i++) // Melakukan pengecekan inputan untuk proses parsing
    {
      if (dataIn[i] == '|') // Jika karakter "|", maka karakter selanjutnya adalah parsingan baru
      { // Menyiapkan array baru untuk parsingan berikutnya
        j++;
        dt[j]="";
      }
      else // Jika bukan karakter "|", maka data disimpan dalam parsingan yang sama
      {
        dt[j] = dt[j] + dataIn[i]; // Karakter disimpan dalam parsingan yang sama
      }
    }
    
    char Output[100]; // Array untuk menampilkan inputan tiap parsingan 1 paket
    for(i = 0; i <= j; i++) // Menampilkan data hasil parsingan ke serial monitor
    {
      if (dt[i].length() >= 3) // Jika panjang data parsingan lebih dari 3 karakter maka boleh ditampilkan dan dihitung sebagai 1 paket data
      {
        sprintf(Output, "Routing %c | %s", dataIn[1], dt[i].c_str());
        Serial.println(Output); m = m + dt[i].length(); n++;
        delay(10);    
      }
   }
   lengthIn = lengthIn + m-1;
   k = k + n;
   Serial.print("Panjang Bytes = "); Serial.println(m-1);
   Serial.print("Paket = "); Serial.println(n);
   Serial.print("Total Bytes = "); Serial.println(lengthIn);
   Serial.print("Total Paket Diterima   = "); Serial.println(k);
   
   memset(dt, 0, sizeof(dt));
   memset(Output, 0, sizeof(Output));
   started = false;
   ended = false;
   databaru = false;
   dataIn = "";
   i = 0;
   j = 0;
   m = 0;
   n = 0;
}
