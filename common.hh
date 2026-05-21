/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include "prime_field.hh"
#include "mersenne_horner_check.hh"
#include "cauchy_prime_field_erasure_coding.hh"

typedef CODE::PrimeField<uint32_t, 0x7FFFFFFF> M31;

static void pack_m31(M31 *dst, std::ifstream &src, int count, int64_t bytes)
{
	uint64_t acc = 0;
	int64_t pos = 0;
	for (int i = 0, k = 0; i < count; i++) {
		for (; k < 31 && pos < bytes; pos++, k += 8)
			acc |= uint64_t(src.get()) << k;
		dst[i] = M31(acc & 0x7FFFFFFF);
		acc >>= 31;
		k -= 31;
	}
}

