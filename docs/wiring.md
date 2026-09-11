# Anschlussplan

![Anschlussplan für Taster und 0–10-V-Dimmer](wiring.svg)

Der Plan entspricht der Firmware: `CONTROLLINO_A0` für den Taster und `CONTROLLINO_AO0` für den Dimmer.
Die Zeichnung zeigt Verbindungen, nicht die räumliche Position der Klemmen.

| Von | Nach |
| --- | --- |
| Netzteil +24 V | CONTROLLINO VIN / Versorgung +24 V |
| Netzteil 0 V / Minus | CONTROLLINO GND |
| Netzteil +24 V | Erster Kontakt des Tasters |
| Zweiter Kontakt des Tasters | CONTROLLINO **A0** |
| CONTROLLINO **AO0** | 0–10-V-Steuereingang des Dimmers |
| CONTROLLINO GND | Signalmasse des Dimmers |

Der Taster ist ein **Schliesser (NO)**: ungedrückt offen, gedrückt verbindet er +24 V mit A0.
Verwende die **Schraubklemme A0**, nicht den Pinheader. Es ist kein zusätzlicher Pullup vorgesehen.
Bei einem beleuchteten Taster sind die Anschlüsse für dessen Beleuchtung getrennt zu behandeln.

**AO0** ist der erste analoge Ausgang. **AO1 bleibt unbenutzt.** AO0 muss im Spannungsmodus
0–10 V bleiben; die Werkseinstellung wird durch vorhandene 0-Ω-Brücken bestimmt.
Bei 17 % gibt die Firmware etwa 1,69 V aus, bei 100 % etwa 10 V, bei Aus 0 V.

`DIM+` und `DIM−` im Diagramm sind beispielhafte Bezeichnungen. Der Dimmer muss einen
aktiven 0–10-V-Spannungseingang besitzen, und sein Hersteller muss die gemeinsame Signalmasse
zulassen. Ein reiner 1–10-V-Stromsenkenanschluss ist damit nicht automatisch kompatibel.
Für vollständiges Ausschalten muss der Dimmer bei 0 V abschalten können.
Dimmer-Versorgung und LED-Leistungsanschlüsse richten sich nach dessen Modell und sind nicht dargestellt.

Geprüfte Herstellerunterlagen:

- [MAXI Automation Pinout](https://www.controllino.com/wp-content/uploads/2023/05/CONTROLLINO_MAXI_AUTOMATION_Pinout_Table.pdf)
- [Analoge Ausgänge und Spannungs-/Strommodus](https://www.controllino.com/current-outputs-maxi-automation/)