"""Build the minimal, editable FNC Light wiring diagram (no external assets)."""
from html import escape
from pathlib import Path

OUTPUT = Path(__file__).resolve().parents[1] / "docs" / "wiring.svg"
STUB_LENGTH = 50
DIMMED_STUB_LENGTH = 110
CONNECTION_FONT_SIZE = 20
CONNECTION_FONT_WEIGHT = "600"
COLORS = {"24v": "#ed7b15", "gnd": "#202c38", "blue": "#1767d2",
          "white": "#ffffff", "dim": "#7737be", "phase": "#87502e", "pe": "#efc526", "dimmed": "#858c94"}


class Diagram:
    def __init__(self):
        self.elements = ['''<svg xmlns="http://www.w3.org/2000/svg" width="1770" height="940" viewBox="0 0 1770 940" role="img" aria-labelledby="title desc">
<title id="title">FNC Power wiring diagram</title>
<desc id="desc">24V orange and GND black supply the KinCony DM4. A blue and white normally open push button connects digital IN4 to input GND. OUT4 supplies a violet 0-10V dimming signal with black signal ground. Mains L, N and PE feed the 24V DC supply, which powers the DM4 and LED dimmer. The 24V and GND distribution branches are integrated into supply panel 1. Grey GND Dimmed is a separate net shown as short labelled stubs. All dimmer inputs are on the left; its outputs are on the right.</desc>
<defs><linearGradient id="header" x2="1" y2="1"><stop stop-color="#1253a2"/><stop offset="1" stop-color="#173e77"/></linearGradient></defs>
<rect width="1770" height="940" fill="#f2f6fb"/>
<g font-family="Segoe UI, Arial, sans-serif" fill="#17324e">''']

    def text(self, x, y, value, size=18, color="#17324e", weight="400", anchor="start"):
        self.elements.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}" text-anchor="{anchor}">{escape(value)}</text>')

    def box(self, x, y, width, height, fill="#fff", stroke="#d6e1ee", radius=12):
        self.elements.append(f'<rect x="{x}" y="{y}" width="{width}" height="{height}" rx="{radius}" fill="{fill}" stroke="{stroke}"/>')

    def panel(self, x, y, width, height, number, title):
        self.box(x, y, width, height)
        self.box(x, y, width, 52, "url(#header)", "none")
        self.box(x+12, y+9, 34, 34, "#fff", "none", 8)
        self.text(x+29, y+34, str(number), 24, "#173e77", "700", "middle")
        self.text(x+60, y+34, title, 24, "#fff", "700")

    def wire(self, points, color, width=5):
        coordinates = " ".join(f"{x},{y}" for x, y in points)
        if color == "white":
            self.elements.append(f'<polyline points="{coordinates}" fill="none" stroke="#8da0b3" stroke-width="{width+3}" stroke-linejoin="round"/>')
        else:
            self.elements.append(f'<polyline points="{coordinates}" fill="none" stroke="#f2f6fb" stroke-width="{width+4}" stroke-linecap="round" stroke-linejoin="round"/>')
        self.elements.append(f'<polyline points="{coordinates}" fill="none" stroke="{COLORS.get(color, color)}" stroke-width="{width}" stroke-linecap="round" stroke-linejoin="round"/>')

    def terminal(self, x, y):
        self.elements.append(f'<circle cx="{x}" cy="{y}" r="6" fill="#fff" stroke="#17324e" stroke-width="2"/>')

    def stub(self, x, y, direction, label, color):
        length = DIMMED_STUB_LENGTH if color == "dimmed" else STUB_LENGTH
        self.elements.append(f'<line x1="{x}" y1="{y}" x2="{x+direction*length}" y2="{y}" stroke="{COLORS[color]}" stroke-width="5" stroke-linecap="butt"/>')
        self.text(x+direction*length/2, y-10, label, 15, COLORS[color], "600", "middle")

    def connection(self, x, y, label, anchor="start"):
        self.text(x, y, label, CONNECTION_FONT_SIZE, weight=CONNECTION_FONT_WEIGHT, anchor=anchor)

    def save(self):
        self.elements.append("</g></svg>")
        OUTPUT.write_text("\n".join(self.elements), encoding="utf-8")


def draw_supply(d):
    d.panel(24, 115, 550, 425, 1, "24V DC Supply")
    d.elements.append('<g transform="translate(-5 0)">')
    d.box(110, 210, 150, 250, "#edf2f7", "#9eafc4", 7)
    d.text(185, 315, "24V", 24, weight="700", anchor="middle")
    d.text(185, 343, "DC", 15, "#526b84", anchor="middle")
    d.text(48, 195, "230V AC", 17, weight="600")
    for y, label, color in [(270, "L", "phase"), (330, "N", "blue"), (390, "PE", "pe")]:
        d.text(48, y-12, label, 16, weight="600")
        d.wire([(48, y), (110, y)], color)
        d.terminal(110, y)
    draw_supply_bus(d, 300, "24V DC", "24v")
    draw_supply_bus(d, 410, "GND", "gnd")
    d.elements.append("</g>")


def draw_supply_bus(d, feed_y, label, color):
    bus_x = 350
    d.text(266, feed_y-10, label, 18, COLORS[color], "600")
    d.wire([(260, feed_y), (bus_x, feed_y)], color, 4)
    d.wire([(bus_x, feed_y-27), (bus_x, feed_y+27)], color, 4)
    d.terminal(260, feed_y)
    for offset, device in [(-27, "KinCony"), (0, "LED Dimmer"), (27, "FNC Tower Connector")]:
        y = feed_y+offset
        d.wire([(bus_x, y), (bus_x+24, y)], color, 4)
        d.elements.append(f'<circle cx="{bus_x}" cy="{y}" r="5" fill="{COLORS[color]}"/>')
        d.text(bus_x+36, y+6, device, 18)


def draw_controller(d):
    d.panel(689, 115, 460, 785, 2, "KinCony DM4")
    d.box(705, 203, 190, 218, "#edf4fc", "#d6e1ee", 8)
    d.text(721, 236, "POWER", 14, "#526b84", "700")
    for y, label, net, color in [(260, "+24V DC", "24V", "24v"), (340, "GND", "GND", "gnd")]:
        d.stub(689, y, -1, net, color)
        d.connection(721, y+7, label)
        d.terminal(689, y)
    d.box(943, 203, 190, 218, "#edf4fc", "#d6e1ee", 8)
    d.text(1117, 236, "0-10V OUTPUTS", 14, "#526b84", "700", "end")
    for y, label, color in [(270, "OUT4 / 0-10V", "dim"), (350, "Signal GND", "gnd")]:
        d.connection(1117, y+7, label, "end")
        d.wire([(1133, y), (1264, y)], color)
        d.terminal(1149, y)
    d.box(705, 585, 190, 190, "#edf4fc", "#d6e1ee", 8)
    d.text(721, 618, "DIGITAL INPUTS", 14, "#526b84", "700")
    for y, label in [(650, "IN4"), (740, "GND")]:
        d.connection(721, y+7, label)
        d.terminal(689, y)


def draw_dimmer(d):
    d.panel(1264, 115, 370, 480, 3, "LED Dimmer")
    d.text(1288, 208, "CONTROL INPUT", 14, "#526b84", "700")
    for y, label in [(270, "0-10V"), (350, "GND")]:
        d.connection(1288, y+7, label)
        d.terminal(1264, y)
    d.text(1288, 408, "POWER INPUT", 14, "#526b84", "700")
    for y, label, color in [(455, "+24V", "24v"), (535, "GND", "gnd")]:
        d.connection(1288, y+7, label)
        d.stub(1264, y, -1, "24V" if color == "24v" else "GND", color)
        d.terminal(1264, y)
    d.text(1610, 208, "POWER OUTPUT", 14, "#526b84", "700", "end")
    d.connection(1610, 277, "GND Dimmed", "end")
    d.stub(1634, 270, 1, "GND Dimmed", "dimmed")
    d.terminal(1634, 270)


def draw_load_supply(d):
    d.panel(1264, 660, 370, 240, 5, "FNC Tower Connector")
    for y, label, color in [(750, "24V", "24v"), (810, "GND", "gnd"), (870, "GND Dimmed", "dimmed")]:
        d.connection(1288, y+7, label)
        d.stub(1264, y, -1, label, color)
        d.terminal(1264, y)


def draw_button(d):
    d.panel(204, 574, 370, 220, 4, "Push Button")
    d.text(437, 703, "NO", 24, weight="700", anchor="middle")
    d.wire([(357, 650), (689, 650)], "blue")
    d.wire([(357, 740), (689, 740)], "white")
    d.wire([(357, 650), (387, 720)], "gnd", 3)
    d.wire([(322, 695), (346, 695)], "gnd", 3)
    d.wire([(318, 682), (318, 708)], "gnd", 3)
    for x in (357, 574):
        for y in (650, 740):
            d.terminal(x, y)
    for y in (650, 740):
        d.terminal(689, y)


def main():
    diagram = Diagram()
    diagram.text(24, 52, "FNC Power", 40, weight="700")
    diagram.text(265, 52, "Wiring Diagram", 33, weight="600")
    diagram.text(1746, 50, "KinCony DM4 | 24V DC | OUT4 + IN4", 18, "#526b84", anchor="end")
    diagram.text(1746, 83, "Functional connections - not physical terminal positions", 17, "#526b84", anchor="end")
    draw_supply(diagram)
    draw_controller(diagram)
    draw_dimmer(diagram)
    draw_load_supply(diagram)
    draw_button(diagram)
    for x in (1149, 1264):
        for y in (270, 350):
            diagram.terminal(x, y)
    diagram.save()
    print(OUTPUT)


if __name__ == "__main__":
    main()
