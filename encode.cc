/*
Cauchy Mersenne Prime Field Erasure Coding

Copyright 2026 Ahmet Inan <inan@aicodix.de>
*/

#include "common.hh"

static void unpack_m31(std::ofstream &dst, const M31 *src, int count)
{
	uint64_t acc = 0;
	int num = 0;
	for (int i = 0; i < count; i++) {
		acc |= uint64_t(src[i].v) << num;
		num += 31;
		for (; num >= 8; acc >>= 8, num -= 8)
			dst.put(acc & 255);
	}
	if (num)
		dst.put(acc & 255);
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

static M31 encode_m31(M31 *dst, std::ifstream &src, int64_t bytes)
{
	int count = (bytes * 8 + 30) / 31;
	pack_m31(dst, src, count, bytes);
	M31 sub = find_unused_m31(dst, count);
	for (int i = 0; i < count; ++i)
		if (dst[i].v == 0x7FFFFFFF)
			dst[i] = sub;
	return sub;
}

static long long parse_bytes(const char *str)
{
	char *end;
	long long bytes = std::strtoll(str, &end, 10);
	switch (*end) {
	case 'k':
	case 'K':
		return bytes << 10;
	case 'm':
	case 'M':
		return bytes << 20;
	case 'g':
	case 'G':
		return bytes << 30;
	}
	return bytes;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::cerr << "usage: " << argv[0] << " INPUT SIZE CHUNKS.." << std::endl;
		return 1;
	}
	int chunk_count = argc - 3;
	if (chunk_count > 16384) {
		std::cerr << "Maximum of 16384 chunks are supported." << std::endl;
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
	long long chunk_bytes = parse_bytes(argv[2]);
	long long cme_overhead = 3 + 3 + 4 + 4; // CME (IDENT+SPLITS) SIZE HASH
	long long avail_bytes = chunk_bytes - cme_overhead;
	long long avail_values = (avail_bytes * 8) / 31;
	long long avail_bits = avail_values * 31LL;
	int block_count = (31 + input_bytes * 8 + avail_bits - 1) / avail_bits;
	if (avail_values < 1 || block_count > 1024) {
		std::cerr << "Size of chunks too small." << std::endl;
		return 1;
	}
	if (chunk_count < block_count) {
		std::cerr << "Need at least " << block_count << " chunks." << std::endl;
		return 1;
	}
	std::cerr << "CME(" << chunk_count << ", " << block_count << ")" << std::endl;
	std::ifstream input_file(input_name, std::ios::binary);
	if (input_file.bad()) {
		std::cerr << "Couldn't open file \"" << input_name << "\" for reading." << std::endl;
		return 1;
	}
	int block_values = (31 + input_bytes * 8 + block_count * 31 - 1) / (block_count * 31);
	int total_values = block_values * block_count;
	M31 *input_values = new M31[total_values];
	M31 sub = encode_m31(input_values+1, input_file, input_bytes);
	input_values[0] = sub;
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
		int32_t ident_splits = (block_count - 1) | (i << 10);
		chunk_file.write(reinterpret_cast<char *>(&ident_splits), 3);
		uint32_t size = input_bytes - 1;
		chunk_file.write(reinterpret_cast<char *>(&size), 4);
		uint32_t hash = mhc()();
		chunk_file.write(reinterpret_cast<char *>(&hash), 4);
		unpack_m31(chunk_file, chunk_values, block_values);
	}
	delete[] input_values;
	delete[] chunk_values;
	return 0;
}

