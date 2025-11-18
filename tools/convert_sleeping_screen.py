#!/usr/bin/env python3
"""
Convert image.jpg to 4-bit grayscale hex array for M5Paper e-ink display
Target size: 540x960 pixels (portrait mode)
Format: Two pixels packed per byte (high nibble, low nibble)
Grayscale range: 0 (white) to 15 (black)
"""

from PIL import Image
import sys

def convert_image_to_4bit_grayscale(input_path, output_name="sleepingScreen"):
    """Convert image to 4-bit grayscale array"""
    
    # Target dimensions for M5Paper in portrait mode
    target_width = 540
    target_height = 960
    
    # Open and process image
    img = Image.open(input_path)
    
    # Convert to grayscale
    img = img.convert('L')
    
    # Resize to target dimensions
    img = img.resize((target_width, target_height), Image.Resampling.LANCZOS)
    
    # Get pixel data
    pixels = list(img.getdata())
    
    # Convert 8-bit grayscale (0-255) to 4-bit grayscale (0-15)
    # No inversion needed - M5Paper IT8951 uses 0=black, 15=white
    pixels_4bit = [p >> 4 for p in pixels]
    
    # Pack two pixels per byte
    packed_bytes = []
    for i in range(0, len(pixels_4bit), 2):
        high_nibble = pixels_4bit[i]
        low_nibble = pixels_4bit[i + 1] if i + 1 < len(pixels_4bit) else 0
        packed_bytes.append((high_nibble << 4) | low_nibble)
    
    # Generate C array
    array_size = len(packed_bytes)
    
    print(f"// {output_name} converted to 4-bit grayscale for M5Paper e-ink")
    print(f"// Scaled to: {target_width}x{target_height} pixels (portrait mode)")
    print(f"// Format: Two pixels packed per byte (high nibble, low nibble)")
    print(f"// Grayscale range: 0 (black) to 15 (white)")
    print()
    print(f"const uint16_t {output_name}_gray_width = {target_width};")
    print(f"const uint16_t {output_name}_gray_height = {target_height};")
    print()
    print(f"const uint8_t {output_name}_gray[{array_size}] PROGMEM = {{")
    
    # Write bytes in rows of 16
    for i in range(0, len(packed_bytes), 16):
        row = packed_bytes[i:i+16]
        hex_str = ", ".join(f"0x{b:02X}" for b in row)
        if i + 16 < len(packed_bytes):
            print(f"    {hex_str},")
        else:
            print(f"    {hex_str}")
    
    print("};")
    print()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_sleeping_screen.py <image_path>")
        sys.exit(1)
    
    input_image = sys.argv[1]
    convert_image_to_4bit_grayscale(input_image)
