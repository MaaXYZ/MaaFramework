#!/usr/bin/env python3

from PIL import Image, ImageFont
from pilmoji import Pilmoji
from pilmoji.source import AppleEmojiSource

def generate_bee_template():
    img = Image.new('RGB', (60, 60), (46, 41, 45))

    font = ImageFont.truetype('/System/Library/Fonts/Apple Color Emoji.ttc', 40)

    with Pilmoji(img, source=AppleEmojiSource) as pilmoji:
        pilmoji.text((10, -30), '🐝', font=font)

    img.save('Bee.png')

if __name__ == "__main__":
    generate_bee_template()
