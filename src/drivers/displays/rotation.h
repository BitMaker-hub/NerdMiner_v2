//
// Rotation can be 0, 90, 180 or 270 degrees, but TFT_eSPI cryptically
// identifies these as 0, 1, 2, and 3.  ?!?!?!?!?!?
//
// Created by dwight on 3/14/24.
//

#ifndef NERDMINER_V2_ROTATION_H

// Rotation value 0 sets the display to a portrait (tall) mode, with the USB jack at the top right.
#define ROTATION_0      0
#define PORTRAIT        ROTATION_0

// Rotation 1 is landscape (wide) mode, with the USB jack at the bottom right.
#define ROTATION_90     1
#define LANDSCAPE       ROTATION_90

// Rotation value 2 is also a portrait mode, with the USB jack at the bottom left.
#define ROTATION_180        2
#define PORTRAIT_INVERTED   ROTATION_180

// Rotation 3 is also landscape, but with the USB jack at the top left.
#define ROTATION_270        3
#define LANDSCAPE_INVERTED  ROTATION_270

// Helper function to rotate display left (-90 deg)
int rotationLeft(int d) {
    return (d-ROTATION_90) % 4;
}

// Helper function to rotate display right (90 deg)
int rotationRight(int d) {
    return (d+ROTATION_90) % 4;
}

// Helper function to rotate display 180 deg
int flipRotation(int d) {
    return (d+ROTATION_180) % 4;
}

#define NERDMINER_V2_ROTATION_H
#endif //NERDMINER_V2_ROTATION_H
