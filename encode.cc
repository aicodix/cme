/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include "prime_field.hh"
#include "mersenne_packing.hh"
#include "mersenne_horner_check.hh"
#include "cauchy_prime_field_erasure_coding.hh"

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::cerr << "usage: " << argv[0] << " INPUT SPLITS CHUNKS.." << std::endl;
		return 1;
	}
	const char *input_name = argv[1];
	struct stat sb;
	if (stat(input_name, &sb) < 0 || sb.st_size < 1) {
		std::cerr << "Couldn't get size of file \"" << input_name << "\"." << std::endl;
		return 1;
	}
	const long long MAX_SIZE = 1LL << 32;
	if (sb.st_size > MAX_SIZE) {
		std::cerr << "Size of file \"" << input_name << "\" too large." << std::endl;
		return 1;
	}
	long long input_bytes = sb.st_size;
	int split_count = std::atoi(argv[2]);
	if (split_count < 0 || split_count > 255) {
		std::cerr << "Number of splits must be between 0 and 255." << std::endl;
		return 1;
	}
	int chunk_count = argc - 3;
	int block_count = split_count + 1;
	if (chunk_count < block_count) {
		std::cerr << "Need at least " << block_count << " chunks." << std::endl;
		return 1;
	}
	if (chunk_count > 65536) {
		std::cerr << "Maximum of 65536 chunks are supported." << std::endl;
		return 1;
	}
	std::cerr << "CME(" << chunk_count << ", " << block_count << ")" << std::endl;
	std::ifstream input_file(input_name, std::ios::binary);
	if (input_file.bad()) {
		std::cerr << "Couldn't open file \"" << input_name << "\" for reading." << std::endl;
		return 1;
	}
	int block_values = (31 + input_bytes * 8 + block_count * 31 - 1) / (block_count * 31);
	long long block_bytes = (block_values * 31LL + 7) / 8;
	int total_values = block_values * block_count;
	uint8_t *input_data = new uint8_t[input_bytes];
	input_file.read(reinterpret_cast<char *>(input_data), input_bytes);
	typedef CODE::PrimeField<uint32_t, 0x7FFFFFFF> M31;
	M31 *input_values = new M31[total_values];
	auto remap = new CODE::MersenneRemapping<MAX_SIZE>();
	remap->encode(input_values, input_data, input_bytes);
	delete remap;
	delete[] input_data;
	for (int i = (31 + input_bytes * 8 + 30) / 31; i < total_values; ++i)
		input_values[i] = M31(0);
	CODE::MersenneHornerCheck mhc;
	for (int i = 0; i < total_values; ++i)
		mhc(input_values[i]);
	CODE::CauchyPrimeFieldErasureCoding<M31> cme;
	M31 *chunk_values = new M31[block_values];
	uint8_t *chunk_data = new uint8_t[block_bytes];
	for (int i = 0; i < chunk_count; ++i) {
		int chunk_ident = block_count + i;
		cme.encode(input_values, chunk_values, chunk_ident, block_values, block_count);
		CODE::MersennePacking::unpack(chunk_data, chunk_values, block_values, block_bytes);
		const char *chunk_name = argv[3+i];
		std::ofstream chunk_file(chunk_name, std::ios::binary | std::ios::trunc);
		if (chunk_file.bad()) {
			std::cerr << "Couldn't open file \"" << chunk_name << "\" for writing." << std::endl;
			return 1;
		}
		chunk_file.write("CME", 3);
		uint8_t splits = split_count;
		chunk_file.write(reinterpret_cast<char *>(&splits), 1);
		uint16_t ident = i;
		chunk_file.write(reinterpret_cast<char *>(&ident), 2);
		uint32_t size = input_bytes - 1;
		chunk_file.write(reinterpret_cast<char *>(&size), 4);
		uint32_t hash = mhc()();
		chunk_file.write(reinterpret_cast<char *>(&hash), 4);
		chunk_file.write(reinterpret_cast<char *>(chunk_data), block_bytes);
	}
	delete[] chunk_data;
	delete[] input_values;
	return 0;
}

