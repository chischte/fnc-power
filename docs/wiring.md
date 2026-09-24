# Anschlussplan KinCony DM4

![Anschlussplan für Taster und 0–10-V-Dimmer](wiring.svg)

Die Zeichnung zeigt Verbindungen, nicht die räumliche Position der Klemmen.
Verwendet werden der **vierte digitale Eingang** und der **vierte 0–10-V-Ausgang**.
IN4 und OUT4 sind hier Funktionsbezeichnungen; die Beschriftung am tatsächlichen Board beachten.

| Von | Nach |
| --- | --- |
| Netzteil +12–24 V DC | DM4 Versorgung + |
| Netzteil 0 V / Minus | DM4 Versorgung GND |
| DM4 vierter digitaler Eingang IN4 | Erster Kontakt des Tasters |
| Zweiter Kontakt des Tasters | DM4 GND der digitalen Eingänge |
| DM4 vierter 0–10-V-Ausgang OUT4 (DAC D) | Dimmer 0–10-V-Steuereingang |
| DM4 GND der Ausgänge | Dimmer Signalmasse |
| DM4 RJ45 | Router/Switch für DHCP und OTA |
| DM4 USB-C | PC für Erstinstallation und serielles Uhrstellen |

Der Taster ist ein **potentialfreier Schliesser (NO)**. Ungedrückt offen, gedrückt verbindet er
IN4 mit GND. **Die bisherige Verbindung des Tasters mit +24 V entfernen. Keine externe Spannung
auf den digitalen Eingang geben.** Der DM4 stellt die Eingangsbeschaltung bereits bereit;
kein externer Pull-up nötig. Die Eingänge werden über den PCF8574 aktiv LOW gelesen.

Nicht mit den analogen Eingängen A1–A4 oder freien ESP32-GPIOs verwechseln.
Bei beleuchteten Tastern die Anschlüsse für die Beleuchtung getrennt behandeln.
Ein normaler mechanischer Taster und seine zwei Leitungen können weiterverwendet werden.

OUT4 ist der vierte analoge Ausgang (MCP4728 Kanal D). OUT1–3 werden beim Firmwarestart auf 0 gesetzt.
12-Bit-Sollwerte: 35 % = 1433/4095, nominal ca. 3,50 V; Service 100 % = 4095/4095, nominal ca. 10 V.
Die DAC-Referenz ist intern mit Verstärkung 1× wie im KinCony-ESPHome-Beispiel.
Diese Werte am vorhandenen Board vor Anschluss des Dimmers messen.

`DIM+` und `DIM−` sind beispielhafte Bezeichnungen. Der Dimmer muss einen aktiven
0–10-V-Spannungseingang unterstützen und die gemeinsame Signalmasse zulassen.
Ein reiner 1–10-V-Stromsenkenanschluss ist nicht automatisch kompatibel.
Für vollständiges Ausschalten muss der Dimmer bei 0 V abschalten können.
Dimmer-Versorgung und LED-Leistungsanschlüsse richten sich nach dessen Modell und sind nicht dargestellt.

Die vor unserer Firmware laufende Werksfirmware sowie im DAC-EEPROM gespeicherte Werte können
andere Ausgangsspannungen erzeugen. Den ersten Upload und die Spannungsprüfung ohne angeschlossenen
Dimmer durchführen. Erst ab erfolgreicher Initialisierung kontrolliert unsere Firmware die Ausgänge.
Beim OTA-Update wird OUT4 abgeschaltet; nach Neustart gilt der gespeicherte Modus und aktuelle Zeitplan.

Eine SD-Karte ist nicht erforderlich. Die RTC benötigt zur Überbrückung von Stromausfällen
eine zum Board passende Pufferbatterie; Typ und Polarität nach Herstellervorgabe beachten.
USB ersetzt für den Betrieb der 0–10-V-Ausgangsstufe nicht die externe Versorgung.

## Firmwarezuordnung

| Baustein | Verbindung |
| --- | --- |
| I²C | SDA GPIO8, SCL GPIO18, 100 kHz |
| Taster IN4 | PCF8574, Adresse 0x24, P3, aktiv LOW |
| OUT4 | MCP4728, Adresse 0x60, Kanal D |
| RTC | DS3231, Adresse 0x68 |
| Ethernet W5500 | SCK GPIO1, MOSI GPIO2, MISO GPIO41, CS GPIO42, IRQ GPIO43, RESET GPIO44 |

Quellen: [DM4-Produktbeschreibung](https://www.kincony.com/esp32s3-dimmer-module-dm4-4ch-dimmer.html),
[Pinbelegung](https://www.kincony.com/forum/showthread.php?tid=8993),
[Herstellerkonfiguration](https://www.kincony.com/forum/showthread.php?tid=8994).
