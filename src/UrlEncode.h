#ifndef _PLAGEOJ_URLENCODE_H
#define _PLAGEOJ_URLENCODE_H

#include <Arduino.h>

/**
 * Percent-encodes a string.
 * @param msg UTF-8 string to encode.
 * @returns Percent-encoded string.
 */
String urlEncode(const char *msg);

/**
 * Percent-encodes a string.
 * @param msg UTF-8 string to encode.
 * @returns Percent-encoded string.
 */
String urlEncode(String msg);

#endif