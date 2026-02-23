#!/usr/bin/env python3

from PIL import Image, ImageDraw, ImageFont

def generate_bee_template():
    img = Image.new('RGB', (60, 60), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    font = ImageFont.truetype('/System/Library/Fonts/Apple Color Emoji.ttc', 40)

    draw.text((10, 10), '🐝', fill=(0, 0, 0), font=font)

    img.save('Bee.png')

if __name__ == "__main__":
    generate_bee_template()
