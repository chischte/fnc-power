# fnc-power

0–10-V-Lichtsteuerung für **KinCony DM4 (ESP32-S3, N16R8)**, gebaut mit PlatformIO.

## Konfiguration

Die Einstellungen stehen zentral in [`UserConfig.h`](UserConfig.h) im Projektstamm:

```cpp
constexpr int brightnessPercent = 35;
constexpr int serviceBrightnessPercent = 80;
constexpr int maxBrightnessPercent = 100;
constexpr int onHour = 20;
constexpr int onMinute = 0;
constexpr int offHour = 8;
constexpr int offMinute = 0;
```

`brightnessPercent` ist der gewünschte Automatikwert, `serviceBrightnessPercent` die Servicehelligkeit; `maxBrightnessPercent` begrenzt
alle Betriebsarten, einschliesslich Servicelicht und Taster-Rückmeldungen.
Alle Helligkeitsangaben sind absolute Prozentwerte des 0–10-V-Bereichs: 17 % bleibt bei einem
Maximum von 70 % weiterhin 17 %. Ein Sollwert über dem Maximum wird auf das Maximum begrenzt.
0 % als Maximum hält den Ausgang immer auf 0 V.

Am bisherigen CONTROLLINO-Aufbau wurden bei 17 % mindestens 44 lux in der Box gemessen, bei 50 % mindestens 800 lux.
Bei 40 % wurden 510 lux gemessen, bei 30 % wurden 260 lux gemessen.
Bei 35 % wurden mindestens 369 lux gemessen. Die Automatik bleibt deshalb auf 35 %
(nominal ca. 3,50 V am DM4); das Ziel von mindestens 300 lux wurde am bisherigen Aufbau erreicht.
Die aufsteigend sortierte Messtabelle steht direkt in `UserConfig.h`.

Zeiten sind lokale RTC-Zeiten mit Minutenauflösung. Zeitfenster tagsüber und über Mitternacht
werden unterstützt; gleiche Ein-/Ausschaltzeiten deaktivieren die Automatik (Service bleibt verfügbar).
Ungültige Prozentwerte, Stunden oder Minuten erzeugen beim Build eine Fehlermeldung.
Die automatischen Übergänge dauern weiterhin 60 Sekunden.

Nach Änderungen per USB oder OTA neu bauen und hochladen (siehe unten).
Die Werte sind Build-Defaults. Die Automatikhelligkeit kann der Viewer dauerhaft im NVS ueberschreiben und auf den Build-Default zuruecksetzen.
Die folgenden Verhaltensangaben beschreiben die Standardkonfiguration.

## Verhalten

| Aktion / Uhrzeit | Ausgang |
| --- | --- |
| Ab 20:00:00 | In 60 Sekunden von 0 auf 35 %, danach 35 % (ca. 3,50 V) |
| Ab 08:00:00 | In 60 Sekunden von 35 auf 0 %, danach Aus (0 V) |
| Kurzer Tastendruck, beim Loslassen | In 5 s vom aktuellen Wert auf 100 %, nach 10 Minuten zurück zum Zeitplan |
| Weiterer kurzer Druck im Servicebetrieb | Beendet das Servicelicht sofort; zurück zum aktuellen Automatikwert |
| Langer Druck (1,25 Sekunden) | Dauerhaft aus; sofort auf Maximum, dann in 5 s auf 0 % |
| Erneuter langer Druck (0,75 Sekunden) | Automatik; in 5 s von 0 auf Maximum, dreimal kurz blinken, dann in 0,4 s zum aktuellen Automatikwert |

Die 10 Minuten Servicezeit beginnen mit dem kurzen Tastendruck und enthalten die 5-Sekunden-Rampe.
Erneuter kurzer Druck bricht auch eine laufende Service-Rampe ab; Rückkehr und Zeitablauf erfolgen ohne Rampe.

Die automatischen Rampen laufen auch während Servicebetrieb, dauerhaft Aus und Taster-Rückmeldungen
im Hintergrund weiter. Nach einer Übersteuerung gilt der aktuelle Wert der Zeitplanrampe.
Die lange Rückmelderampe dauert 5 Sekunden; die kurzen Übergänge davor bzw. danach dauern
0,4 Sekunden. Bereits erreichte Startwerte benötigen keinen vorbereitenden Übergang.
Beim Aktivieren wird das Rampenmaximum zunächst 300 ms gehalten; danach folgen drei Blinkimpulse
mit jeweils 300 ms Aus und 300 ms auf Maximalhelligkeit (standardmässig 100 %).
Beim Deaktivieren gibt es keine Blinkimpulse.
Ein erneuter langer Druck während der Rückmeldung startet vom zuletzt ausgegebenen Wert;
beim Aktivieren wird der neue Rampenstart bei Bedarf in 0,4 Sekunden erreicht,
beim Deaktivieren wird sofort auf Maximum geschaltet. Longpress: 0,75 Sekunden zum Aktivieren und 1,25 Sekunden zum Deaktivieren der Automatik.
Die Haltezeit beginnt nach der 30-ms-Entprellung; langes Halten schaltet nur einmal pro Tastendruck.
Beim Start oder nach Wiederherstellung einer gültigen RTC gilt sofort der aktuelle Tag-/Nacht-Sollwert.
Ein Uhrstellen über eine Schaltgrenze startet ebenfalls eine 60-Sekunden-Rampe vom aktuellen Automatikwert.
Der analoge Ausgang und die Rampenberechnung haben 12 Bit Auflösung (0–4095): zwischen 0 und 35 %
erfolgt die Rampe in 1433 kleinen Stufen, durchschnittlich etwa alle 42 ms. Rechnerische Schrittweite:
ca. 2,44 mV; die absolute Genauigkeit hängt von der Ausgangsstufe ab.

Langes Halten löst genau einmal aus, beim Loslassen folgt kein kurzer Druck.
Der Taster wird mit 30 ms entprellt. Dimmrampen und Servicebetrieb blockieren die Hauptschleife nicht.
Das Servicelicht funktioniert auch bei ausgeschalteter Automatik und ohne gueltige RTC.
Ein kurzer Druck bricht eine laufende Modus-Rueckmeldung ab und startet das Servicelicht.
Der gespeicherte Automatikmodus bleibt unveraendert. Ist die Automatik aus, geht das Licht
nach erneutem kurzem Druck oder nach 10 Minuten wieder direkt auf 0 %.
Ein langer Druck beendet den Servicebetrieb. Dauerhaft Aus bleibt im internen NVS-Flash über Neustarts erhalten;
ein Gerät ohne gespeicherten Modus startet in Automatik. Servicebetrieb und Rampen werden nach Neustart nicht fortgesetzt.

## Anschlüsse

[Anschlussplan mit Klemmentabelle](docs/wiring.md)

![Anschlussplan](docs/wiring.svg)

- DM4 mit **12–24 V DC** versorgen; das bisherige 24-V-Netzteil kann verwendet werden.
- **Vierter digitaler Eingang IN4 → Schliessertaster → Eingangs-GND.** Keine externe Spannung
  an den Kontakt legen. Die bisherige +24-V-Leitung am Taster muss entfernt/umgeklemmt werden.
  Kein zusätzlicher Pull-up nötig. Nicht die analogen A1–A4 oder freie GPIOs verwenden.
- **Vierter 0–10-V-Ausgang (OUT4 / DAC D)** → Steuereingang des Dimmers;
  Ausgangs-GND → Signalmasse des Dimmers, soweit vom Dimmerhersteller zugelassen.
- Ausgänge 1–3 werden beim Firmwarestart auf 0 gesetzt und bleiben unbenutzt.
- Für LAN/OTA RJ45 mit dem lokalen Netzwerk verbinden. **Keine SD-Karte nötig.**

Der Dimmer muss bei 0 V tatsächlich abschalten. Ein reiner 1–10-V-Stromsenkenanschluss ist nicht
automatisch mit dem aktiven Spannungsausgang kompatibel. Die Luxwerte oben wurden am bisherigen
CONTROLLINO-Aufbau gemessen; am DM4 Spannung und Beleuchtungsstärke erneut prüfen.
Die Firmware setzt die DAC-Referenz intern / Verstärkung 1× entsprechend KinConys ESPHome-Beispiel.

## Netzwerk vorbereiten

`network.example.json` nach **`network.local.json`** kopieren. Die lokale Datei ist von Git ausgeschlossen.
Ein bereits vorhandenes `network.local.json` behalten; bei der Portierung wurde lokal eines mit
LAN und einem zufälligen OTA-Passwort angelegt.

- `ethernet`: standardmässig `true`, IP-Adresse automatisch über DHCP.
- `hostname`: standardmässig `fnc-power`.
- `wifi_ssid` und `wifi_password`: optionales 2,4-GHz-WLAN, sofern keine Zugangsdaten-Headerdatei vorhanden ist.
- `ota_password`: ein eigenes Passwort mit mindestens 12 Zeichen verwenden. Leer deaktiviert OTA.

Alternativ WLAN-Daten in `wifi-credentials.h` mit den Makros `WIFI_SSID` und `WIFI_PASSWORD` ablegen.
Die Firmware verwendet zuerst diese Datei im Projektstamm, danach `../wifi-credentials.h` und zuletzt
die WLAN-Felder der JSON-Konfiguration. Hier wird die bestehende Datei
`C:\Users\michi\git\wifi-credentials.h` verwendet, ohne sie ins Projekt zu kopieren.
Die Vorlage [wifi-credentials.example.h](wifi-credentials.example.h) enthält keine Zugangsdaten.
`wifi-credentials.h` im Projekt ist ebenfalls von Git ausgeschlossen; nur die Vorlage gehört in Git.

Ohne lokale JSON-Datei baut die Firmware mit den Beispieldaten: LAN aktiv, OTA deaktiviert.
Eine vorhandene WLAN-Headerdatei wird dabei weiterhin verwendet.
Netzwerkdaten werden beim Build eingebettet; Änderungen erfordern einen neuen Upload.
Keine Zugangsdaten oder Firmwaredateien veröffentlichen. OTA ist für ein vertrauenswürdiges lokales
Netzwerk vorgesehen, ohne Internet-Portfreigabe; die Übertragung ist nicht TLS-verschlüsselt.

Die Steuerung wartet nicht auf eine Netzwerkverbindung. Ein fehlendes LAN/WLAN beeinflusst Zeitplan
und Taster nicht. `LAN IP ...`, `WiFi IP ...` und `OTA ready` erscheinen im seriellen Monitor.
Das OLED wird in dieser Firmware noch nicht verwendet; die IP ist auch im DHCP-/Routermenü sichtbar.
Tuya, Cloud, Home Assistant, SD-Karte und Analogeingänge werden nicht verwendet.

## Bauen und erster USB-Upload

PlatformIO in VS Code installieren. Die festgelegte pioarduino-Plattform verwendet Arduino-ESP32 3.3.12
für den W5500-Ethernettreiber. Git muss im Prozesspfad verfügbar sein. Der erste Build lädt die
ESP32-Toolchain herunter. Das Board hat 16 MB Flash / 8 MB PSRAM; die Partitionstabelle enthält
zwei Firmwareplätze von je 6 MiB für OTA und einen separaten NVS-Bereich.

```powershell
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
pio run -e kincony_dm4
pio device list
pio run -e kincony_dm4 -t upload --upload-port COM5
pio device monitor -e kincony_dm4 --port COM5
```

`COM5` durch den tatsächlichen Port ersetzen. Alternativ die PlatformIO-Tasks in VS Code verwenden.
Die UTF-8-Einstellungen verhindern unter Windows Zeichensatzfehler bei der Upload-Fortschrittsanzeige.
Falls `pio` im Terminal fehlt: `& "$env:USERPROFILE/.platformio/penv/Scripts/platformio.exe"` statt `pio`.
USB-C mit einem Datenkabel anschliessen und den DM4 extern versorgen. Falls die automatische
Bootloader-Umschaltung nicht funktioniert: ESP32-Download-Taste halten, ESP32-Reset kurz drücken,
Download loslassen und erneut hochladen. Nicht die Tuya-Taste verwenden.
Die Firmware verwendet natives USB-CDC für den seriellen Monitor; der COM-Port kann nach dem
Erstupload wechseln. UART0 auf GPIO43/44 wird nicht verwendet, da diese Pins zum Ethernet gehören.

Der erste Upload muss per USB erfolgen, damit unsere Firmware und OTA-Partitionierung installiert werden.
Er ersetzt die KinCony-Werksfirmware. Ein Build allein führt keinen Upload aus.
Vor dem ersten Funktionstest den Dimmer noch nicht anschliessen: die vorhandene DAC-EEPROM-Konfiguration
kann vor Start unserer Firmware einen alten Ausgangswert herstellen. Unsere Firmware schreibt laufende
Dimmwerte ausschliesslich in die flüchtigen DAC-Register und setzt beim Start alle Ausgänge auf 0.
Ein garantiert glitchfreier Ausgang während Einschalten/Reset ist damit nicht zugesichert.

## OTA-Uploads

Nach dem ersten USB-Upload die ausgegebene IP-Adresse verwenden:

```powershell
pio run -e kincony_dm4_ota -t upload --upload-port 192.168.1.50
```

Die Adresse ist ein Beispiel. Der Upload übernimmt das Passwort automatisch aus `network.local.json`.
Der PC muss den DM4 erreichen können; ArduinoOTA verwendet UDP 3232 sowie eine TCP-Rückverbindung
zum Upload-PC. Bei einer Firewallabfrage den lokalen Zugriff für den Uploader erlauben.
Vorhandene eingehende Sperrregeln für PlatformIOs `python.exe` können den Rückkanal blockieren.
Dann kann der DM4 bereits `OTA started` melden und mit `ERR OTA 2` abbrechen, während der
Uploader einen Authentifizierungsfehler anzeigt. In diesem Fall die Firewallregeln prüfen.
Mit `pio run -e kincony_dm4_ota` lässt sich dieses Buildziel ohne Upload prüfen.

Während eines authentifizierten Updates wird Ausgang 4 auf 0 gesetzt; Taster und Rampen pausieren.
Nach Erfolg startet der DM4 neu und übernimmt den aktuellen Zeitplan bzw. den gespeicherten Aus-Modus.
Service und Rückmelderampen werden nach Neustart nicht fortgesetzt. Bei einem Übertragungsfehler nimmt
die bisherige Firmware die lokale Steuerung wieder auf. Ein DAC-Fehler beim Abschalten bricht das Update ab.
Eine unvollständig übertragene Firmware wird nicht als neue Startpartition aktiviert. Ein automatisches
Rollback bei einer vollständig übertragenen, aber fehlerhaften Anwendung ist nicht eingerichtet;
USB bleibt der Wiederherstellungsweg.

Für einen Passwortwechsel zunächst die neue Konfiguration **per USB** installieren; beim normalen
OTA-Upload müssen lokales Passwort und das Passwort der bereits laufenden Firmware übereinstimmen.
Die Flash-Partitionierung ebenfalls nur per USB ändern. Keine Browser-Uploadseite implementiert.

## Status im lokalen Netzwerk

`http://<Geraete-IP>/` zeigt die lokale RTC-Zeit, RTC-Status, Betriebsmodus,
Automatikfreigabe, Zeitfenster und den aktuellen Sollwert von OUT4 (12 Bit).
Die Seite aktualisiert sich alle fünf Sekunden. Die Spannung wird aus dem Sollwert
berechnet; sie ist keine Messung am Ausgang. Bei Schreibfehlern kann die tatsächliche
Ausgangsspannung vom Sollwert abweichen.

Die Seite und GET /api/status sind im lokalen Netzwerk ohne Anmeldung lesbar. Das Uhrstellen über das separate
Werkzeug benötigt das konfigurierte OTA-Passwort. Während eines OTA-Uploads ist die Seite nicht verfügbar.
`Zeitplan EIN` bei `Automatik freigegeben: Nein` bedeutet, dass der gespeicherte
Aus-Modus die Automatik übersteuert. Ohne gültige Zeit bleibt die Automatik dauerhaft
auf ihrer normalen Helligkeit (35 %). Die Seite zeigt RTC-Zustand, verwendete Zeitquelle
und ob die RTC im aktuellen Lauf bereits per NTP gestellt wurde.

## Uhr stellen

Die PC-Uhr lässt sich über WLAN oder LAN übertragen, ohne erneuten Firmware-Upload:

```powershell
python tools/set_clock_network.py 192.168.31.182
```

Die IP-Adresse anpassen. Das Werkzeug übernimmt das Passwort aus `network.local.json`,
sendet die lokale PC-Zeit an `POST /time` und wartet auf die bestätigte RTC-Schreibprüfung.
HTTP ist unverschlüsselt, einschliesslich des Passwort-Headers; nur im vertrauenswürdigen
lokalen Netzwerk verwenden. Eine gültige RTC wird durch NTP nicht überschrieben.
Alternativ funktioniert weiterhin das unten beschriebene USB-Verfahren.

Die DS3231 hält wie bisher **lokale Uhrzeit**. Beim Boot wird sie weder zurückgesetzt noch auf eine
Build-Zeit gestellt. Ist die RTC ungültig, fragt die Firmware über WLAN oder LAN automatisch
NTP ab (`ch.pool.ntp.org`, `pool.ntp.org`, `time.cloudflare.com`). Dafür braucht sie Zugang
zu einem Zeitserver; eine WLAN-Verbindung allein garantiert noch keine Zeitantwort.
Die Antwort wird in Schweizer Lokalzeit (CET/CEST) umgerechnet und in die RTC geschrieben.
Eine gültige RTC hat immer Vorrang und wird nicht regelmässig per NTP überschrieben.
Ohne Batterie erfolgt dies nach jedem vollständigen Stromausfall erneut. Mit Batterie
läuft die RTC über Stromausfälle hinweg weiter, ohne Konfigurationsänderung.

Ohne gültige Zeit bleibt die Automatik dauerhaft auf `brightnessPercent` (35 %, ca. 3,5 V),
auch wenn kein Netzwerk verfügbar ist. Sobald die Zeit wieder gültig ist, gilt sofort der
aktuelle Zeitplan. Bei WLAN-Ausfall läuft eine gültige RTC normal weiter. Falls das Schreiben
der RTC fehlschlägt, wird die bereits synchronisierte ESP32-Uhr verwendet und alle 30 Sekunden
ein neuer RTC-Schreibversuch unternommen. Servicelicht und bewusst gewähltes dauerhaft Aus
haben weiterhin Vorrang vor dem Automatik-Ersatzbetrieb.

I²C-Fehler, Oszillatorstopp, deaktivierter Batterieoszillator und ungültige
Kalenderwerte werden erkannt. Unterstützter Kalenderbereich: 2000–2099.

Nach dem ersten Upload den seriellen Monitor schliessen und die PC-Uhr übertragen:

```powershell
& "$env:USERPROFILE/.platformio/penv/Scripts/python.exe" tools/set_clock.py COM5
```

Alternativ mit 115200 Baud und Zeilenende LF im seriellen Monitor:

```text
TIME 2026-09-24 19:55:00
TIME?
```

Das Datum ist ein Beispiel; die tatsächliche lokale Zeit einsetzen. `OK TIME` bestätigt die Übernahme.
Eine geeignete RTC-Pufferbatterie nach Boardvorgabe einsetzen, wenn die Uhr Stromausfälle überstehen soll.
Die gültige RTC speichert weiterhin lokale Zeit ohne automatische Sommerzeitkorrektur.
Nach Sommer-/Winterzeitwechsel die RTC vom PC setzen; bei einer NTP-Ersteinrichtung wird
die dann gültige CET/CEST-Zeit berücksichtigt. Für OTA selbst ist keine gültige Uhr erforderlich.

## Aufbau und Tests

- `lib/Control/Control.h`: hardwareunabhängige Licht- und Tasterlogik mit 12-Bit-Helligkeit.
- `UserConfig.h`: Sollwerte, Maximum und Schaltzeiten.
- `src/Dm4Hardware.cpp`: MCP4728-DAC und PCF8574-Tastereingang.
- `src/I2cBus.cpp`: begrenzte I²C-Transaktionen mit Fehlerprüfung, 100 kHz / 20 ms Timeout.
- `src/RtcClock.cpp`, `include/RtcRegisters.h`: DS3231-Zugriff und Kalenderumrechnung.
- `include/ModeStore.h`: dauerhafter Modus im ESP32-NVS.
- `src/NetworkServices.cpp`: LAN, optionales WLAN und ArduinoOTA.
- `src/NetworkClock.cpp`: nicht wartende NTP-Ersteinrichtung bei ungültiger RTC, Zeitzone Schweiz.
- `src/StatusPage.cpp`: lokale Statusanzeige über HTTP mit begrenzter, nicht wartender Anfrageverarbeitung.
- `src/SerialClock.cpp`: begrenzter, nicht wartender serieller Befehlsparser.
- `src/main.cpp`: Zusammenführung der Module und Fehlerbehandlung.

```powershell
python tools/test_control.py
pio run -e kincony_dm4
pio run -e kincony_dm4_ota
```

Die Hosttests benötigen Python und LLVM/Clang oder GCC. Sie prüfen den tatsächlichen C++-Code:
Zeitplan, Service, Moduswechsel, Rampen, Entprellung, lange Tastendrücke, `millis()`-Überlauf,
12-Bit-Zwischenstufen sowie DS3231-Kalender- und 12-/24-Stunden-Dekodierung.
Firmware-Builds ersetzen keine Prüfung am angeschlossenen DM4.

Die Hardware wird bei fehlgeschlagener Initialisierung im Sekundentakt erneut angesprochen.
Bei einem Lesefehler des Tasters wird die Entprellung verworfen; erst ein gültig losgelassener
Taster schaltet die Bedienung wieder frei. Das gilt auch für einen beim Boot gehaltenen Taster.
DAC-Schreibfehler werden gemeldet und erneut versucht; bei Bus-/Hardwarefehlern kann der Ausgang
seinen letzten Spannungswert behalten. Netzwerkfehler unterbrechen die Lichtsteuerung nicht.

## Inbetriebnahme am Gerät

1. Ohne angeschlossenen Dimmer starten. `DM4 I/O OK` prüfen; Spannung an Ausgang 4 messen.
2. Uhr stellen, bei Tag ca. 0 V und bei Nacht ca. 3,50 V prüfen. Kurz drücken: in 5 s auf ca. 10 V.
3. Ausgänge 1–3 auf 0 V prüfen; erst dann den kompatiblen Dimmer anschliessen.
4. Uhr auf 07:59:55 und 19:59:55 stellen und beide 60-Sekunden-Rampen beobachten.
5. Service tags/nachts, beide langen Tastendrücke sowie Neustart bei dauerhaft Aus prüfen.
6. Mit Pufferbatterie Versorgung kurz trennen und Uhr nach Wiederkehr prüfen.
7. LAN trennen: lokale Bedienung muss weiterlaufen. LAN verbinden und IP/OTA prüfen.
8. OTA-Upload und anschliessenden Neustart testen; aktuelle Uhrzeit wieder setzen.

## Gesicherter CONTROLLINO-Stand

Git-Tag **`controllino-stable`**, Commit **`0a4dfc6`** (`Save stable Controllino firmware`).
Er enthält den unveränderten Stand vor der DM4-Portierung. Für eine spätere Rückportierung die dortige
Hardwareanbindung übernehmen und an die aktuelle Steuerlogik anpassen; der aktive Build unterstützt DM4.

## Hardwarequellen

- [KinCony DM4: Hardware und Versorgung](https://www.kincony.com/esp32s3-dimmer-module-dm4-4ch-dimmer.html)
- [DM4: Pins und I²C-Adressen](https://www.kincony.com/forum/showthread.php?tid=8993)
- [DM4: Eingänge, DAC-Referenz und LAN-Konfiguration](https://www.kincony.com/forum/showthread.php?tid=8994)
- [Microchip MCP4728: 12-Bit-DAC](https://www.microchip.com/en-us/product/mcp4728)
- [Analog Devices DS3231: Datenblatt](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf)

## Licht-Viewer und mehrere DM4

Start: `python tools/light_viewer.py` (Abhaengigkeit: `pip install -r tools/viewer-requirements.txt`).
Desktop-Verknuepfung mit Lampensymbol: `powershell -ExecutionPolicy Bypass -File tools/install_viewer_shortcut.ps1`.
Der Viewer findet aktualisierte DM4 per `_fnc-light._tcp.local.`; zwei feste Spalten zeigen je ein Geraet anhand seines `.local`-Namens. IP-Adressen erscheinen nur im Infotext. Die Suche wird alle 30 Sekunden wiederholt.
Er zeigt lokale Uhrzeit, Ein-/Ausschaltzeiten, Modus, Ausgangssollwert und Automatikhelligkeit.
Ein- und Ausschaltzeit lassen sich im Viewer im Format HH:MM aendern und werden gemeinsam dauerhaft im NVS gespeichert. UserConfig.h liefert die Startwerte. Gleiche Zeiten deaktivieren den Zeitplan.

Jedes neue Geraet bekommt `<hostname>-<MAC>.local`. Der Viewer kann den DNS-Namen dauerhaft
umbenennen (Kleinbuchstaben, Ziffern, Bindestriche, maximal 63 Zeichen); fuer jedes Geraet einen
anderen Namen verwenden. Die Umbenennung aktualisiert mDNS ohne Neustart. mDNS funktioniert im
lokalen Multicast-Netz; ueber getrennte VLANs ist ein mDNS-Reflector erforderlich.
OTA-Uploads deshalb mit `--upload-port <neuer-name>.local` oder der IP ausfuehren.

Die Automatikhelligkeit wird im NVS gespeichert, bleibt nach Neustart erhalten und ist auf das
konfigurierte Maximum begrenzt. Default setzt sie auf PowerConfig::brightnessPercent (35 %) zurueck.
Aenderungen waehrend der Einschaltzeit verwenden die 60-Sekunden-Rampe; tagsueber bleibt das Licht aus.
Servicemodus startet manuell fuer zehn Minuten und laesst sich vorzeitig beenden.
Schreibzugriffe verwenden das OTA-Passwort aus network.local.json; nur im vertrauten lokalen Netz.

API: GET /api/status liefert JSON. POST /api/control benoetigt einen leeren Body (Content-Length: 0),
X-OTA-Password, X-Command (name, brightness, reset, service) und X-Value (DNS-Name, Prozentzahl,
bzw. on/off). Das Passwort wird nicht im Viewer angezeigt.
Der zweite DM4 muss fuer den ersten Upload extern versorgt und per USB-Datenkabel verbunden sein.
Gleiche Firmware und Partitionierung koennen fuer beide DM4 verwendet werden.

Falls Windows Multicast-Antworten blockiert, sucht der Viewer beim Start zusaetzlich per
Unicast-mDNS im privaten /24-Netz der eingegebenen Startadresse. Die Startadresse des
bestehenden Geraets ist 192.168.31.182; bei einem Netzwechsel eine erreichbare Adresse eingeben.
Am 17.11.2026 wurde der erste DM4 per OTA aktualisiert und als fnc-licht-1.local eingerichtet.
OTA ueber diesen Namen, NVS-Namen nach Neustart, mDNS-Erkennung und Helligkeit/Default wurden geprueft.
Der zweite DM4 wartet noch auf externe Versorgung und den ersten USB-Upload.

Schaltzeiten-API: X-Command: schedule, X-Value: HH:MM,HH:MM (EIN,AUS).
Beide Zeiten werden als gemeinsamer NVS-Wert gespeichert. Aenderungen ueber eine
Schaltgrenze verwenden die normale 60-Sekunden-Rampe.

FNC Light zeigt fest zwei Geraetespalten. Der zweite DM4 wird nach der Einrichtung automatisch der zweiten Spalte zugeordnet; bis dahin wird er als offline angezeigt.

FNC Light verwendet fest die Dark-Mode-Palette des fnc-fogger-Viewers. Die beiden Geraetezuordnungen bleiben lokal in viewer.devices.local.json erhalten; die Datei enthaelt keine Zugangsdaten.
