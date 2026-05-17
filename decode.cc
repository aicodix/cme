/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#include <set>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include "prime_field.hh"
#include "mersenne_packing.hh"
#include "mersenne_horner_check.hh"
#include "cauchy_prime_field_erasure_coding.hh"

int main(int argc, char **argv)
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " OUTPUT CHUNKS.." << std::endl;
		return 1;
	}
	int chunk_count = argc - 2;
	typedef CODE::PrimeField<uint32_t, 0x7FFFFFFF> M31;
	int *chunk_ident = nullptr;
	M31 *chunk_values = nullptr;
	uint8_t *chunk_data = nullptr;
	int block_values = 0;
	int block_count = 0;
	int block_index = 0;
	int total_values = 0;
	long long block_bytes = 0;
	long long output_bytes = 0;
	uint32_t hash_value = 0;
	bool first = true;
	std::set<uint16_t> list;
	for (int i = 0; i < chunk_count; ++i) {
		const char *chunk_name = argv[2+i];
		std::ifstream chunk_file(chunk_name, std::ios::binary);
		if (chunk_file.bad()) {
			std::cerr << "Couldn't open file \"" << chunk_name << "\" for reading." << std::endl;
			return 1;
		}
		char magic[3];
		chunk_file.read(magic, 3);
		uint8_t splits;
		chunk_file.read(reinterpret_cast<char *>(&splits), 1);
		uint16_t ident;
		chunk_file.read(reinterpret_cast<char *>(&ident), 2);
		uint32_t size;
		chunk_file.read(reinterpret_cast<char *>(&size), 4);
		uint32_t hash;
		chunk_file.read(reinterpret_cast<char *>(&hash), 4);
		if (!chunk_file || magic[0] != 'C' || magic[1] != 'M' || magic[2] != 'E' || list.count(ident)) {
			std::cerr << "Skipping file \"" << chunk_name << "\"." << std::endl;
			continue;
		}
		if (first) {
			first = false;
			block_count = splits + 1;
			output_bytes = size + 1LL;
			block_values = (31 + output_bytes * 8 + block_count * 31 - 1) / (block_count * 31);
			block_bytes = (block_values * 31LL + 7) / 8;
			total_values = block_values * block_count;
			hash_value = hash;
			chunk_ident = new int[block_count];
			chunk_values = new M31[total_values];
			chunk_data = new uint8_t[block_bytes];
		} else if (block_count != splits + 1 || output_bytes != size + 1LL || hash_value != hash) {
			std::cerr << "Skipping file \"" << chunk_name << "\"." << std::endl;
			continue;
		}
		list.insert(ident);
		chunk_ident[block_index] = ident + block_count;
		chunk_file.read(reinterpret_cast<char *>(chunk_data), block_bytes);
		CODE::MersennePacking::pack(chunk_values + block_values * i, chunk_data, block_values, block_bytes);
		if (++block_index >= block_count)
			break;
	}
	if (block_index != block_count) {
		std::cerr << "Need " << block_count << " valid chunks but only got " << block_index << "." << std::endl;
		return 1;
	}
	M31 *output_values = new M31[total_values];
	CODE::CauchyPrimeFieldErasureCoding<M31> cme;
	for (int i = 0; i < block_count; ++i)
		cme.decode(output_values + block_values * i, chunk_values, chunk_ident, i, block_values, block_count);
	delete[] chunk_ident;
	delete[] chunk_data;
	CODE::MersenneHornerCheck mhc;
	for (int i = 0; i < total_values; ++i)
		mhc(output_values[i]);
	if (mhc()() != hash_value) {
		std::cerr << "hash value does not match!" << std::endl;
		return 1;
	}
	uint8_t *output_data = new uint8_t[output_bytes];
	CODE::MersennePacking::decode(output_data, output_values, output_bytes);
	delete[] output_values;
	const char *output_name = argv[1];
	if (output_name[0] == '-' && output_name[1] == 0)
		output_name = "/dev/stdout";
	std::ofstream output_file(output_name, std::ios::binary | std::ios::trunc);
	if (output_file.bad()) {
		std::cerr << "Couldn't open file \"" << output_name << "\" for writing." << std::endl;
		return 1;
	}
	output_file.write(reinterpret_cast<char *>(output_data), output_bytes);
	delete[] output_data;
	return 0;
}

