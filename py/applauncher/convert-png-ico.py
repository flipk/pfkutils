#!/usr/bin/env python3

from PIL import Image

# 1. Define the file paths
png_file = 'app-launcher.png'
ico_file = 'app-launcher.ico'

# 2. Open the PNG image
img = Image.open(png_file)

# 3. Save as ICO, specifying desired sizes
# Common sizes include 16x16, 24x24, 32x32, 48x48, 64x64, 128x128, and 256x256.
# Windows uses different sizes for different views.
# Including a few common ones is good practice.
icon_sizes = [(32, 32), (48, 48), (64, 64), (128, 128), (256, 256), (512, 512)]
img.save(ico_file, sizes=icon_sizes)

print(f"Successfully converted '{png_file}' to '{ico_file}'")
