#pragma once

// Centralized I2C hash-cluster configuration.
// 
// How to use (code-only, without platformio.ini):
// - Set NM_I2C_HASH_ROLE to one of the values below and rebuild.
// - For slaves, also set NM_I2C_HASH_SLAVE_ADDRESS (each slave must have a unique address on the bus).
//
// Build-flag precedence:
// - If platformio.ini already defines I2C_HASH_MASTER/I2C_HASH_SLAVE (or pins/clock/address), those take precedence
//   over the NM_* values here.

#define NM_I2C_HASH_ROLE_OFF 0
#define NM_I2C_HASH_ROLE_MASTER 1
#define NM_I2C_HASH_ROLE_SLAVE 2

#ifndef NM_I2C_HASH_ROLE
// Default: normal (non-cluster) build.
#define NM_I2C_HASH_ROLE NM_I2C_HASH_ROLE_OFF
#endif

// Backwards compatibility: historical I2C_SLAVE define meant "I2C master feeding slaves".
#if defined(I2C_SLAVE) && !defined(I2C_HASH_MASTER)
#define I2C_HASH_MASTER 1
#endif

// If the role wasn't set by build flags, allow enabling it here.
#if !defined(I2C_HASH_MASTER) && !defined(I2C_HASH_SLAVE)
#if (NM_I2C_HASH_ROLE == NM_I2C_HASH_ROLE_MASTER)
#define I2C_HASH_MASTER 1
#elif (NM_I2C_HASH_ROLE == NM_I2C_HASH_ROLE_SLAVE)
#define I2C_HASH_SLAVE 1
#endif
#endif

#if defined(I2C_HASH_MASTER) && defined(I2C_HASH_SLAVE)
#error "Define only one: I2C_HASH_MASTER or I2C_HASH_SLAVE"
#endif

// Default I2C bus parameters (override per-board if needed).
#ifndef NM_I2C_HASH_BUS_PORT
#define NM_I2C_HASH_BUS_PORT 0
#endif
#ifndef NM_I2C_HASH_SDA_PIN
#define NM_I2C_HASH_SDA_PIN 21
#endif
#ifndef NM_I2C_HASH_SCL_PIN
#define NM_I2C_HASH_SCL_PIN 22
#endif
#ifndef NM_I2C_HASH_CLOCK_HZ
// 100kHz is the most tolerant setting across boards/wiring. You can bump to 400000 if stable.
#define NM_I2C_HASH_CLOCK_HZ 100000
#endif
#ifndef NM_I2C_HASH_SLAVE_ADDRESS
#define NM_I2C_HASH_SLAVE_ADDRESS 0x21
#endif

// Map NM_* defaults into the existing macros used by the code, but only if build_flags didn't already set them.
#ifndef I2C_HASH_BUS_PORT
#define I2C_HASH_BUS_PORT NM_I2C_HASH_BUS_PORT
#endif
#ifndef I2C_HASH_SDA_PIN
#define I2C_HASH_SDA_PIN NM_I2C_HASH_SDA_PIN
#endif
#ifndef I2C_HASH_SCL_PIN
#define I2C_HASH_SCL_PIN NM_I2C_HASH_SCL_PIN
#endif
#ifndef I2C_HASH_CLOCK_HZ
#define I2C_HASH_CLOCK_HZ NM_I2C_HASH_CLOCK_HZ
#endif
#ifndef I2C_HASH_SLAVE_ADDRESS
#define I2C_HASH_SLAVE_ADDRESS NM_I2C_HASH_SLAVE_ADDRESS
#endif

