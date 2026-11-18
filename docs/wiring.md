# Anschlussplan FNC Power

![Anschlussplan KinCony DM4](wiring.svg)

Der Plan zeigt die Funktionsanschluesse, nicht die raeumliche Klemmenanordnung.
Pro Lichtsteuerung werden **IN4** und **OUT4 / 0-10V** verwendet.

| Verbindung | Kabelfarbe |
| --- | --- |
| Netzteil +24V DC -> DM4 Versorgung + | Orange |
| Netzteil 0V -> DM4 Versorgungs-GND | Schwarz |
| Netzteil +24V DC -> LED-Dimmer Versorgung + | Orange |
| Netzteil 0V -> LED-Dimmer Versorgungs-GND | Schwarz |
| LED-Dimmer +24V OUT -> FNC Tower Connector 24V | Orange |
| Netzteil 0V direkt -> FNC Tower Connector GND | Schwarz |
| LED-Dimmer GND Dimmed -> FNC Tower Connector GND Dimmed | Grau |
| DM4 digital IN4 -> NO-Taster, erster Kontakt | Blau |
| NO-Taster, zweiter Kontakt -> DM4 Eingangs-GND | Weiss |
| DM4 OUT4 / 0-10V -> Dimmer 0-10V IN / DIM+ | Violett |
| DM4 Ausgangs-GND -> Dimmer Signal-GND / DIM- | Schwarz |
| Netzspannung L -> Netzteil L | Braun |
| Netzspannung N -> Netzteil N | Blau |
| Schutzleiter PE -> Netzteil PE | Gelb dargestellt; reale Ader gelb-gruen |

Der **potentialfreie Schliesser (NO)** verbindet beim Druecken IN4 mit Eingangs-GND.
Keine 24V an IN4 anlegen. Die beiden NO-Adern haben keine Polaritaet; Blau/Weiss
ist die Zuordnung dieses Plans. OUT1-3 bleiben unbenutzt.

OUT4 liefert **analog 0-10V**, kein PWM-Signal. Die violette Ader fuehrt dieses
Dimmsignal. Der Dimmer muss diesen aktiven Spannungseingang unterstuetzen und bei
0V abschalten. DIM+/DIM- sind beispielhafte Klemmenbezeichnungen.

230V AC (L, N, PE) speist das Netzteil in Feld 1. Die gelbe PE-Darstellung
steht fuer die bestaetigte gelb-gruene Schutzleiterader.

Feld 5 **FNC Tower Connector** hat die drei Anschlussfunktionen 24V, GND und GND Dimmed:
24V und GND Dimmed kommen vom LED-Dimmer; der normale GND kommt direkt vom Netzteil.
**GND Dimmed nicht mit normalem GND verbinden**, da dies den gedimmten Rueckleiter ueberbruecken wuerde.
PE und DC-GND sind getrennte Funktionen; der Plan zeigt keine Verbindung zwischen beiden.
Die Anschluesse sind funktional bezeichnet, die tatsaechlichen Klemmen nach Geraetebeschriftung zuordnen.

## 24V-Verteiler in Feld 1

- Netzteil: +24V
- DM4: Versorgung +24V DC
- LED-Dimmer: +24V IN und +24V OUT
- FNC Tower Connector: 24V (physisch vom Dimmerausgang)

## GND-Verteiler in Feld 1

- Netzteil: 0V
- DM4: Versorgungs-GND, Eingangs-GND und Signal-GND
- LED-Dimmer: Versorgungs-GND und Signal-GND
- NO-Taster: weisse Ader
- FNC Tower Connector: GND

Die beiden Verteiler sind direkt in Feld 1 beim Netzteil dargestellt. Die kurzen,
offen endenden Linien mit 24V bzw. GND an den anderen Feldern gehoeren zum
entsprechenden Verteiler. Die Signal-GND-Leitung zwischen DM4 und Dimmer sowie beide Tasterleitungen zum
DM4 sind direkt und durchgehend gezeichnet. **GND Dimmed ist ein separates Netz** und wird an Dimmer und FNC Tower Connector
mit kurzen grauen Leitungsstummeln dargestellt. Beim Dimmer liegen Steuer- und
Versorgungseingaenge links, die Leistungsausgaenge rechts. PE gehoert nicht zu Common GND.

SVG neu erzeugen: `python tools/build_wiring_diagram.py`.
Erstinstallation, Ausgangspruefung und OTA: [README](../README.md).

Quellen: [KinCony DM4](https://www.kincony.com/kincony-dm4-4ch-esp32-dimmer-board-released.html),
[Hersteller-Pinbelegung](https://www.kincony.com/forum/showthread.php?tid=8993),
[Herstellerkonfiguration](https://www.kincony.com/forum/showthread.php?tid=8994),
[IEC 60445:2021, Schutzleiterkennzeichnung](https://standards.iteh.ai/catalog/standards/iec/7275345f-6850-44bf-a57f-02870c0e23d2/iec-60445-2021).
