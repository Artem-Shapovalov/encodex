/* Known-plaintext regression probe for the single-block Encodex API.
 *
 * This probe tries the old byte-independent known-plaintext attack. It returns
 * failure if that attack still decrypts the target block.
 */

#include "encodex.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PAIRS 16u

static const uint8_t secret_key[ENCODEX_KEY_SIZE_BYTES] = {
	0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u,
	0x09u, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u,
	0x17u, 0x18u, 0x19u, 0x20u, 0x21u, 0x22u, 0x23u, 0x24u,
	0x25u, 0x26u, 0x27u, 0x28u, 0x29u, 0x30u, 0x31u, 0x32u
};

static uint8_t rol8(uint8_t value, unsigned int shift)
{
	shift &= 7u;
	return (uint8_t)(((unsigned int)value << shift) |
		((unsigned int)value >> ((8u - shift) & 7u)));
}

static uint8_t ror8(uint8_t value, unsigned int shift)
{
	shift &= 7u;
	return (uint8_t)(((unsigned int)value >> shift) |
		((unsigned int)value << ((8u - shift) & 7u)));
}

static uint8_t enc_byte(uint8_t plain, uint8_t key_byte, uint8_t noise)
{
	return (uint8_t)((uint8_t)(rol8(plain, key_byte) + key_byte) ^ noise);
}

static uint8_t dec_byte(uint8_t cipher, uint8_t key_byte, uint8_t noise)
{
	return ror8((uint8_t)((cipher ^ noise) - key_byte), key_byte);
}

static void print_block(const char* label, const uint8_t* block)
{
	size_t idx;

	(void)printf("%s", label);
	for (idx = 0u; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		(void)printf("%02x", block[idx]);
	}
	(void)printf("\n");
}

int main(void)
{
	uint8_t plain[PAIRS][ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t cipher[PAIRS][ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t source_for_output[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t rec_key[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t rec_noise[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t target_plain[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t target_cipher[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t recovered[ENCODEX_BLOCK_SIZE_BYTES];
	size_t pair;
	size_t src;
	size_t out;
	size_t candidate;
	size_t idx;

	(void)memset(source_for_output, 0xff, sizeof(source_for_output));
	(void)memset(rec_key, 0, sizeof(rec_key));
	(void)memset(rec_noise, 0, sizeof(rec_noise));

	for (pair = 0u; pair < PAIRS; pair++)
	{
		for (idx = 0u; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
		{
			plain[pair][idx] = (uint8_t)(
				((pair + 17u) * 73u) ^
				((idx + 41u) * 151u) ^
				((pair * idx + 19u) * 37u));
			cipher[pair][idx] = plain[pair][idx];
		}
		encodex(cipher[pair], secret_key);
	}

	for (src = 0u; src < ENCODEX_BLOCK_SIZE_BYTES; src++)
	{
		size_t output_matches;
		size_t matched_out;
		uint8_t matched_key;
		uint8_t matched_noise;

		output_matches = 0u;
		matched_out = 0u;
		matched_key = 0u;
		matched_noise = 0u;

		for (out = 0u; out < ENCODEX_BLOCK_SIZE_BYTES; out++)
		{
			int output_seen;

			output_seen = 0;
			for (candidate = 0u; candidate < 256u; candidate++)
			{
				uint8_t noise;
				int ok;

				noise = (uint8_t)(cipher[0u][out] ^
					(uint8_t)(rol8(plain[0u][src], (unsigned int)candidate) +
						(uint8_t)candidate));
				ok = 1;

				for (pair = 1u; pair < PAIRS; pair++)
				{
					if (cipher[pair][out] != enc_byte(
							plain[pair][src],
							(uint8_t)candidate,
							noise))
					{
						ok = 0;
						break;
					}
				}

				if ((ok != 0) && (output_seen == 0))
				{
					output_seen = 1;
					output_matches++;
					matched_out = out;
					matched_key = (uint8_t)candidate;
					matched_noise = noise;
				}
			}
		}

		if (output_matches != 1u)
		{
			(void)printf("attack inconclusive for source %lu: %lu outputs\n",
				(unsigned long)src,
				(unsigned long)output_matches);
			return 0;
		}

		source_for_output[matched_out] = (uint8_t)src;
		rec_key[src] = matched_key;
		rec_noise[src] = matched_noise;
	}

	for (idx = 0u; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		target_plain[idx] = (uint8_t)(0x80u + (idx * 7u));
		target_cipher[idx] = target_plain[idx];
	}
	encodex(target_cipher, secret_key);

	for (out = 0u; out < ENCODEX_BLOCK_SIZE_BYTES; out++)
	{
		src = source_for_output[out];
		recovered[src] = dec_byte(target_cipher[out], rec_key[src], rec_noise[src]);
	}

	print_block("target plaintext:  ", target_plain);
	print_block("target ciphertext: ", target_cipher);
	print_block("recovered:         ", recovered);

	if (memcmp(target_plain, recovered, sizeof(target_plain)) == 0)
	{
		(void)printf("byte-independent known-plaintext attack succeeded\n");
		return 1;
	}

	(void)printf("byte-independent known-plaintext attack failed\n");
	return 0;
}
