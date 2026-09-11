# fnc-power

0–10-V-Lichtsteuerung für **CONTROLLINO MAXI Automation**, gebaut mit PlatformIO.

## Verhalten

| Aktion / Uhrzeit | Ausgang |
| --- | --- |
| Ab 20:00:00 | In 60 Sekunden von 0 auf 17 %, danach 17 % (ca. 1,69 V) |
| Ab 08:00:00 | In 60 Sekunden von 17 auf 0 %, danach Aus (0 V) |
| Kurzer Tastendruck, beim Loslassen | 100 % für 10 Minuten, danach aktueller Zeitplan |
| Weiterer kurzer Druck im Servicebetrieb | Startet die 10 Minuten erneut |
| Langer Druck (2 Sekunden) | Dauerhaft aus; einmal 100 → 0 % in 3 Sekunden |
| Erneuter langer Druck | Automatik; einmal 0 → 100 % in 3 Sekunden, danach aktueller Zeitplan |

Die automatischen Rampen laufen auch während Servicebetrieb, dauerhaft Aus und Taster-Rückmeldungen
im Hintergrund weiter. Nach einer Übersteuerung gilt der aktuelle Wert der Zeitplanrampe.
Die Rückmeldungen bei langem Tastendruck dauern weiterhin 3 Sekunden.
Beim Start oder nach Wiederherstellung einer gültigen RTC gilt sofort der aktuelle Tag-/Nacht-Sollwert.
Ein Uhrstellen über eine Schaltgrenze startet ebenfalls eine 60-Sekunden-Rampe vom aktuellen Automatikwert.
Der analoge Ausgang hat 8 Bit Auflösung: zwischen 0 und 17 % erfolgt die Rampe in 43 kleinen Stufen.

Langes Halten löst genau einmal aus, beim Loslassen folgt kein kurzer Druck.
Der Taster wird mit 30 ms entprellt. Dimmrampen und Servicebetrieb blockieren die Hauptschleife nicht.
Kurze Tastendrücke werden bei dauerhaft Aus und während einer Rückmelderampe ignoriert.
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

- `lib/Control/Control.h`: hardwareunabhängige Taster- und Lichtlogik; Zeiten/Helligkeit in `Config`.
- `src/RtcClock.cpp`: SPI-Zugriff auf die RTC, Kalenderprüfung und Uhrstellen.
- `src/SerialClock.cpp`: begrenzter, nicht wartender serieller Befehlsparser.
- `src/main.cpp`: Anschlüsse, EEPROM und Zusammenführung der Module.

```powershell
python tools/test_control.py
pio run
```

Die Hosttests benötigen Python und LLVM/Clang oder GCC. Sie testen den tatsächlichen C++-Code:
Tagesverlauf, Serviceablauf/-verlängerung, Moduswechsel, Rampenendpunkte, Entprellung,
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
