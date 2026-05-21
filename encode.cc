/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#include "common.hh"

static void unpack_m31(std::ofstream &dst, const M31 *src, int count, int64_t bytes)
{
	uint64_t acc = 0;
	int64_t pos = 0;
	for (int i = 0, k = 0; i < count; i++) {
		acc |= uint64_t(src[i].v) << k;
		k += 31;
		for (; (k >= 8 || i == count - 1) && pos < bytes; pos++, acc >>= 8, k -= 8)
			dst.put(acc & 255);
	}
}

static M31 find_unused_m31(const M31 *dst, int count)
{
	typedef unsigned used_word;
	const int used_width = 8 * sizeof(used_word);
	const int used_length = count / used_width + 1;
	used_word *used_values = new used_word[used_length];
	for (int i = 0; i < used_length; ++i)
		used_values[i] = 0;
	for (int i = 0; i < count; ++i)
		if (int(dst[i].v)/used_width < used_length)
			used_values[dst[i].v/used_width] |= 1 << dst[i].v%used_width;
	int s = 0;
	while (s/used_width < used_length && used_values[s/used_width] & 1 << s%used_width)
		++s;
	delete[] used_values;
	return M31(s);
}

static void encode_m31(M31 *dst, std::ifstream &src, int64_t bytes)
{
	int count = (bytes * 8 + 30) / 31;
	pack_m31(dst+1, src, count, bytes);
	M31 sub = find_unused_m31(dst+1, count);
	*dst++ = sub;
	for (int i = 0; i < count; ++i)
		if (dst[i].v == 0x7FFFFFFF)
			dst[i] = sub;
}

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
	M31 *input_values = new M31[total_values];
	encode_m31(input_values, input_file, input_bytes);
	for (int i = (31 + input_bytes * 8 + 30) / 31; i < total_values; ++i)
		input_values[i] = M31(0);
	CODE::MersenneHornerCheck mhc;
	for (int i = 0; i < total_values; ++i)
		mhc(input_values[i]);
	CODE::CauchyPrimeFieldErasureCoding<M31> cme;
	M31 *chunk_values = new M31[block_values];
	for (int i = 0; i < chunk_count; ++i) {
		int chunk_ident = block_count + i;
		cme.encode(input_values, chunk_values, chunk_ident, block_values, block_count);
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
		unpack_m31(chunk_file, chunk_values, block_values, block_bytes);
	}
	delete[] input_values;
	return 0;
}

