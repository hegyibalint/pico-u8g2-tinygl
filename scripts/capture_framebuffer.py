import os
import asyncio

from PIL import Image
import numpy as np


def unpack_framebuffer_bits(framebuffer: bytes) -> list[int]:
    """
    The framebuffer comes in a format, where each *bit* represents a pixel.
    This function will unpack the bits into bytes, where each byte represents a pixel.
    """
    pixels = []
    for byte in framebuffer:
        for i in reversed(range(8)):
            bit = (byte >> i) & 1
            pixels.append(255 if bit else 0)

    assert len(pixels) == len(framebuffer) * 8
    return pixels


def convert_to_image(framebuffer: bytes, width: int, height: int) -> Image:
    pixels = np.array(framebuffer).astype(np.uint8)
    pixels = pixels.reshape(height, width)
    image = Image.fromarray(pixels)
    return image


async def read_size_t(reader: asyncio.StreamReader):
    number = await reader.read(4)
    # We send the bytes in reverse order
    number = number[::-1]
    number = int.from_bytes(number, "big")
    return number


async def read_byte(reader: asyncio.StreamReader):
    raw_byte = await reader.read(1)
    raw_byte = int.from_bytes(raw_byte, "big")
    # Sanity check to check if the channel coding is intact
    if raw_byte & 0x0F != 0x0F:
        raise ValueError(f"Invalid byte received: {raw_byte}, lower nibble is not 0x0F")
    # Extract the byte
    encoded_byte = (raw_byte >> 4) & 0x0F
    return encoded_byte


async def read_framebuffer(reader: asyncio.StreamReader, length: int):
    framebuffer = []
    for _ in range(length):
        high_byte = await read_byte(reader)
        high_byte = high_byte << 4
        low_byte = await read_byte(reader)
        framebuffer.append(high_byte | low_byte)
    return framebuffer


async def read_frames(reader: asyncio.StreamReader):
    frame_i = 0

    while True:
        bytes = await reader.read(1)
        if bytes[0] == 0:
            # Read the frame size (sent as a size_t)
            length = await read_size_t(reader)
            width = await read_size_t(reader)
            height = await read_size_t(reader)
            # Sanity check to check if the unpacked framebuffer is of the correct size
            if length != (width * height) / 8:
                print(f"Invalid frame size: {length} != {width} * {height} / 8")
                continue
            # Read the encoded frame: every byte will be send as two bytes
            frame = await read_framebuffer(reader, length)
            print(f"Received frame of size {width}x{height}")
            # Unpack and convert the framebuffer
            image_bytes = unpack_framebuffer_bits(frame)
            image = convert_to_image(image_bytes, width, height)
            # Save the image
            image.save(f"fb/frame_{frame_i:04d}.png")
            frame_i += 1


async def main():
    # Make the fb directory if it doesn't exist
    if not os.path.exists("fb"):
        os.makedirs("fb")
    # Delete all pictures in the fb directory
    for file in os.listdir("fb"):
        os.remove(f"fb/{file}")

    while True:
        try:
            reader, _ = await asyncio.open_connection("localhost", 19021)
            print("Connected to server")
            break
        except Exception:
            print("Could not connect to server, retrying in 5 seconds...")
            await asyncio.sleep(5)

    await read_frames(reader)


if __name__ == "__main__":
    asyncio.run(main())
