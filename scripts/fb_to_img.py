from PIL import Image
import numpy as np

# Script to turn a binary dump of the framebuffer into an image
# How to use:
#  - Open up `arm-none-eabi-gdb build/pico_oled.elf`
#  - Get the address of the framebuffer: `p frame_buffer->pbuf`
#  - Use the 0x.... address to dump the framebuffer: `dump binary memory pbuf_dump.bin {{address of pbuff}} {{address of pbuff}} + 2048`
#  - Activate the .venv, install requirements.txt and execute `python3 fb_to_img.py`

# Framebuffer specifications
width, height = 256, 64
file_path = "pbuf_dump.bin"
output_image_path = "framebuffer_image.png"

# Read the binary data
with open(file_path, "rb") as f:
    framebuffer_data = f.read()

# We will store here the bit-to-byte unpacked data
pixels = []
for byte in framebuffer_data:
    for i in reversed(range(8)):
        bit = (byte >> i) & 1
        pixels.append(255 if bit else 0)

# Reshape the array into a 2D array
pixels = np.array(pixels).astype(np.uint8).reshape(height, width)

# Convert to an image
image = Image.fromarray(pixels)
image = image.resize((width * 4, height * 4), Image.NEAREST)
image.save(output_image_path)
print(f"Image saved as {output_image_path}")