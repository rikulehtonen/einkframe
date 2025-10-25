"""
Image conversion script to convert png images to a custom 3-bit color byte format.
"""

import os
from PIL import Image

# ===== Color Mapping (3-bit codes) =====
COLOR_MAP = {
    (0, 0, 0):        0x0,  # BLACK   -> 000
    (255, 255, 255):  0x1,  # WHITE   -> 001
    (255, 255, 0):    0x2,  # YELLOW  -> 010
    (255, 0, 0):      0x3,  # RED     -> 011
    (0, 0, 255):      0x5,  # BLUE    -> 101
    (0, 255, 0):      0x6,  # GREEN   -> 110
}

def pixel_to_code(pixel):
    """Convert RGB pixel to color code."""
    r, g, b = pixel[:3]
    return COLOR_MAP.get((r, g, b), 0x1)  # default WHITE if not found

def convert_file(input_bmp_path, output_file_head=None):


    img = Image.open(input_bmp_path)
    pal_image = Image.new("P", (1,1))
    pal_image.putpalette( (0,0,0,
                           255,255,255,
                           255,255,0,
                           255,0,0,
                           0,0,0,
                           0,0,255,
                           0,255,0) + (0,0,0)*249)
    img = img.convert("RGB").quantize(palette=pal_image)
    img.save(output_file_head + ".bmp", format="BMP")
    img = img.convert("RGB")

    pixels = list(img.getdata())

    packed_bytes = []

    # Ensure even count by padding last pixel with WHITE if needed
    if len(pixels) % 2 != 0:
        pixels.append((255, 255, 255))

    for i in range(0, len(pixels), 2):
        code1 = pixel_to_code(pixels[i])
        code2 = pixel_to_code(pixels[i + 1])
        packed_byte = (code1 << 4) | code2
        packed_bytes.append(packed_byte)

    # Optionally save to a binary file
    if output_file_head:
        with open(output_file_head + ".bin", "wb") as f:
            f.write(bytes(packed_bytes))

    return packed_bytes

if __name__ == "__main__":

    INPUT_DIRECTORY = 'input'  # set directory path
    OUTPUT_DIRECTORY = 'output'  # set output directory path

    for entry in os.scandir(INPUT_DIRECTORY):
        if entry.is_file():  # check if it's a file
            print(f"processing file: {entry.path}")
            if entry.path.lower().endswith('.png'):
                OUTPUT_FILE_HEAD = f"{OUTPUT_DIRECTORY}/{os.path.splitext(os.path.basename(entry.path))[0]}"
                print(OUTPUT_FILE_HEAD)
                convert_file(entry.path, OUTPUT_FILE_HEAD)

    # Print as hex array
    #print(", ".join([f"0x{byte:02X}" for byte in result]))
