# fnc-power

0–10-V-Lichtsteuerung für **CONTROLLINO MAXI Automation**, gebaut mit PlatformIO.

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

Bei 17 % wurden mindestens 44 lux in der Box gemessen, bei 50 % mindestens 800 lux.
Bei 40 % wurden 510 lux gemessen, bei 30 % wurden 260 lux gemessen.
Bei 35 % wurden mindestens 369 lux gemessen. Die Automatik bleibt deshalb auf 35 %
(ca. 3,49 V); das Ziel von mindestens 300 lux ist erreicht.
Die aufsteigend sortierte Messtabelle steht direkt in `UserConfig.h`.

Zeiten sind lokale RTC-Zeiten mit Minutenauflösung. Zeitfenster tagsüber und über Mitternacht
werden unterstützt; gleiche Ein-/Ausschaltzeiten deaktivieren die Automatik (Service bleibt verfügbar).
Ungültige Prozentwerte, Stunden oder Minuten erzeugen beim Build eine Fehlermeldung.
Die automatischen Übergänge dauern weiterhin 60 Sekunden.

Nach Änderungen `pio run -t upload --upload-port COM5` ausführen (Port anpassen).
Die Werte sind Build-Konfiguration und werden nicht zur Laufzeit vom Gerät aus einer Datei gelesen.
Die folgenden Verhaltensangaben beschreiben die Standardkonfiguration.

## Verhalten

| Aktion / Uhrzeit | Ausgang |
| --- | --- |
| Ab 20:00:00 | In 60 Sekunden von 0 auf 35 %, danach 35 % (ca. 3,49 V) |
| Ab 08:00:00 | In 60 Sekunden von 35 auf 0 %, danach Aus (0 V) |
| Kurzer Tastendruck, beim Loslassen | In 5 s vom aktuellen Wert auf 80 %, nach 10 Minuten zurück zum Zeitplan |
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
Der analoge Ausgang hat 8 Bit Auflösung: zwischen 0 und 35 % erfolgt die Rampe in 89 kleinen Stufen.

Langes Halten löst genau einmal aus, beim Loslassen folgt kein kurzer Druck.
Der Taster wird mit 30 ms entprellt. Dimmrampen und Servicebetrieb blockieren die Hauptschleife nicht.
Das Servicelicht funktioniert auch bei ausgeschalteter Automatik und ohne gueltige RTC.
Ein kurzer Druck bricht eine laufende Modus-Rueckmeldung ab und startet das Servicelicht.
Der gespeicherte Automatikmodus bleibt unveraendert. Ist die Automatik aus, geht das Licht
nach erneutem kurzem Druck oder nach 10 Minuten wieder direkt auf 0 %.
Ein langer Druck beendet den Servicebetrieb. Dauerhaft Aus bleibt im EEPROM über Neustarts erhalten;
ein fabrikneues EEPROM startet in Automatik. Servicebetrieb und Rampen werden nach Neustart nicht fortgesetzt.

## Anschlüsse

[Anschlussplan mit Klemmentabelle](docs/wiring.md)

![Anschlussplan](docs/wiring.svg)

- CONTROLLINO mit 24 V DC versorgen.
- **AO0** → 0–10-V-Steuereingang des Dimmers; dessen Signalmasse mit CONTROLLINO GND verbinden,
  soweit vom Dimmerhersteller vorgesehen. AO0 muss als Spannungsausgang konfiguriert sein (Werkseinstellung).
- **A0** → Schliessertaster → +24 V. Verwendet wird die Schraubklemme A0, digital als aktiv HIGH gelesen.
  Die interne Eingangsbeschaltung hält einen offenen Taster auf LOW; kein Arduino-Pullup nötig.
- Die 24 V gehören an die Schraubklemmen, nicht an den 5-V-Pinheader.

17 % beschreibt den analogen Sollwert (43/255 des 10-V-Bereichs). Ob das 17 % Lichtleistung ergibt,
hängt von der Kennlinie des Dimmers ab. Der Dimmer muss bei **0 V tatsächlich abschalten**;
ein Gerät mit Mindesthelligkeit bei 0 V benötigt zusätzlich einen Freigabe-/Abschaltkontakt.

## Bauen und Hochladen

PlatformIO IDE in VS Code installieren (Erweiterungsempfehlung ist hinterlegt).

```powershell
pio run
pio device list
pio run -t upload --upload-port COM5
```

`COM5` durch den tatsächlichen USB-Port ersetzen. Zielboard und Bibliotheken sind in `platformio.ini` festgelegt.
Ein Firmware-Upload wurde bei der Projekterstellung nicht durchgeführt.

## Uhr stellen

Die eingebaute RV-2123-RTC hält die **lokale Uhrzeit**. Bei Neustart wird sie weder zurückgesetzt
noch auf eine alte Build-Zeit gestellt. Ohne gültige RTC bleibt der Zeitplan aus;
Servicebetrieb und Taster-Rückmeldungen funktionieren weiterhin. Ein erkennbarer Oszillatorstopp,
ein gestoppter Zähler oder ungültige Kalenderwerte werden als Fehler gemeldet.

Nach dem ersten Upload den seriellen Monitor schliessen und die PC-Uhr übertragen:

```powershell
& "$env:USERPROFILE/.platformio/penv/Scripts/python.exe" tools/set_clock.py COM5
```

Alternativ mit 115200 Baud und Zeilenende LF im seriellen Monitor:

```text
TIME 2026-09-11 19:55:00
TIME?
```

Die erste Zeile ist ein Formatbeispiel: tatsächliches lokales Datum und Uhrzeit einsetzen.
`OK TIME` bestätigt die Übernahme; `TIME?` liest die Uhr zurück. Zulässig sind 2000–2099.
Es gibt keine Netzwerk- oder automatische Sommerzeitkorrektur. Nach Sommer-/Winterzeitwechsel
die Uhr erneut vom PC setzen. Ohne Umstellung läuft der Rhythmus in der eingestellten festen Zeit weiter.
Die RTC-Pufferung ist zeitlich begrenzt; nach längerer stromloser Lagerung die Uhr prüfen/stellen.

## Aufbau und Tests

- `lib/Control/Control.h`: hardwareunabhängige Taster- und Lichtlogik; Bedienzeiten in `Config`, Benutzereinstellungen in `UserConfig.h`.
- `src/RtcClock.cpp`: SPI-Zugriff auf die RTC, Kalenderprüfung und Uhrstellen.
- `src/SerialClock.cpp`: begrenzter, nicht wartender serieller Befehlsparser.
- `src/main.cpp`: Anschlüsse, EEPROM und Zusammenführung der Module.

```powershell
python tools/test_control.py
pio run
```

Die Hosttests benötigen Python und LLVM/Clang oder GCC. Sie testen den tatsächlichen C++-Code:
Tagesverlauf, Serviceablauf und Umschalten per kurzem Tastendruck, Moduswechsel, Rampenendpunkte, Entprellung,
einmalige Langerkennung, die 60-Sekunden-Automatikrampen mit Übersteuerungen und den `millis()`-Überlauf. Der Firmware-Build prüft die AVR-Integration.
Elektrische Ausgangswerte, RTC-Pufferung und Dimmerverhalten müssen am angeschlossenen Gerät geprüft werden.

Zur Inbetriebnahme: Uhr auf 07:59:55 und 19:59:55 stellen und Umschaltungen beobachten;
Servicebetrieb tags und nachts prüfen; beide langen Tastendrücke und Neustart bei dauerhaft Aus testen.
Anschliessend die aktuelle Uhrzeit wieder setzen.

## Hardwarequellen

- [CONTROLLINO: Analoge Ausgänge](https://www.controllino.com/current-outputs-maxi-automation/)
- [CONTROLLINO: RTC](https://www.controllino.com/rtc-real-time-clock/)
- [CONTROLLINO-Bibliothek und Pinzuordnung](https://github.com/CONTROLLINO-PLC/CONTROLLINO_Library)
- [Micro Crystal: RV-2123 Anwendungshandbuch](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-2123-C2_App-Manual.pdf)
