# Host unit test for checkValid() — no Arduino/ESP32 toolchain needed.
#
# checkValid() and reverse_bytes() have no Arduino dependency (the only
# Serial use is inside #ifdef DEBUG_MINING), so we extract exactly those two
# functions from src/utils.cpp at build time. That keeps ONE source of truth:
# the test always exercises the shipped implementation, and no production file
# has to be restructured for testability.
#
#   make        -> build and run
#   make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -Wall -Wextra -g -fsanitize=address,undefined
SRC       = ../../src/utils.cpp

all: run

extracted.cpp: $(SRC)
	@printf '// AUTO-GENERATED from src/utils.cpp — do not edit.\n' > $@
	@printf '#include <stdint.h>\n#include <stddef.h>\n#include <string.h>\n\n' >> $@
	@sed -n '/^void reverse_bytes/,/^}/p' $(SRC) >> $@
	@printf '\n' >> $@
	@sed -n '/^bool checkValid/,/^}/p' $(SRC) >> $@
	@grep -c '^bool checkValid' $@ >/dev/null || (echo "checkValid cikarilamadi" && false)

test_checkValid: test_checkValid.cpp extracted.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

run: test_checkValid
	./test_checkValid

clean:
	rm -f test_checkValid extracted.cpp

.PHONY: all run clean
