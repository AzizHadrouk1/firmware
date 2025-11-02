#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

ESP8266WebServer server(80);
Adafruit_MCP23X17 mcp;

// --- Variables globales ---
String wifiSSID = "";
String wifiPASS = "";
String emailFrom = "";
String emailPass = "";
String emailTo   = "";
bool apMode = false;

// =========================
// 🔠 Base64 Helper
// =========================
String base64Encode(const String &plain) {
  const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  int val = 0, valb = -6;
  for (size_t i = 0; i < plain.length(); i++) {
    val = (val << 8) + (unsigned char)plain[i];
    valb += 8;
    while (valb >= 0) {
      out += base64_chars[(val >> valb) & 0x3F];
      valb -= 6;
    }
  }
  if (valb > -6) out += base64_chars[((val << 8) >> (valb + 8)) & 0x3F];
  while (out.length() % 4) out += '=';
  return out;
}

// =========================
// ✉️ Envoi Email
// =========================
void sendEmail(String subject, String body) {
  if (emailFrom == "" || emailPass == "" || emailTo == "") {
    Serial.println("⚠️ SMTP non configuré !");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("🔗 Connexion au serveur Gmail...");
  if (!client.connect("smtp.gmail.com", 465)) {
    Serial.println("❌ Échec de connexion SMTP !");
    return;
  }

  Serial.println("✅ Connecté au serveur Gmail SMTP.");
  client.println("EHLO esp8266.local");
  delay(100);

  client.println("AUTH LOGIN");
  delay(100);

  client.println(base64Encode(emailFrom));
  delay(100);
  client.println(base64Encode(emailPass));
  delay(100);

  client.println("MAIL FROM:<" + emailFrom + ">");
  client.println("RCPT TO:<" + emailTo + ">");
  client.println("DATA");
  client.println("From: <" + emailFrom + ">");
  client.println("To: <" + emailTo + ">");
  client.println("Subject: " + subject);
  client.println();
  client.println(body);
  client.println(".");
  client.println("QUIT");

  Serial.println("📧 Email envoyé : " + subject);
}

// =========================
// 💾 Sauvegarde & Lecture SPIFFS
// =========================
void saveWiFi(String ssid, String pass) {
  File f = SPIFFS.open("/wifi.txt", "w");
  if (f) {
    f.println(ssid);
    f.println(pass);
    f.close();
  }
}

void saveSMTP(String from, String pass, String to) {
  File f = SPIFFS.open("/smtp.txt", "w");
  if (f) {
    f.println(from);
    f.println(pass);
    f.println(to);
    f.close();
  }
}

void loadWiFi() {
  if (SPIFFS.exists("/wifi.txt")) {
    File f = SPIFFS.open("/wifi.txt", "r");
    if (f) {
      wifiSSID = f.readStringUntil('\n'); wifiSSID.trim();
      wifiPASS = f.readStringUntil('\n'); wifiPASS.trim();
      f.close();
    }
  }
}

void loadSMTP() {
  if (SPIFFS.exists("/smtp.txt")) {
    File f = SPIFFS.open("/smtp.txt", "r");
    if (f) {
      emailFrom = f.readStringUntil('\n'); emailFrom.trim();
      emailPass = f.readStringUntil('\n'); emailPass.trim();
      emailTo   = f.readStringUntil('\n'); emailTo.trim();
      f.close();
    }
  }
}

// =========================
// 🌐 Pages HTML
// =========================

// ---- Page principale ----
const char* mainPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP8266 Configuration</title>
<style>
  body { font-family: Arial; text-align:center; background-color:#eef2f3; }
  h1 { color:#333; }
  button {
    padding:10px 20px; border:none; border-radius:8px;
    font-size:16px; cursor:pointer; margin:10px;
  }
  .wifi { background-color:#2196F3; color:white; }
  .smtp { background-color:#9C27B0; color:white; }
  .quit { background-color:#E53935; color:white; }
</style>
</head>
<body>
<h1>⚙️ Panneau de configuration</h1>
<button class="wifi" onclick="location.href='/wifi'">Configurer Wi-Fi</button><br>
<button class="smtp" onclick="location.href='/smtp'">Configurer SMTP</button><br>
<button class="quit" onclick="fetch('/quit')">Quitter</button>
</body>
</html>
)rawliteral";

// ---- Page Wi-Fi ----
const char* wifiPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Configurer Wi-Fi</title>
<meta http-equiv="refresh" content="60;url=/" />
<style>
  body { font-family: Arial; text-align:center; background:#fafafa; }
  h1 { color:#2196F3; }
  form { background:white; padding:20px; border-radius:10px; display:inline-block; }
  input,button { margin:8px; padding:10px; border-radius:6px; }
  .back { background-color:#757575; color:white; }
  .save { background-color:#4CAF50; color:white; }
</style>
<script>
  setTimeout(() => { 
    document.body.innerHTML = "<h1>⏰ Temps écoulé !</h1><p>Rechargez la page pour recommencer.</p>"; 
  }, 60000);
</script>
</head>
<body>
<h1>Wi-Fi</h1>
<form action="/savewifi" method="POST">
  SSID: <input type="text" name="ssid"><br>
  Mot de passe: <input type="password" name="pass"><br>
  <input type="submit" class="save" value="✅ Sauvegarder">
</form><br>
<button class="back" onclick="location.href='/'">⬅ Retour</button>
</body>
</html>
)rawliteral";

// ---- Page SMTP ----
const char* smtpPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Configurer SMTP</title>
<meta http-equiv="refresh" content="60;url=/" />
<style>
  body { font-family: Arial; text-align:center; background:#fafafa; }
  h1 { color:#9C27B0; }
  form { background:white; padding:20px; border-radius:10px; display:inline-block; }
  input,button { margin:8px; padding:10px; border-radius:6px; }
  .back { background-color:#757575; color:white; }
  .save { background-color:#4CAF50; color:white; }
</style>
<script>
  setTimeout(() => { 
    document.body.innerHTML = "<h1>⏰ Temps écoulé !</h1><p>Rechargez la page pour recommencer.</p>"; 
  }, 60000);
</script>
</head>
<body>
<h1>SMTP</h1>
<form action="/savesmtp" method="POST">
  Expéditeur: <input type="text" name="from"><br>
  Mot de passe (App): <input type="password" name="smtppass"><br>
  Destinataire: <input type="text" name="to"><br>
  <input type="submit" class="save" value="✅ Sauvegarder">
</form><br>
<button class="back" onclick="location.href='/'">⬅ Retour</button>
</body>
</html>
)rawliteral";

// =========================
// 📡 Gestion Wi-Fi / AP
// =========================
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  Serial.println("📱 Nouveau périphérique connecté !");
  Serial.print("IP du périphérique : ");
  Serial.println(WiFi.softAPIP());
}

// =========================
// ⚙️ SETUP
// =========================
void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 Démarrage ESP8266...");

  // MCP23017
  if (!mcp.begin_I2C(0x20)) {
    Serial.println("❌ MCP23017 non détecté !");
    while (1);
  }
  for (int i = 0; i < 3; i++) mcp.pinMode(i, INPUT_PULLUP);

  // SPIFFS
  if (!SPIFFS.begin()) Serial.println("❌ Erreur SPIFFS !");

  loadWiFi();
  loadSMTP();

  if (wifiSSID != "" && wifiPASS != "") {
    WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
      delay(500); Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Connecté au Wi-Fi !");
      Serial.print("📡 IP: "); Serial.println(WiFi.localIP());
    } else apMode = true;
  } else apMode = true;

  if (apMode) {
    WiFi.softAP("ESP_Config");
    Serial.print("🌐 AP actif : "); Serial.println(WiFi.softAPIP());
    WiFi.onSoftAPModeStationConnected(&onStationConnected);

    // Pages
    server.on("/", HTTP_GET, []() { server.send(200, "text/html", mainPage); });
    server.on("/wifi", HTTP_GET, []() { server.send(200, "text/html", wifiPage); });
    server.on("/smtp", HTTP_GET, []() { server.send(200, "text/html", smtpPage); });
    server.on("/quit", HTTP_GET, []() {
      Serial.println("🚪 Un périphérique a quitté la tente (serveur web).");
      server.send(200, "text/plain", "Au revoir !");
    });
    server.on("/savewifi", HTTP_POST, []() {
      wifiSSID = server.arg("ssid"); wifiPASS = server.arg("pass");
      saveWiFi(wifiSSID, wifiPASS);
      server.send(200, "text/html", "<h1>✅ Wi-Fi sauvegardé. Redémarrage dans 15s...</h1><br><button onclick=\"location.href='/'\">⬅ Retour</button>");
      delay(15000); ESP.restart();
    });
    server.on("/savesmtp", HTTP_POST, []() {
      emailFrom = server.arg("from"); emailPass = server.arg("smtppass"); emailTo = server.arg("to");
      saveSMTP(emailFrom, emailPass, emailTo);
      server.send(200, "text/html", "<h1>✅ SMTP sauvegardé. Redémarrage dans 15s...</h1><br><button onclick=\"location.href='/'\">⬅ Retour</button>");
      delay(15000); ESP.restart();
    });
    server.begin();
  }
}

// =========================
// 🔁 LOOP
// =========================
void loop() {
  if (apMode) server.handleClient();
  else {
    for (int i = 0; i < 3; i++) {
      int state = mcp.digitalRead(i);
      if (state == LOW) {
        String subject, body;
        switch (i) {
          case 0: subject="🚨 Maintenance"; body="Bouton Maintenance appuyé."; break;
          case 1: subject="🚨 Contrôle Qualité"; body="Bouton Contrôle appuyé."; break;
          case 2: subject="🚨 Ressources"; body="Bouton Ressources appuyé."; break;
        }
        sendEmail(subject, body);
        delay(5000);
      }
    }
    delay(300);
  }
}
