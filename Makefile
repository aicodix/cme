
CXXFLAGS = -std=c++17 -W -Wall -O3 -ffast-math -fno-exceptions -fno-rtti -I../code
CXX = clang++ -stdlib=libc++ -march=native
#CXX = g++ -march=native

#CXX = armv7a-hardfloat-linux-gnueabi-g++ -static -mfpu=neon -march=armv7-a
#QEMU = qemu-arm

#CXX = aarch64-unknown-linux-gnu-g++ -static -march=armv8-a+crc+simd -mtune=cortex-a72
#QEMU = qemu-aarch64

CHUNKS := $(shell seq -f "chunk%03g.cme" 0 999)
ERASED := $(shell seq -f "chunk%03g.cme" 0 999 | sort -R | head -n 131)

.PHONY: all

all: encode decode

test: encode decode
	dd if=/dev/urandom of=input.dat bs=512 count=256
	$(QEMU) ./encode input.dat 1024 $(CHUNKS)
	$(QEMU) ./decode output.dat $(ERASED)
	diff -q -s input.dat output.dat
	rm input.dat output.dat $(CHUNKS)

encode: encode.cc common.hh
	$(CXX) $(CXXFLAGS) $< -o $@

decode: decode.cc common.hh
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f encode decode

