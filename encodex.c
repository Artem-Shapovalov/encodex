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

/** \brief The seed value for the pseudo-random generator */
static uint32_t random_seed = 0xc0ffee;

/** \brief Initializes the pseudo-random generator with a new seed.
 *  \param seed The value for the new seed. */
static void prnd_init(uint32_t seed)
{
	random_seed = seed;
}

/** \brief Pseudo-random generator.
 *  \return Pseudo-random value. */
static uint32_t prnd(void)
{
	random_seed ^= random_seed << 13;
	random_seed ^= random_seed >> 17;
	random_seed ^= random_seed <<  4;

	return random_seed;
}

/** \brief Sets the bit value to requested inside the 32-bit value.
 *  \param tgt Valid pointer to the value, where the bit should obtain a new
 *             value.
 *  \param val The value of the bit. If equals to 0, the requested bit would
 *             be set to 0, otherwise to 1.
 *  \param idx Number of the bit to set the value. Should not be greater than
 *             31. */
static void set_bit(uint32_t* tgt, uint8_t val, size_t idx)
{
	if (val != 0u)
	{
		*tgt |= (1u << idx);
	}
	else
	{
		*tgt &= ~(1u << idx);
	}
}

/** \brief Returns the value of the requested bit.
 *  \param tgt The word to get the bit value.
 *  \param idx Number of the bit to get the value. Should not be greater than
 *             31.
 *  \return The value of the requested bit in the word. May be 1 or 0. */
static uint8_t get_bit(uint32_t tgt, size_t idx)
{
	return ((tgt & ((uint32_t)1u << (idx & 0x1fu))) != 0u) ? 1u : 0u;
}

/** \brief Returns the XOR operation over two bits.
 *  \param b1 The value of the first bit. If equals 0 it would be recognized as
 *            0, otherwise as 1.
 *  \param b2 The value of the second bit. If equals 0 it would be recornized
 *            as 0, otherwise as 1.
 *  \return The result of the XOR operation. May be 1 or 0. */
static uint8_t xor_bit(uint8_t b1, uint8_t b2)
{
	uint8_t _b1;
	uint8_t _b2;

	_b1 = (b1 != 0u) ? 1 : 0;
	_b2 = (b2 != 0u) ? 1 : 0;
	return (!(_b1 == _b2)) ? 1 : 0;
}

/** \brief Restores the value that was before XOR operation with left shift.
 *  \param val The word to restore.
 *  \param shift Value of the previous shift. Should not be greater than 31. */
static uint32_t revert_lshift(uint32_t val, size_t shift)
{
	uint32_t res;
	register size_t idx;

	res = 0;

	for (idx = 0; idx < shift; idx++)
	{
		set_bit(&res, get_bit(val, idx), idx);
	}

	for (idx = shift; idx < sizeof(uint32_t) * 8u; idx++)
	{
		set_bit(&res,
			xor_bit(
				get_bit(res, idx - shift),
				get_bit(val, idx)),
			idx);
	}

	return res;
}

/** \brief Restores the value that was before XOR operation with right shift.
 *  \param val The word to restore.
 *  \param shift Value of the previous shift. Should not be greater than 31. */
static uint32_t revert_rshift(uint32_t val, size_t shift)
{
	uint32_t res;
	register size_t idx;

	res = 0;

	for (idx = (sizeof(uint32_t) * 8u) - shift;
			idx < sizeof(uint32_t) * 8u; idx++)
	{
		set_bit(&res, get_bit(val, idx), idx);
	}

	for (idx = 0; idx < (sizeof(uint32_t) * 8u) - shift; idx++)
	{
		set_bit(&res,
			xor_bit(
				get_bit(res, idx + shift),
				get_bit(val, idx)),
			idx);
	}

	return res;
}

/** \brief Restores the previous value in the pseudo-random sequence.
 *  \param current Current pseudo-random value.
 *  \return The previous pseudo-random value. */
static uint32_t prnd_prev(uint32_t current)
{
	uint32_t _current;

	_current = current;
	_current = revert_lshift(_current,  4u);
	_current = revert_rshift(_current, 17u);
	_current = revert_lshift(_current, 13u);

	return _current;
}

/** \brief Cyclic rotation to left for each byte in a block with a values
 *         given by the key.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void rol_block(uint8_t* block, const uint8_t* key)
{
	register size_t idx;

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		register uint8_t shift;
		shift = key[idx] % 8u;
		block[idx] = 
			(0xffu & (block[idx] << shift)) |
			(0xffu & (block[idx] >> (8u - shift)));
	}
}

/** \brief Adding the key values to the block with overflow.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void add_key(uint8_t* block, const uint8_t* key)
{
	register size_t idx;

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		register uint8_t d;
		d = block[idx];
		d += key[idx];
		block[idx] = d;
	}
}

/** \brief Convolutes the key to the uint32_t number
 *  \param key Valid pointer to the key. The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTESl This memory should be initialized
 *             with the encryption key.
 *  \return The consoluted key value. */
static uint32_t convolute(const uint8_t* key)
{
	register size_t idx;
	uint32_t seed;
	uint8_t* seed_bytes;

	seed_bytes = (uint8_t*)&seed;
	seed = 0;

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		seed_bytes[idx % sizeof(uint32_t)] ^= key[idx];
	}

	if (seed == 0u)
	{
		seed = 0xc0ffee;
	}

	return seed;
}

/** \brief Performs the XOR operation with each byte of memory block and
 *         pseudo-random values.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void noize(uint8_t* block, const uint8_t* key)
{
	register size_t idx;

	prnd_init(convolute(key));

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		block[idx] ^= prnd() % 256u;
	}
}

/** \brief Change the order of the bytes in a memory block according the key.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void shuffle(uint8_t* block, const uint8_t* key)
{
	register size_t idx1;

	for (idx1 = 0; idx1 < ENCODEX_BLOCK_SIZE_BYTES; idx1++)
	{
		register size_t idx2;
		register uint8_t buf;

		idx2 = key[idx1] % ENCODEX_KEY_SIZE_BYTES;
		buf = block[idx2];
		block[idx2] = block[idx1];
		block[idx1] = buf;
	}
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void encodex(uint8_t* block, const uint8_t* key)
{
	rol_block(block, key);
	add_key  (block, key);
	noize    (block, key);
	shuffle  (block, key);
}

/** \brief Reverts the rol_block function call.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void revert_rol_block(uint8_t* block, const uint8_t* key)
{
	register size_t idx;

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		register uint8_t shift;

		shift = key[idx] % 8u;
		block[idx] = 
			(block[idx] >> shift) |
			(block[idx] << (8u - shift));
	}
}

/** \brief Reverts the add_key function call.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void revert_add_key(uint8_t* block, const uint8_t* key)
{
	register size_t idx;

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		register uint8_t d;

		d = block[idx];
		d -= key[idx];
		block[idx] = d;
	}
}

/** \brief Reverts the noize function call.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void revert_noize(uint8_t* block, const uint8_t* key)
{
	register size_t idx;
	uint32_t seed;

	prnd_init(convolute(key));
	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		seed = prnd();
	}

	for (idx = 0; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		block[ENCODEX_BLOCK_SIZE_BYTES - 1u - idx] ^= seed % 256u;
		seed = prnd_prev(seed);
	}
}

/** \brief Reverts the shuffle function call.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               modified and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. This memory should be correctly
 *             initialized with a key. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES. */
static void revert_shuffle(uint8_t* block, const uint8_t* key)
{
	register size_t idx1;

	for (idx1 = 0; idx1 < ENCODEX_BLOCK_SIZE_BYTES; idx1++)
	{
		register size_t idx2;
		register uint8_t buf;

		idx2 = key[ENCODEX_BLOCK_SIZE_BYTES - 1u - idx1]
			% ENCODEX_KEY_SIZE_BYTES;
		buf = block[idx2];
		block[idx2] = block[ENCODEX_BLOCK_SIZE_BYTES - 1u - idx1];
		block[ENCODEX_BLOCK_SIZE_BYTES - 1u - idx1] = buf;
	}
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void decodex(uint8_t* block, const uint8_t* key)
{
	revert_shuffle  (block, key);
	revert_noize    (block, key);
	revert_add_key  (block, key);
	revert_rol_block(block, key);
}

/** \brief Cypher block-chaining algorithm. Updates the value of the given key
 *         according to the seed and returns the new seed value.
 *  \param key Valid pointer to the block of memory. This function overwrites
 *             the memory by this pointer. The size of the memory should be
 *             equal to the ENCODEX_KEY_SIZE_BYTES.
 *  \param seed The value for the pseudo-random key modification. Initially,
 *              it's a key convolution, but after that it should be result
 *              of previous function calls.
 *  \return The updated seed value. */
static uint32_t cbc(uint8_t* key, uint32_t seed)
{
	register size_t idx;
	
	prnd_init(seed);
	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] ^= prnd() % 256u;
	}

	return prnd_prev(prnd());
}

void encodex_cbc_stream_init(const uint8_t* key, uint32_t* seed)
{
	*seed = convolute(key);
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void encodex_cbc_stream(uint8_t* block, uint8_t* key, uint32_t* seed)
{
	*seed = cbc(key, *seed);
	encodex(block, key);
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void decodex_cbc_stream(uint8_t* block, uint8_t* key, uint32_t* seed)
{
	*seed = cbc(key, *seed);
	decodex(block, key);
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void encodex_cbc(uint8_t* blocks, size_t blocks_num, const uint8_t* key)
{
	register size_t idx;
	uint32_t seed;
	uint8_t _key[ENCODEX_KEY_SIZE_BYTES];

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		_key[idx] = key[idx];
	}

	encodex_cbc_stream_init(_key, &seed);

	for (idx = 0; idx < blocks_num; idx++)
	{
		encodex_cbc_stream(&blocks[idx * ENCODEX_BLOCK_SIZE_BYTES],
				_key, &seed);
	}
}

/* cppcheck-suppress unusedFunction */
/* cppcheck-suppress misra-c2012-8.7 */
void decodex_cbc(uint8_t* blocks, size_t blocks_num, const uint8_t* key)
{
	register size_t idx;
	uint32_t seed;
	uint8_t _key[ENCODEX_KEY_SIZE_BYTES];

	for (idx = 0; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		_key[idx] = key[idx];
	}

	encodex_cbc_stream_init(_key, &seed);

	for (idx = 0; idx < blocks_num; idx++)
	{
		decodex_cbc_stream(&blocks[idx * ENCODEX_BLOCK_SIZE_BYTES],
				_key, &seed);
	}
}
