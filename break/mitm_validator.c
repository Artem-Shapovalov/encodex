/* Active MITM regression probe with a receiver accept/reject oracle.
 *
 * Attacker assumptions:
 * - sees one valid ciphertext packet;
 * - cannot call the encoder;
 * - does not know the key;
 * - does not know the packet format;
 * - may submit modified ciphertexts and observe accepted/rejected.
 *
 * This tries the old two-byte mutation search. It returns failure if a forged
 * accepted packet is found within the query budget.
 */

#include "encodex.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define QUERY_LIMIT 100000ul

static const uint8_t secret_key[ENCODEX_KEY_SIZE_BYTES] = {
	0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u,
	0x09u, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u,
	0x17u, 0x18u, 0x19u, 0x20u, 0x21u, 0x22u, 0x23u, 0x24u,
	0x25u, 0x26u, 0x27u, 0x28u, 0x29u, 0x30u, 0x31u, 0x32u
};

static uint8_t checksum31(const uint8_t* packet)
{
	size_t idx;
	uint8_t sum;

	sum = 0u;
	for (idx = 0u; idx < 31u; idx++)
	{
		sum = (uint8_t)(sum + packet[idx]);
	}

	return sum;
}

static int hidden_validate_plaintext(const uint8_t* packet)
{
	size_t idx;
	int valid;

	valid = 1;
	if ((packet[0] != 'P') || (packet[1] != 'K') || (packet[2] != 'T'))
	{
		valid = 0;
	}

	if ((packet[3] < 1u) || (packet[3] > 4u))
	{
		valid = 0;
	}

	if (packet[4] != 26u)
	{
		valid = 0;
	}

	for (idx = 5u; idx < 31u; idx++)
	{
		if ((packet[idx] < 'A') || (packet[idx] > 'Z'))
		{
			valid = 0;
		}
	}

	if (packet[31] != checksum31(packet))
	{
		valid = 0;
	}

	return valid;
}

static int receiver_accepts(const uint8_t* ciphertext)
{
	uint8_t packet[ENCODEX_BLOCK_SIZE_BYTES];

	(void)memcpy(packet, ciphertext, sizeof(packet));
	decodex(packet, secret_key);
	return hidden_validate_plaintext(packet);
}

static void hidden_decrypt_for_demo(uint8_t* plaintext, const uint8_t* ciphertext)
{
	(void)memcpy(plaintext, ciphertext, ENCODEX_BLOCK_SIZE_BYTES);
	decodex(plaintext, secret_key);
}

static void print_packet(const char* label, const uint8_t* packet)
{
	size_t idx;

	(void)printf("%s", label);
	for (idx = 0u; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		(void)printf("%02x", packet[idx]);
	}
	(void)printf("  ");
	for (idx = 0u; idx < ENCODEX_BLOCK_SIZE_BYTES; idx++)
	{
		(void)printf("%c",
			((packet[idx] >= 32u) && (packet[idx] <= 126u)) ? packet[idx] : '.');
	}
	(void)printf("\n");
}

int main(void)
{
	uint8_t plain[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t original_cipher[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t forged_cipher[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t original_plain[ENCODEX_BLOCK_SIZE_BYTES];
	uint8_t forged_plain[ENCODEX_BLOCK_SIZE_BYTES];
	size_t idx;
	size_t pos1;
	size_t pos2;
	size_t val1;
	size_t val2;
	unsigned long queries;
	int found;

	plain[0] = 'P';
	plain[1] = 'K';
	plain[2] = 'T';
	plain[3] = 1u;
	plain[4] = 26u;
	for (idx = 5u; idx < 31u; idx++)
	{
		plain[idx] = (uint8_t)('A' + ((idx * 7u) % 26u));
	}
	plain[31] = checksum31(plain);

	(void)memcpy(original_cipher, plain, sizeof(original_cipher));
	encodex(original_cipher, secret_key);

	queries = 0u;
	found = 0;
	(void)memcpy(forged_cipher, original_cipher, sizeof(forged_cipher));

	for (pos1 = 0u; (pos1 < ENCODEX_BLOCK_SIZE_BYTES) && (found == 0) &&
			(queries < QUERY_LIMIT); pos1++)
	{
		for (pos2 = 0u; (pos2 < ENCODEX_BLOCK_SIZE_BYTES) &&
				(found == 0) && (queries < QUERY_LIMIT); pos2++)
		{
			if (pos1 == pos2)
			{
				continue;
			}

			for (val1 = 0u; (val1 < 256u) && (found == 0) &&
					(queries < QUERY_LIMIT); val1++)
			{
				if ((uint8_t)val1 == original_cipher[pos1])
				{
					continue;
				}

				for (val2 = 0u; (val2 < 256u) && (queries < QUERY_LIMIT);
					val2++)
				{
					if ((uint8_t)val2 == original_cipher[pos2])
					{
						continue;
					}

					(void)memcpy(forged_cipher, original_cipher,
						sizeof(forged_cipher));
					forged_cipher[pos1] = (uint8_t)val1;
					forged_cipher[pos2] = (uint8_t)val2;

					queries++;
					if (receiver_accepts(forged_cipher) != 0)
					{
						found = 1;
						break;
					}
				}
			}
		}
	}

	if (found == 0)
	{
		(void)printf("no accepted mutation found within %lu oracle queries\n",
			QUERY_LIMIT);
		return 0;
	}

	hidden_decrypt_for_demo(original_plain, original_cipher);
	hidden_decrypt_for_demo(forged_plain, forged_cipher);

	(void)printf("accepted forged packet after %lu oracle queries\n", queries);
	print_packet("original plaintext: ", original_plain);
	print_packet("forged plaintext:   ", forged_plain);
	print_packet("original cipher:    ", original_cipher);
	print_packet("forged cipher:      ", forged_cipher);

	return (memcmp(original_cipher, forged_cipher, sizeof(original_cipher)) != 0) ?
		1 : 0;
}
