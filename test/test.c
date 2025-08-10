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

#include "encodex.c"

#include <stdio.h>

static void random_check(void)
{
	size_t i;
	uint32_t values[10];
	uint32_t tmp;

	printf("\nPseudo-random number generator\n");

	for (i = 0; i < 10u; i++)
	{
		values[i] = prnd();
	}

	tmp = prnd();

	for (i = 0; i < 10u; i++)
	{
		tmp = prnd_prev(tmp);

		printf("	%08x == %08x\n", values[9 - i], tmp);
		if (values[9u - i] != tmp)
		{
			printf("	fail\n");
			return;
		}

	}
	
	tmp = prnd_prev(tmp);
	printf("	Initial default seed: %08x\n", tmp); 
	if (tmp != 0xc0ffee)
	{
		printf("	fail\n");
		return;
	}

	printf("	OK\n");
}

static void print_block(const uint8_t* mem, const char* name)
{
	size_t idx;

	printf("	%s:	", name);
	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		printf("%02x", mem[idx]);
	}
	printf("\n");

}

static int compare(const uint8_t* mem, const uint8_t* exp)
{
	size_t idx;
	int res;

	res = 0;

	print_block(mem, "Actual");
	print_block(exp, "Expect");

	printf("		");
	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		printf("%s", mem[idx] == exp[idx] ? "  " : "^^");
		if (mem[idx] != exp[idx])
		{
			res = 1;
		}
	}
	printf("\n");

	return res;
}

static void rol_check(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES];

	printf("\nCyclic shift (roll) for each byte of block\n");

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 5;
		mem[idx] = 0x01;
		exp[idx] = mem[idx];
	}

	rol_block(mem, key);
	print_block(mem, "Memory");
	revert_rol_block(mem, key);

	printf("	%s\n", compare(mem, exp) == 0 ? "OK" : "fail");
}

static void add_check_key(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES];

	printf("\nAdding key with overflow check\n");

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 0xff;
		mem[idx] = 0x01;
		exp[idx] = mem[idx];
	}

	add_key(mem, key);
	print_block(mem, "Memory");
	revert_add_key(mem, key);

	printf("	%s\n", compare(mem, exp) == 0 ? "OK" : "fail");
}

static void noize_denoize_check(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES];

	printf("\nReverting random noize check\n");
	
	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 0xff;
		mem[idx] = 0x01;
		exp[idx] = mem[idx];
	}

	noize(mem, key);
	print_block(mem, "Memory");
	revert_noize(mem, key);

	printf("	%s\n", compare(mem, exp) == 0 ? "OK" : "fail");
}

static void shuffle_check(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES];

	printf("\nReverting shuffle check\n");
	
	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 0xff & (0x01 + idx * 3);
		mem[idx] = idx;
		exp[idx] = mem[idx];
	}

	shuffle(mem, key);
	print_block(mem, "Memory");
	revert_shuffle(mem, key);

	printf("	%s\n", compare(mem, exp) == 0 ? "OK" : "fail");
}

static void encodex_check(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES];

	printf("\nENCODEX check\n");
	
	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 0xff & (0x01 + idx * 3);
		mem[idx] = idx;
		exp[idx] = mem[idx];
	}

	encodex(mem, key);
	print_block(mem, "Memory");
	decodex(mem, key);

	printf("	%s\n", compare(mem, exp) == 0 ? "OK" : "fail");
}

static void encodex_cbc_check(void)
{
	size_t idx;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint8_t mem[ENCODEX_BLOCK_SIZE_BYTES * 10];
	uint8_t exp[ENCODEX_BLOCK_SIZE_BYTES * 10];
	size_t counter;

	printf("\nENCODEX CBC check\n");

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = 0xff & (0x01 + idx * 3);
	}

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES * 10; idx++)
	{
		mem[idx] = idx % 256;
		exp[idx] = mem[idx];
	}

	encodex_cbc(mem, 10, key);
	decodex_cbc(mem, 10, key);

	counter = 0;
	for (idx = 0; idx < 10; idx++)
	{
		counter += compare(
			mem + idx * ENCODEX_BLOCK_SIZE_BYTES,
			exp + idx * ENCODEX_BLOCK_SIZE_BYTES);
	}

	printf("	%s\n", counter == 0 ? "OK" : "fail");
}

int main(int argc, char** argv)
{
	printf("== Encodex tests ==\n");

	random_check();
	rol_check();
	add_check_key();
	noize_denoize_check();
	shuffle_check();
	encodex_check();
	encodex_cbc_check();

	return 0;
}
