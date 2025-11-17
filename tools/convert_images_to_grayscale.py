#!/usr/bin/env python3
"""
Convert RGB565 color images to 4-bit grayscale for M5Paper e-ink display.
This script converts the images from images_536_240.h to grayscale format,
with optional scaling to fit the M5Paper screen width (540 pixels).
"""

import re
import sys

def rgb565_to_gray4(rgb565, opacity=1.0, invert=False):
    """
    Convert RGB565 value to 4-bit grayscale (0-15).
    RGB565 format: RRRRR GGGGGG BBBBB
    
    Args:
        opacity: Not used anymore - keeping original image brightness
        invert: If True, invert the grayscale (0 becomes 15, etc.)
    """
    # Extract RGB components
    r = ((rgb565 >> 11) & 0x1F) * 255 // 31  # 5 bits red
    g = ((rgb565 >> 5) & 0x3F) * 255 // 63   # 6 bits green
    b = (rgb565 & 0x1F) * 255 // 31          # 5 bits blue
    
    # Convert to grayscale using standard weights
    gray8 = int(0.299 * r + 0.587 * g + 0.114 * b)
    
    # Convert 8-bit grayscale (0-255) to 4-bit (0-15)
    # For e-ink: higher values = darker
    gray4 = gray8 >> 4
    
    # Invert if requested
    if invert:
        gray4 = 15 - gray4
    
    return gray4

def bilinear_interpolate(pixels, src_width, src_height, x, y):
    """
    Bilinear interpolation for smooth scaling.
    pixels: list of grayscale values (0-15)
    x, y: floating point coordinates in source image
    """
    x1 = int(x)
    y1 = int(y)
    x2 = min(x1 + 1, src_width - 1)
    y2 = min(y1 + 1, src_height - 1)
    
    # Get four neighboring pixels
    q11 = pixels[y1 * src_width + x1]
    q21 = pixels[y1 * src_width + x2]
    q12 = pixels[y2 * src_width + x1]
    q22 = pixels[y2 * src_width + x2]
    
    # Interpolation weights
    wx = x - x1
    wy = y - y1
    
    # Bilinear interpolation
    result = (q11 * (1 - wx) * (1 - wy) +
              q21 * wx * (1 - wy) +
              q12 * (1 - wx) * wy +
              q22 * wx * wy)
    
    return int(result)

def scale_image(gray_pixels, src_width, src_height, dst_width, dst_height):
    """
    Scale image using bilinear interpolation.
    """
    scaled = []
    x_ratio = src_width / dst_width
    y_ratio = src_height / dst_height
    
    for dst_y in range(dst_height):
        for dst_x in range(dst_width):
            src_x = dst_x * x_ratio
            src_y = dst_y * y_ratio
            
            pixel = bilinear_interpolate(gray_pixels, src_width, src_height, src_x, src_y)
            scaled.append(pixel)
    
    return scaled

def convert_image_array(input_file, output_file, image_name, width, height, scale_to_width=None):
    """
    Read RGB565 image array from input file and write 4-bit grayscale to output file.
    Two pixels are packed into each uint8_t.
    
    Args:
        scale_to_width: If specified, scale image to this width (maintaining aspect ratio)
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
    
    # Convert to grayscale without opacity (original brightness)
    gray_values = []
    for hex_val in hex_values:
        rgb565 = int(hex_val, 16)
        gray4 = rgb565_to_gray4(rgb565, opacity=1.0)  # Full opacity = original image
        gray_values.append(gray4)
    
    # Scale if requested
    output_width = width
    output_height = height
    
    if scale_to_width and scale_to_width != width:
        # Calculate new height to maintain aspect ratio
        output_width = scale_to_width
        output_height = int(height * scale_to_width / width)
        
        print(f"  Scaling from {width}x{height} to {output_width}x{output_height}...")
        gray_values = scale_image(gray_values, width, height, output_width, output_height)
    
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
        if scale_to_width and scale_to_width != width:
            f.write(f"// Scaled to: {output_width}x{output_height} pixels\n")
        f.write(f"// Format: Two pixels packed per byte (high nibble, low nibble)\n")
        f.write(f"// Grayscale range: 0 (white) to 15 (black)\n\n")
        f.write(f"const uint16_t {image_name}_gray_width = {output_width};\n")
        f.write(f"const uint16_t {image_name}_gray_height = {output_height};\n\n")
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
    
    print(f"✓ Converted {image_name}: {len(gray_values)} pixels -> {len(packed_bytes)} bytes")
    return True

def convert_image_array_fullscreen(input_file, output_file, image_name, src_width, src_height, dst_width, dst_height, invert=False):
    """
    Read RGB565 image array and convert to 4-bit grayscale, scaling to exact dimensions.
    
    Args:
        invert: If True, invert the grayscale values
    """
    print(f"Converting {image_name} ({src_width}x{src_height} -> {dst_width}x{dst_height}){' [INVERTED]' if invert else ''}...")
    
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
    
    total_pixels = src_width * src_height
    if len(hex_values) != total_pixels:
        print(f"WARNING: Expected {total_pixels} pixels, found {len(hex_values)}")
    
    # Convert to grayscale without opacity (original brightness)
    gray_values = []
    for hex_val in hex_values:
        rgb565 = int(hex_val, 16)
        gray4 = rgb565_to_gray4(rgb565, opacity=1.0, invert=invert)  # Full opacity = original image
        gray_values.append(gray4)
    
    # Scale to target dimensions
    print(f"  Scaling from {src_width}x{src_height} to {dst_width}x{dst_height}...")
    gray_values = scale_image(gray_values, src_width, src_height, dst_width, dst_height)
    
    # Pack two 4-bit values into each byte
    packed_bytes = []
    for i in range(0, len(gray_values), 2):
        if i + 1 < len(gray_values):
            packed = (gray_values[i] << 4) | gray_values[i + 1]
        else:
            packed = gray_values[i] << 4
        packed_bytes.append(packed)
    
    # Write output file
    with open(output_file, 'w') as f:
        f.write(f"// {image_name} converted to 4-bit grayscale for M5Paper e-ink\n")
        f.write(f"// Original size: {src_width}x{src_height} pixels\n")
        f.write(f"// Scaled to: {dst_width}x{dst_height} pixels (full screen)\n")
        f.write(f"// Format: Two pixels packed per byte (high nibble, low nibble)\n")
        f.write(f"// Grayscale range: 0 (white) to 15 (black)\n\n")
        f.write(f"const uint16_t {image_name}_gray_width = {dst_width};\n")
        f.write(f"const uint16_t {image_name}_gray_height = {dst_height};\n\n")
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
    
    print(f"✓ Converted {image_name}: {len(gray_values)} pixels -> {len(packed_bytes)} bytes")
    return True


def main():
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    input_file = os.path.join(project_root, 'src/media/images_536_240.h')
    output_file = os.path.join(project_root, 'src/media/images_m5paper_gray.h')
    
    # M5Paper portrait mode - images fit width only (540x240)
    PORTRAIT_WIDTH = 540
    PORTRAIT_HEIGHT = 240
    
    # Splash screen in landscape mode (960x540)
    LANDSCAPE_WIDTH = 960
    LANDSCAPE_HEIGHT = 540
    
    print("M5Paper Grayscale Image Converter")
    print("=" * 50)
    print(f"Portrait images: {PORTRAIT_WIDTH}x{PORTRAIT_HEIGHT} pixels")
    print(f"Landscape splash: {LANDSCAPE_WIDTH}x{LANDSCAPE_HEIGHT} pixels")
    print("=" * 50)
    
    # Images to convert - splash is landscape, others are portrait
    # Last parameter: invert=True for images that need grayscale inversion
    images = [
        ('initScreen', 536, 240, LANDSCAPE_WIDTH, LANDSCAPE_HEIGHT, True),  # Landscape splash - INVERTED
        ('MinerScreen', 536, 240, PORTRAIT_WIDTH, PORTRAIT_HEIGHT, True),  # INVERTED
        ('minerClockScreen', 536, 240, PORTRAIT_WIDTH, PORTRAIT_HEIGHT, False),
        ('globalHashScreen', 536, 240, PORTRAIT_WIDTH, PORTRAIT_HEIGHT, False),
        ('setupModeScreen', 536, 240, PORTRAIT_WIDTH, PORTRAIT_HEIGHT, False),
    ]
    
    # Create header file
    with open(output_file, 'w') as f:
        f.write("// M5Paper Grayscale Images\n")
        f.write("// Auto-generated from images_536_240.h\n")
        f.write("// Format: 4-bit grayscale, two pixels per byte\n")
        f.write(f"// Portrait mode: {PORTRAIT_WIDTH}x{PORTRAIT_HEIGHT} pixels\n")
        f.write(f"// Splash screen (landscape): {LANDSCAPE_WIDTH}x{LANDSCAPE_HEIGHT} pixels\n\n")
        f.write("#ifndef IMAGES_M5PAPER_GRAY_H\n")
        f.write("#define IMAGES_M5PAPER_GRAY_H\n\n")
        f.write("#include <Arduino.h>\n\n")
    
    success_count = 0
    for img_name, src_width, src_height, dst_width, dst_height, invert in images:
        # Append each image to the output file
        with open(output_file, 'a') as f:
            pass  # File already open in append mode below
        
        if convert_image_array_fullscreen(input_file, output_file + '.tmp', img_name, 
                                          src_width, src_height, dst_width, dst_height, invert):
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
    print(f"✓ Splash screen: {LANDSCAPE_WIDTH}x{LANDSCAPE_HEIGHT}px (landscape)")
    print(f"✓ Other screens: {PORTRAIT_WIDTH}x{PORTRAIT_HEIGHT}px (portrait, text below)")

if __name__ == '__main__':
    main()
