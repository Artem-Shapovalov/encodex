/* Chosen-plaintext regression probe for the single-block Encodex API.
 *
 * This probe assumes an encryption oracle and tries the old byte-independent
 * codebook attack. It returns failure if that attack succeeds.
 */

#include "encodex.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t secret_key[ENCODEX_KEY_SIZE_BYTES] = {
	0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u,
	0x09u, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u,
	0x17u, 0x18u, 0x19u, 0x20u, 0x21u, 0x22u, 0x23u, 0x24u,
	0x25u, 0x26u, 0x27u, 0x28u, 0x29u, 0x30u, 0x31u, 0x32u
};

static void oracle_encrypt(uint8_t block[ENCODEX_BLOCK_SIZE_BYTES])
{
	encodex(block, secret_key);
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
	uint8_t inverse[ENCODEX_BLOCK_SIZE_BYTES][256];
	uint8_t source_for_output[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t output_for_source[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t zero_cipher[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t probe[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t plain[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t cipher[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t recovered[ENCODEX_BLOCK_SIZE_BYTES];
	size_t pos;
	size_t out;
	size_t value;

	(void)memset(inverse, 0, sizeof(inverse));
	(void)memset(source_for_output, 0xff, sizeof(source_for_output));
	(void)memset(output_for_source, 0xff, sizeof(output_for_source));

	(void)memset(zero_cipher, 0, sizeof(zero_cipher));
	oracle_encrypt(zero_cipher);

	for (pos = 0u; pos < ENCODEX_BLOCK_SIZE_BYTES; pos++)
	{
		(void)memset(probe, 0, sizeof(probe));
		probe[pos] = 1u;
		oracle_encrypt(probe);

		for (out = 0u; out < ENCODEX_BLOCK_SIZE_BYTES; out++)
		{
			if (probe[out] != zero_cipher[out])
			{
				source_for_output[out] = (uint8_t)pos;
				output_for_source[pos] = (uint8_t)out;
				break;
			}
		}

		for (value = 0u; value < 256u; value++)
		{
			(void)memset(probe, 0, sizeof(probe));
			probe[pos] = (uint8_t)value;
			oracle_encrypt(probe);
			inverse[output_for_source[pos]][probe[output_for_source[pos]]] =
				(uint8_t)value;
		}
	}

	for (pos = 0u; pos < ENCODEX_BLOCK_SIZE_BYTES; pos++)
	{
		plain[pos] = (uint8_t)(0xa0u + pos);
	}

	(void)memcpy(cipher, plain, sizeof(cipher));
	oracle_encrypt(cipher);

	for (out = 0u; out < ENCODEX_BLOCK_SIZE_BYTES; out++)
	{
		recovered[source_for_output[out]] = inverse[out][cipher[out]];
	}

	print_block("plaintext:  ", plain);
	print_block("ciphertext: ", cipher);
	print_block("recovered:  ", recovered);

	if (memcmp(plain, recovered, sizeof(plain)) == 0)
	{
		(void)printf("byte-independent chosen-plaintext attack succeeded\n");
		return 1;
	}

	(void)printf("byte-independent chosen-plaintext attack failed\n");
	return 0;
}
