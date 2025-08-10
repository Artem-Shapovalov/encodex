/* Copyright © 2025 Artem Shapovalov <artem_shapovalov@aol.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of  this  software and associated documentation files  (the “Software”),  to
 * deal  in the Software without restriction, including without limitation  the
 * rights  to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell  copies of the Software, and to permit persons to whom the Software  is
 * furnished to do so, subject to the following conditions:
 * 
 * The  above copyright notice and this permission notice shall be included  in
 * all copies or substantial portions of the Software.
 * 
 * THE  SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS  OR
 * IMPLIED,  INCLUDING  BUT NOT LIMITED TO THE WARRANTIES  OF  MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL  THE
 * AUTHORS  OR  COPYRIGHT  HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES  OR  OTHER
 * LIABILITY,  WHETHER  IN AN ACTION OF CONTRACT, TORT  OR  OTHERWISE,  ARISING
 * FROM,  OUT  OF  OR  IN CONNECTION WITH THE SOFTWARE  OR  THE  USE  OR  OTHER
 * DEALINGS IN THE SOFTWARE. */

#include "encodex.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct cli_result
{
	int error;
	int help;
	int encode;
	int cbc;
	const char* ifile;
	const char* ofile;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
};

static struct cli_result cli(int argc, char** argv)
{
	struct cli_result res;
	register size_t idx;
	const char* key;

	uint8_t allow;

	allow = 1u;

	res.help = 0;
	res.error = 0;
	res.encode = 0;
	res.cbc = 0;
	res.ifile = NULL;
	res.ofile = NULL;

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		res.key[idx] = 0;
	}

	if ((argc > 1) && (strcmp("--help", argv[1]) == 0))
	{
		res.help = 1;
		allow = 0;
	}

	if ((allow == 1u) && (argc < 5))
	{
		res.error = 1;
		allow = 0;
	}

	if ((allow == 1u) && (argc > 6))
	{
		res.error = 2;
		allow = 0;
	}

	if (allow == 1u)
	{
		if (strcmp("encode", argv[1]) == 0)
		{
			res.encode = 1;
		}
		else if (strcmp("decode", argv[1]) == 0)
		{
			res.encode = 0;
		}
		else
		{
			res.error = 3;
			allow = 0;
		}
	}

	if (allow == 1u)
	{
		if (strcmp("cbc", argv[2]) == 0)
		{
			res.cbc = 1;
		}

		if (res.cbc != 0)
		{
			res.ifile = argv[3];
			res.ofile = argv[4];
		}
		else
		{
			res.ifile = argv[2];
			res.ofile = argv[3];
		}

		if (res.cbc != 0)
		{
			key = argv[5];
		}
		else
		{
			key = argv[4];
		}

		if (strlen(key) != (ENCODEX_KEY_SIZE_BYTES * 2u))
		{
			res.error = 4;
			allow = 0;
		}
	}

	if (allow == 1u)
	{
		uint8_t byte_tmp;
		for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
		{
			switch (key[idx * 2u])
			{
				case '0': byte_tmp = 0x00u; break;
				case '1': byte_tmp = 0x10u; break;
				case '2': byte_tmp = 0x20u; break;
				case '3': byte_tmp = 0x30u; break;
				case '4': byte_tmp = 0x40u; break;
				case '5': byte_tmp = 0x50u; break;
				case '6': byte_tmp = 0x60u; break;
				case '7': byte_tmp = 0x70u; break;
				case '8': byte_tmp = 0x80u; break;
				case '9': byte_tmp = 0x90u; break;
				case 'a': byte_tmp = 0xa0u; break;
				case 'b': byte_tmp = 0xb0u; break;
				case 'c': byte_tmp = 0xc0u; break;
				case 'd': byte_tmp = 0xd0u; break;
				case 'e': byte_tmp = 0xe0u; break;
				case 'f': byte_tmp = 0xf0u; break;

				default:
				res.error = 5;
				break;
			}

			switch (key[(idx * 2u) + 1u])
			{
				case '0': byte_tmp += 0x00u; break;
				case '1': byte_tmp += 0x01u; break;
				case '2': byte_tmp += 0x02u; break;
				case '3': byte_tmp += 0x03u; break;
				case '4': byte_tmp += 0x04u; break;
				case '5': byte_tmp += 0x05u; break;
				case '6': byte_tmp += 0x06u; break;
				case '7': byte_tmp += 0x07u; break;
				case '8': byte_tmp += 0x08u; break;
				case '9': byte_tmp += 0x09u; break;
				case 'a': byte_tmp += 0x0au; break;
				case 'b': byte_tmp += 0x0bu; break;
				case 'c': byte_tmp += 0x0cu; break;
				case 'd': byte_tmp += 0x0du; break;
				case 'e': byte_tmp += 0x0eu; break;
				case 'f': byte_tmp += 0x0fu; break;

				default:
				res.error = 5;
				break;
			}

			res.key[idx] = byte_tmp;
		}
	}

	return res;
}

static void print_help()
{
	(void)printf("ENCODEX demo application\n");
	(void)printf("Usage: encodex <command> [cbc] <ifile> <ofile> <key>\n");
	(void)printf("	command	- encode/decode\n");
	(void)printf("	cbc	- optional flag, use CBC algorithm\n");
	(void)printf("	ifile	- input file path\n");
	(void)printf("	ofile	- output file path\n");
	(void)printf("	key	- hexadecimal key, 64 characters [0-9a-f]\n");
}

static void print_error(int error)
{
	switch (error)
	{
		case 1: (void)printf("Too few arguments\n"); break;
		case 2: (void)printf("Too much arguments\n"); break;
		case 3: (void)printf("Unknown command\n"); break;
		case 4: (void)printf("Wrong key size\n"); break;
		case 5: (void)printf("Wrong key format\n"); break;
		default: (void)printf("Unknown error\n"); break;
	}
}

static size_t get_file_size(FILE* f)
{
	size_t current_pos;
	size_t size;

	current_pos = ftell(f);
	(void)fseek(f, 0, SEEK_END);
	size = ftell(f);
	(void)fseek(f, current_pos, SEEK_SET);

	return size;
}

static void write_block(const uint8_t* block, size_t offset, FILE* ofp)
{
	size_t idx;

	for (idx = offset; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		(void)fputc(block[idx], ofp);
	}
}

static void encode_file(FILE* ifp, FILE* ofp, const uint8_t* key)
{
	size_t idx;
	size_t file_size;
	const uint8_t* file_size_bytes;
	uint8_t block[ENCODEX_BLOCK_SIZE_BYTES];
	size_t block_counter;

	file_size = get_file_size(ifp);
	file_size_bytes = (uint8_t*)&file_size;
	for (idx = 0; idx < sizeof(size_t); idx++)
	{
		(void)fputc(file_size_bytes[idx], ofp);
	}

	block_counter = ENCODEX_BLOCK_SIZE_BYTES
		- (file_size % ENCODEX_BLOCK_SIZE_BYTES);

	for (idx = 0; idx < block_counter; idx++)
	{
		block[idx] = 0;
	}

	if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
	{
		block_counter = 0;
		encodex(block, key);
		write_block(block, 0, ofp);
	}

	for (idx = 0; idx < file_size; idx++)
	{
		block[block_counter] = fgetc(ifp);
		block_counter++;

		if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
		{
			block_counter = 0;
			encodex(block, key);
			write_block(block, 0, ofp);
		}
	}

}

static void decode_file(FILE* ifp, FILE* ofp, const uint8_t* key)
{
	size_t idx;
	size_t file_size;
	size_t skip_bytes;
	uint8_t* file_size_bytes;
	size_t ifile_size;
	uint8_t block[ENCODEX_BLOCK_SIZE_BYTES];
	size_t block_counter;

	file_size_bytes = (uint8_t*)&file_size;
	for (idx = 0; idx < sizeof(size_t); idx++)
	{
		file_size_bytes[idx] = fgetc(ifp);
	}

	skip_bytes = ENCODEX_BLOCK_SIZE_BYTES
		- (file_size % ENCODEX_BLOCK_SIZE_BYTES);

	block_counter = 0;
	ifile_size = get_file_size(ifp);
	for (idx = sizeof(size_t); idx < ifile_size; idx++)
	{
		block[block_counter] = fgetc(ifp);
		block_counter++;

		if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
		{
			block_counter = 0;

			decodex(block, key);
			write_block(block, skip_bytes, ofp);
			skip_bytes = 0;
		}
	}
}

static void encode_file_cbc(FILE* ifp, FILE* ofp, uint8_t* key)
{
	size_t idx;
	uint32_t seed;
	size_t file_size;
	const uint8_t* file_size_bytes;
	uint8_t block[ENCODEX_BLOCK_SIZE_BYTES];
	size_t block_counter;
	
	encodex_cbc_stream_init(key, &seed);

	file_size = get_file_size(ifp);
	file_size_bytes = (uint8_t*)&file_size;
	for (idx = 0; idx < sizeof(size_t); idx++)
	{
		(void)fputc(file_size_bytes[idx], ofp);
	}

	block_counter = ENCODEX_BLOCK_SIZE_BYTES
		- (file_size % ENCODEX_BLOCK_SIZE_BYTES);

	for (idx = 0; idx < block_counter; idx++)
	{
		block[idx] = 0;
	}

	if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
	{
		block_counter = 0;
		encodex_cbc_stream(block, key, &seed);
		write_block(block, 0, ofp);
	}

	for (idx = 0; idx < file_size; idx++)
	{
		block[block_counter] = fgetc(ifp);
		block_counter++;

		if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
		{
			block_counter = 0;
			encodex_cbc_stream(block, key, &seed);
			write_block(block, 0, ofp);
		}
	}
}

static void decode_file_cbc(FILE* ifp, FILE* ofp, uint8_t* key)
{
	size_t idx;
	size_t file_size;
	size_t skip_bytes;
	uint8_t* file_size_bytes;
	size_t ifile_size;
	uint8_t block[ENCODEX_BLOCK_SIZE_BYTES];
	size_t block_counter;
	uint32_t seed;

	encodex_cbc_stream_init(key, &seed);

	file_size_bytes = (uint8_t*)&file_size;
	for (idx = 0; idx < sizeof(size_t); idx++)
	{
		file_size_bytes[idx] = fgetc(ifp);
	}

	skip_bytes = ENCODEX_BLOCK_SIZE_BYTES
		- (file_size % ENCODEX_BLOCK_SIZE_BYTES);

	block_counter = 0;
	ifile_size = get_file_size(ifp);
	for (idx = sizeof(size_t); idx < ifile_size; idx++)
	{
		block[block_counter] = fgetc(ifp);
		block_counter++;

		if (block_counter >= ENCODEX_BLOCK_SIZE_BYTES)
		{
			block_counter = 0;

			decodex_cbc_stream(block, key, &seed);
			write_block(block, skip_bytes, ofp);
			skip_bytes = 0;
		}
	}
}

int main(int argc, char** argv)
{
	struct cli_result cr;
	FILE* ifp;
	FILE* ofp;

	int close_ifp;
	int close_ofp;

	uint8_t allow;
	int retval;

	close_ifp = 0;
	close_ofp = 0;

	allow = 1u;
	retval = 0;
	
	cr = cli(argc, argv);

	if (cr.error != 0)
	{
		print_error(cr.error);
		print_help();
		allow = 0;
		retval = -1;
	}

	if (allow == 1u)
	{
		if (cr.help != 0)
		{
			print_help();
			allow = 0;
			retval = 0;
		}
	}

	if (allow == 1u)
	{
		ifp = fopen(cr.ifile, "rb");
		if (ifp == NULL)
		{
			(void)printf("Can't open %s\n", cr.ifile);
			allow = 0;
			retval = -1;
		}
		else
		{
			close_ifp = 1;
		}
	}

	if (allow == 1u)
	{
		ofp = fopen(cr.ofile, "wb");
		if (ofp == NULL)
		{
			(void)printf("Can't open %s\n", cr.ofile);
			close_ifp = 1;
			allow = 0;
			retval = -1;
		}
		else
		{
			close_ofp = 1;
		}
	}

	if (allow == 1u)
	{
		if (cr.encode && !cr.cbc)
		{
			encode_file(ifp, ofp, cr.key);
		}
		else if (cr.encode && cr.cbc)
		{
			encode_file_cbc(ifp, ofp, cr.key);
		}
		else if (!cr.encode && !cr.cbc)
		{
			decode_file(ifp, ofp, cr.key);
		}
		else if (!cr.encode && cr.cbc)
		{
			decode_file_cbc(ifp, ofp, cr.key);
		}
		else
		{
		}
	}

	if (close_ifp != 0)
	{
		(void)fclose(ifp);
	}

	if (close_ofp != 0)
	{
		(void)fclose(ofp);
	}

	return retval;
}
