"""Render the native lamp geometry aligned with the companion eye icon."""
from pathlib import Path
from PIL import Image, ImageDraw

SIZE = 512
VERTICAL_OFFSET = 40
ASSETS = Path(__file__).resolve().parents[1] / "assets"


def main():
    lamp = Image.new("RGBA", (SIZE, SIZE))
    draw = ImageDraw.Draw(lamp)
    draw.ellipse((125, 45, 387, 307), fill="#6734f5", outline="#299f98", width=25)
    draw.polygon([(179, 243), (333, 243), (314, 357), (198, 357)], fill="#6734f5")
    draw.line([(207, 280), (229, 337), (283, 337), (305, 280)], fill="#ffffff", width=13)
    draw.rounded_rectangle((193, 343, 319, 421), radius=17, fill="#299f98")
    for height in (369, 394):
        draw.line((208, height, 304, height), fill="#11202c", width=10)
    draw.ellipse((169, 88, 222, 141), fill="white")
    for ray in [(256, 6, 256, 28), (45, 170, 90, 170), (422, 170, 467, 170),
                (87, 40, 118, 71), (394, 40, 425, 9)]:
        draw.line(ray, fill="#299f98", width=16)
    icon = Image.new("RGBA", lamp.size)
    icon.alpha_composite(lamp, (0, VERTICAL_OFFSET))
    icon.save(ASSETS / "light.png")
    icon.save(ASSETS / "light-aligned.ico", sizes=[(size, size) for size in (16, 24, 32, 48, 64, 128, 256)])


if __name__ == "__main__":
    main()
