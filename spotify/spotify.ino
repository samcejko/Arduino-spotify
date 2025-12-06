#include <WiFiS3.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>      

// --- NASTAVENÍ ---
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT / 4> display(GxEPD2_420_GDEY042T81(10, 9, 8, 7));
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
IPAddress server(192, 168, 1, 10); // IP adresa vaseho serveru/PC
const int port = 80;              
const char* path = "/spotify/api.php";

WiFiClient client;

// Proměnné
String g_track = "";
String g_artist = "";
String g_nextTrack = "";
String g_nextArtist = "";
int g_percent = 0;
bool isPaused = false;
int failCount = 0; // Počítadlo chyb, aby to neblikalo hned

const int BMP_SIZE = 5000; 
uint8_t bitmap[BMP_SIZE]; 

// LOGO
const unsigned char spotifyLogo [] PROGMEM = {
  0x00, 0x0f, 0xf0, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0xc0, 
  0x07, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xf8, 0x3f, 0xff, 0xff, 0xfc, 
  0x3f, 0xe0, 0xff, 0xfc, 0x7c, 0x00, 0x07, 0xfe, 0x78, 0x00, 0x00, 0x7e, 0x78, 0x04, 0x00, 0x1e, 
  0xff, 0xff, 0xfc, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0xfc, 0x00, 0x03, 0xff, 
  0xfc, 0x1e, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x7f, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0x80, 0x7f, 0xff, 
  0x7e, 0x00, 0x07, 0xfe, 0x7e, 0x7f, 0x83, 0xfe, 0x7f, 0xff, 0xf1, 0xfe, 0x3f, 0xff, 0xff, 0xfc, 
  0x3f, 0xff, 0xff, 0xfc, 0x1f, 0xff, 0xff, 0xf8, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xe0, 
  0x03, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0x80, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x0f, 0xf0, 0x00
};

void setup() {
  // Init displeje - jeden blik
  display.init(115200, true, 2, false); 
  display.setRotation(0); 
  display.setTextColor(GxEPD_BLACK);
  
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(184, 134, spotifyLogo, 32, 32, GxEPD_BLACK);
  } while (display.nextPage());

  // WiFi - bez timeoutu, prostě dokud se nepřipojí
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); 
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
     WiFi.begin(ssid, password);
     delay(500);
     return;
  }

  fetchData();
  delay(1500); // Pauza mezi dotazy
}

void fetchData() {
  if (!client.connect(server, port)) {
    handleFailure();
    return;
  }

  // HTTP 1.0 je klič! Zabrání chunked encoding bordelu.
  client.print(F("GET ")); client.print(path); client.print(F(" HTTP/1.0\r\n"));
  client.print(F("Host: 192.168.1.154\r\n")); 
  client.print(F("Connection: close\r\n\r\n"));

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) { 
      client.stop(); 
      handleFailure();
      return; 
    }
  }

  // Přeskočení hlaviček
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // Čtení dat
  String status = client.readStringUntil('\n'); status.trim();
  
  if (status == "1") {
    failCount = 0; // Úspěch, resetujeme chyby
    isPaused = false;

    String newTrack = client.readStringUntil('\n'); newTrack.trim();
    String newArtist = client.readStringUntil('\n'); newArtist.trim();
    String pStr = client.readStringUntil('\n'); pStr.trim();
    int newPercent = pStr.toInt();
    
    // Načteme Next Track
    String nTrack = client.readStringUntil('\n'); nTrack.trim();
    String nArtist = client.readStringUntil('\n'); nArtist.trim();

    // Načtení obrázku - pokud je nový track
    bool newImage = false;
    if (newTrack != g_track || g_track == "") {
       readHexImage(true); // Uložit
       newImage = true;
    } else {
       readHexImage(false); // Zahodit (jen přečíst stream)
    }

    // Update globálních proměnných
    bool changed = (newTrack != g_track) || (newPercent != g_percent);
    g_track = newTrack;
    g_artist = newArtist;
    g_percent = newPercent;
    g_nextTrack = nTrack;
    g_nextArtist = nArtist;

    // Kreslení
    if (changed || newImage) {
        drawScreen(newImage); // newImage = true znamená full refresh
    }

  } else if (status == "0") {
    // Server hlásí pauzu
    handleFailure(); // Použijeme logiku selhání (počítadlo), abychom neblikali hned
  } else {
    // Neznámý status (chyba přenosu)
    client.stop();
  }
}

void handleFailure() {
  failCount++;
  // Až po 5 chybách (cca 10-15s) přepneme na Paused obrazovku
  if (failCount > 5 && !isPaused) {
    isPaused = true;
    drawPaused();
  }
}

void readHexImage(bool save) {
  int index = 0;
  unsigned long timeout = millis();
  if (save) memset(bitmap, 0, BMP_SIZE);

  while (client.connected() && millis() - timeout < 4000) {
    if (client.available()) {
      char c1 = client.read();
      if (c1 <= 32) continue; // Ignoruj mezery
      
      while(!client.available() && millis() - timeout < 4000); // Čekej na druhý znak
      char c2 = client.read();
      
      if (save && index < BMP_SIZE) {
        bitmap[index++] = (hexCharToInt(c1) << 4) | hexCharToInt(c2);
      }
      timeout = millis(); // Reset timeoutu při aktivitě
      if (index >= BMP_SIZE && save) break; 
    }
  }
}

void drawScreen(bool fullRefresh) {
  display.init(115200, false, 2, false); // False = soft update
  if (fullRefresh) display.setFullWindow(); // Nový song = pročištění
  else display.setPartialWindow(0, 0, display.width(), display.height());

  display.firstPage();
  do {
    // 1. Vyčistit
    display.fillRect(10, 43, 204, 204, GxEPD_WHITE);  
    display.fillRect(225, 50, 175, 150, GxEPD_WHITE); 
    display.fillRect(225, 200, 175, 50, GxEPD_WHITE); 
    display.fillRect(0, 260, 400, 40, GxEPD_WHITE);   

    // 2. Obrázek
    display.drawRect(8, 43, 204, 204, GxEPD_BLACK);
    display.drawBitmap(10, 45, bitmap, 200, 200, GxEPD_BLACK);

    int textX = 225;
    int cursorY = 80;
    
    // 3. Track
    display.setFont(&FreeMonoBold12pt7b);
    printWrapped(g_track, textX, cursorY);

    // 4. Artist
    cursorY = 140; // Pevná pozice pro artistu
    display.setFont(&FreeMono12pt7b);
    printWrapped(g_artist, textX, cursorY);

    // 5. Bar
    int barY = 220;
    display.drawLine(textX, barY, textX + 165, barY, GxEPD_BLACK);
    int dotX = textX + ((165 * g_percent) / 100);
    display.fillCircle(dotX, barY, 5, GxEPD_BLACK);

    // 6. Header
    display.drawBitmap(10, 10, spotifyLogo, 32, 32, GxEPD_BLACK);

    // 7. Next
    if (g_nextTrack != "-" && g_nextTrack != "") {
       display.setFont(&FreeMono9pt7b);
       display.setCursor(10, 280);
       display.print("Next: ");
       display.print(g_nextTrack.substring(0, 25)); // Ořez
    }

  } while (display.nextPage());
}

// Pomocná fce pro zalomení textu
void printWrapped(String text, int x, int y) {
    if (text.length() > 12) {
        int split = text.lastIndexOf(' ', 12);
        if (split < 0) split = 12;
        
        display.setCursor(x, y);
        display.print(text.substring(0, split));
        display.setCursor(x, y + 25);
        display.print(text.substring(split + (text.charAt(split) == ' ' ? 1 : 0)));
    } else {
        display.setCursor(x, y);
        display.print(text);
    }
}

void drawPaused() {
  display.init(115200, false, 2, false);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(184, 100, spotifyLogo, 32, 32, GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(130, 160);
    display.print("Paused");
  } while (display.nextPage());
}

byte hexCharToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}