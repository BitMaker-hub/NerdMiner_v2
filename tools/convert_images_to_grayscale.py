#!/usr/bin/env python3
"""
Convert RGB565 color images to 4-bit grayscale for M5Paper e-ink display.
This script converts the images from images_536_240.h to grayscale format.
"""

import re
import sys

def rgb565_to_gray4(rgb565):
    """
    Convert RGB565 value to 4-bit grayscale (0-15).
    RGB565 format: RRRRR GGGGGG BBBBB
    """
    # Extract RGB components
    r = ((rgb565 >> 11) & 0x1F) * 255 // 31  # 5 bits red
    g = ((rgb565 >> 5) & 0x3F) * 255 // 63   # 6 bits green
    b = (rgb565 & 0x1F) * 255 // 31          # 5 bits blue
    
    # Convert to grayscale using standard weights
    gray8 = int(0.299 * r + 0.587 * g + 0.114 * b)
    
    # Convert 8-bit grayscale (0-255) to 4-bit (0-15)
    gray4 = gray8 >> 4
    
    return gray4

def convert_image_array(input_file, output_file, image_name, width, height):
    """
    Read RGB565 image array from input file and write 4-bit grayscale to output file.
    Two pixels are packed into each uint8_t.
    """
    print(f"Converting {image_name} ({width}x{height})...")
    
    with open(input_file, 'r') as f:
        content = f.read()
    
    # Find the image array
    pattern = rf'const unsigned short {image_name}\[\d+\] PROGMEM\s*=\s*{{([^}}]+)}};'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        print(f"ERROR: Could not find {image_name} in {input_file}")
        return False
    
    # Extract hex values
    array_content = match.group(1)
    hex_values = re.findall(r'0x[0-9A-Fa-f]+', array_content)
    
    total_pixels = width * height
    if len(hex_values) != total_pixels:
        print(f"WARNING: Expected {total_pixels} pixels, found {len(hex_values)}")
    
    # Convert to grayscale
    gray_values = []
    for hex_val in hex_values:
        rgb565 = int(hex_val, 16)
        gray4 = rgb565_to_gray4(rgb565)
        gray_values.append(gray4)
    
    # Pack two 4-bit values into each byte
    packed_bytes = []
    for i in range(0, len(gray_values), 2):
        if i + 1 < len(gray_values):
            # Pack two pixels: high nibble = first pixel, low nibble = second pixel
            packed = (gray_values[i] << 4) | gray_values[i + 1]
        else:
            # Odd number of pixels, pack with 0
            packed = gray_values[i] << 4
        packed_bytes.append(packed)
    
    # Write output file
    with open(output_file, 'w') as f:
        f.write(f"// {image_name} converted to 4-bit grayscale for M5Paper e-ink\n")
        f.write(f"// Original size: {width}x{height} pixels\n")
        f.write(f"// Format: Two pixels packed per byte (high nibble, low nibble)\n")
        f.write(f"// Grayscale range: 0 (white) to 15 (black)\n\n")
        f.write(f"const uint16_t {image_name}_gray_width = {width};\n")
        f.write(f"const uint16_t {image_name}_gray_height = {height};\n\n")
        f.write(f"const uint8_t {image_name}_gray[{len(packed_bytes)}] PROGMEM = {{\n")
        
        # Write bytes in rows of 16
        for i in range(0, len(packed_bytes), 16):
            row = packed_bytes[i:i+16]
            hex_row = ', '.join([f'0x{b:02X}' for b in row])
            f.write(f'    {hex_row}')
            if i + 16 < len(packed_bytes):
                f.write(',\n')
            else:
                f.write('\n')
        
        f.write('};\n\n')
    
    print(f"✓ Converted {image_name}: {len(hex_values)} pixels -> {len(packed_bytes)} bytes")
    return True

def main():
    input_file = '../src/media/images_536_240.h'
    output_file = '../src/media/images_m5paper_gray.h'
    
    print("M5Paper Grayscale Image Converter")
    print("=" * 50)
    
    # Images to convert
    images = [
        ('initScreen', 536, 240),
        ('MinerScreen', 536, 240),
        ('minerClockScreen', 536, 240),
        ('globalHashScreen', 536, 240),
        ('setupModeScreen', 536, 240),
    ]
    
    # Create header file
    with open(output_file, 'w') as f:
        f.write("// M5Paper Grayscale Images\n")
        f.write("// Auto-generated from images_536_240.h\n")
        f.write("// Format: 4-bit grayscale, two pixels per byte\n\n")
        f.write("#ifndef IMAGES_M5PAPER_GRAY_H\n")
        f.write("#define IMAGES_M5PAPER_GRAY_H\n\n")
        f.write("#include <Arduino.h>\n\n")
    
    success_count = 0
    for img_name, width, height in images:
        # Append each image to the output file
        with open(output_file, 'a') as f:
            pass  # File already open in append mode below
        
        if convert_image_array(input_file, output_file + '.tmp', img_name, width, height):
            # Append temp content to main file
            with open(output_file + '.tmp', 'r') as tmp:
                content = tmp.read()
            with open(output_file, 'a') as out:
                out.write(content)
            success_count += 1
    
    # Close header guard
    with open(output_file, 'a') as f:
        f.write("#endif // IMAGES_M5PAPER_GRAY_H\n")
    
    # Clean up temp file
    import os
    if os.path.exists(output_file + '.tmp'):
        os.remove(output_file + '.tmp')
    
    print("=" * 50)
    print(f"✓ Converted {success_count}/{len(images)} images successfully")
    print(f"✓ Output written to: {output_file}")

if __name__ == '__main__':
    main()
