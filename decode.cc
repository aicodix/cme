/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#include <set>
#include "common.hh"

int main(int argc, char **argv)
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " OUTPUT CHUNKS.." << std::endl;
		return 1;
	}
	int chunk_count = argc - 2;
	int *chunk_ident = nullptr;
	M31 *chunk_values = nullptr;
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
		int32_t ident_splits = 0;
		chunk_file.read(reinterpret_cast<char *>(&ident_splits), 3);
		uint32_t size;
		chunk_file.read(reinterpret_cast<char *>(&size), 4);
		uint32_t hash;
		chunk_file.read(reinterpret_cast<char *>(&hash), 4);
		int splits = ident_splits & 1023;
		int ident = ident_splits >> 10;
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
		} else if (block_count != splits + 1 || output_bytes != size + 1LL || hash_value != hash) {
			std::cerr << "Skipping file \"" << chunk_name << "\"." << std::endl;
			continue;
		}
		list.insert(ident);
		chunk_ident[block_index] = ident + block_count;
		pack_m31(chunk_values + block_values * i, chunk_file, block_values, block_bytes);
		if (++block_index >= block_count)
			break;
	}
	if (block_index != block_count) {
		std::cerr << "Need " << block_count << " valid chunks but only got " << block_index << "." << std::endl;
		return 1;
	}
	const char *output_name = argv[1];
	if (output_name[0] == '-' && output_name[1] == 0)
		output_name = "/dev/stdout";
	std::ofstream output_file(output_name, std::ios::binary | std::ios::trunc);
	if (output_file.bad()) {
		std::cerr << "Couldn't open file \"" << output_name << "\" for writing." << std::endl;
		return 1;
	}
	CODE::CauchyPrimeFieldErasureCoding<M31> cme;
	CODE::MersenneHornerCheck mhc;
	M31 sub;
	int num = 0;
	uint64_t acc = 0;
	int64_t pos = 0;
	M31 *output_values = new M31[block_values];
	for (int i = 0; i < block_count; ++i) {
		cme.decode(output_values, chunk_values, chunk_ident, i, block_values, block_count);
		for (int i = 0; i < block_values; ++i)
			mhc(output_values[i]);
		if (!i)
			sub = output_values[0];
		for (int o = !i; o < block_values; o++) {
			uint64_t val = sub == output_values[o] ? 0x7FFFFFFF : output_values[o]();
			acc |= val << num;
			num += 31;
			for (; num >= 8 && pos < output_bytes; pos++, acc >>= 8, num -= 8)
				output_file.put(acc & 255);
		}
	}
	delete[] output_values;
	delete[] chunk_ident;
	delete[] chunk_values;
	if (mhc()() != hash_value) {
		std::cerr << "hash value does not match!" << std::endl;
		return 1;
	}
	return 0;
}

