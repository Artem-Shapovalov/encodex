/* Encodex visualizer.
 *
 * Converts raw 8-bit grayscale data files to PNG and regenerates Encodex step
 * images for the README.
 */

#include "../encodex.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_WIDTH 300u
#define IMAGE_HEIGHT 384u

static const uint8_t visual_key[ENCODEX_KEY_SIZE_BYTES] = {
	0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u,
	0x09u, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u,
	0x17u, 0x18u, 0x19u, 0x20u, 0x21u, 0x22u, 0x23u, 0x24u,
	0x25u, 0x26u, 0x27u, 0x28u, 0x29u, 0x30u, 0x31u, 0x32u
};

enum visual_step
{
	VIS_ORIGINAL,
	VIS_ROL_BLOCK,
	VIS_ADD_KEY,
	VIS_ROL_FULL_BLOCK,
	VIS_NOIZE,
	VIS_FEISTEL,
	VIS_SHUFFLE,
	VIS_ENCODED
};

static uint32_t crc32_table[256];

static void crc32_init(void)
{
	uint32_t c;
	size_t idx;
	size_t bit;

	for (idx = 0u; idx < 256u; idx++)
	{
		c = (uint32_t)idx;
		for (bit = 0u; bit < 8u; bit++)
		{
			if ((c & 1u) != 0u)
			{
				c = 0xedb88320u ^ (c >> 1u);
			}
			else
			{
				c >>= 1u;
			}
		}
		crc32_table[idx] = c;
	}
}

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len)
{
	size_t idx;

	for (idx = 0u; idx < len; idx++)
	{
		crc = crc32_table[(crc ^ data[idx]) & 0xffu] ^ (crc >> 8u);
	}

	return crc;
}

static uint32_t adler32(const uint8_t* data, size_t len)
{
	uint32_t a;
	uint32_t b;
	size_t idx;

	a = 1u;
	b = 0u;
	for (idx = 0u; idx < len; idx++)
	{
		a = (a + data[idx]) % 65521u;
		b = (b + a) % 65521u;
	}

	return (b << 16u) | a;
}

static void write_u32(FILE* f, uint32_t v)
{
	(void)fputc((int)((v >> 24u) & 0xffu), f);
	(void)fputc((int)((v >> 16u) & 0xffu), f);
	(void)fputc((int)((v >> 8u) & 0xffu), f);
	(void)fputc((int)(v & 0xffu), f);
}

static int write_chunk(
		FILE* f,
		const char type[4],
		const uint8_t* data,
		size_t len)
{
	uint32_t crc;

	write_u32(f, (uint32_t)len);
	(void)fwrite(type, 1u, 4u, f);
	if (len != 0u)
	{
		(void)fwrite(data, 1u, len, f);
	}

	crc = 0xffffffffu;
	crc = crc32_update(crc, (const uint8_t*)type, 4u);
	if (len != 0u)
	{
		crc = crc32_update(crc, data, len);
	}
	write_u32(f, crc ^ 0xffffffffu);

	return ferror(f) == 0 ? 0 : 1;
}

static size_t zlib_stored_size(size_t raw_size)
{
	return 2u + ((raw_size + 65534u) / 65535u) * 5u + raw_size + 4u;
}

static int make_zlib_stored(
		const uint8_t* raw,
		size_t raw_size,
		uint8_t* zlib,
		size_t* zlib_size)
{
	size_t src;
	size_t dst;
	size_t block_len;
	uint16_t len16;
	uint16_t nlen16;

	src = 0u;
	dst = 0u;
	zlib[dst++] = 0x78u;
	zlib[dst++] = 0x01u;

	while (src < raw_size)
	{
		block_len = raw_size - src;
		if (block_len > 65535u)
		{
			block_len = 65535u;
		}

		zlib[dst++] = (src + block_len >= raw_size) ? 1u : 0u;
		len16 = (uint16_t)block_len;
		nlen16 = (uint16_t)(~len16);
		zlib[dst++] = (uint8_t)(len16 & 0xffu);
		zlib[dst++] = (uint8_t)((len16 >> 8u) & 0xffu);
		zlib[dst++] = (uint8_t)(nlen16 & 0xffu);
		zlib[dst++] = (uint8_t)((nlen16 >> 8u) & 0xffu);
		(void)memcpy(&zlib[dst], &raw[src], block_len);
		src += block_len;
		dst += block_len;
	}

	*zlib_size = dst;
	return 0;
}

static void put_u32(uint8_t* data, uint32_t v)
{
	data[0] = (uint8_t)((v >> 24u) & 0xffu);
	data[1] = (uint8_t)((v >> 16u) & 0xffu);
	data[2] = (uint8_t)((v >> 8u) & 0xffu);
	data[3] = (uint8_t)(v & 0xffu);
}

static int write_png(
		const char* path,
		const uint8_t* pixels,
		size_t width,
		size_t height)
{
	FILE* f;
	uint8_t sig[8];
	uint8_t ihdr[13];
	uint8_t* raw;
	uint8_t* zlib;
	size_t raw_size;
	size_t zlib_capacity;
	size_t zlib_size;
	size_t row;
	uint32_t adler;
	int res;

	raw_size = (width + 1u) * height;
	zlib_capacity = zlib_stored_size(raw_size);
	raw = (uint8_t*)malloc(raw_size);
	zlib = (uint8_t*)malloc(zlib_capacity);
	if ((raw == NULL) || (zlib == NULL))
	{
		free(raw);
		free(zlib);
		return 1;
	}

	for (row = 0u; row < height; row++)
	{
		raw[row * (width + 1u)] = 0u;
		(void)memcpy(
			&raw[(row * (width + 1u)) + 1u],
			&pixels[row * width],
			width);
	}

	(void)make_zlib_stored(raw, raw_size, zlib, &zlib_size);
	adler = adler32(raw, raw_size);
	put_u32(&zlib[zlib_size], adler);
	zlib_size += 4u;

	f = fopen(path, "wb");
	if (f == NULL)
	{
		free(raw);
		free(zlib);
		return 1;
	}

	sig[0] = 0x89u;
	sig[1] = 'P';
	sig[2] = 'N';
	sig[3] = 'G';
	sig[4] = 0x0du;
	sig[5] = 0x0au;
	sig[6] = 0x1au;
	sig[7] = 0x0au;

	(void)fwrite(sig, 1u, sizeof(sig), f);
	put_u32(&ihdr[0], (uint32_t)width);
	put_u32(&ihdr[4], (uint32_t)height);
	ihdr[8] = 8u;
	ihdr[9] = 0u;
	ihdr[10] = 0u;
	ihdr[11] = 0u;
	ihdr[12] = 0u;

	res = write_chunk(f, "IHDR", ihdr, sizeof(ihdr));
	res |= write_chunk(f, "IDAT", zlib, zlib_size);
	res |= write_chunk(f, "IEND", NULL, 0u);
	res |= fclose(f);

	free(raw);
	free(zlib);
	return res;
}

static int read_file(const char* path, uint8_t* data, size_t size)
{
	FILE* f;
	size_t n;

	f = fopen(path, "rb");
	if (f == NULL)
	{
		return 1;
	}

	n = fread(data, 1u, size, f);
	if (fclose(f) != 0)
	{
		return 1;
	}

	return n == size ? 0 : 1;
}

static void apply_step_block(
		uint8_t* block,
		const uint8_t* key,
		enum visual_step step)
{
	switch (step)
	{
		case VIS_ROL_BLOCK:
		rol_block(block, key);
		break;

		case VIS_ADD_KEY:
		add_key(block, key);
		break;

		case VIS_ROL_FULL_BLOCK:
		rol_full_block(block, convolute(key) %
			(ENCODEX_BLOCK_SIZE_BYTES * 8u));
		break;

		case VIS_NOIZE:
		noize(block, key);
		break;

		case VIS_FEISTEL:
		feistel_layer(block, key);
		break;

		case VIS_SHUFFLE:
		shuffle(block, key);
		break;

		case VIS_ENCODED:
		encodex(block, key);
		break;

		default:
		break;
	}
}

static void apply_step(
		uint8_t* data,
		size_t size,
		enum visual_step step,
		int use_cbc)
{
	size_t offset;
	uint8_t key[ENCODEX_KEY_SIZE_BYTES];
	uint32_t seed;
	size_t idx;

	for (idx = 0u; idx < ENCODEX_KEY_SIZE_BYTES; idx++)
	{
		key[idx] = visual_key[idx];
	}

	encodex_cbc_stream_init(key, &seed);

	for (offset = 0u; offset < size; offset += ENCODEX_BLOCK_SIZE_BYTES)
	{
		if (use_cbc != 0)
		{
			seed = cbc(key, seed);
		}
		apply_step_block(&data[offset], key, step);
	}
}

static int generate_one(
		const char* input_path,
		const char* output_path,
		enum visual_step step,
		int use_cbc)
{
	uint8_t* data;
	size_t size;
	int res;

	size = IMAGE_WIDTH * IMAGE_HEIGHT;
	data = (uint8_t*)malloc(size);
	if (data == NULL)
	{
		return 1;
	}

	res = read_file(input_path, data, size);
	if (res == 0)
	{
		apply_step(data, size, step, use_cbc);
		res = write_png(output_path, data, IMAGE_WIDTH, IMAGE_HEIGHT);
	}

	free(data);
	return res;
}

static int generate_set(const char* name)
{
	char input[128];
	char output[128];
	int res;

	(void)sprintf(input, "example/%s.data", name);
	res = 0;

#define GEN(suffix, step, cbc) \
	do { \
		(void)sprintf(output, "docs/%s%s.png", name, suffix); \
		res |= generate_one(input, output, step, cbc); \
	} while (0)

	GEN("", VIS_ORIGINAL, 0);
	GEN("_rol_block", VIS_ROL_BLOCK, 0);
	GEN("_add_key", VIS_ADD_KEY, 0);
	GEN("_rol_full_block", VIS_ROL_FULL_BLOCK, 0);
	GEN("_noize", VIS_NOIZE, 0);
	GEN("_feistel", VIS_FEISTEL, 0);
	GEN("_shuffle", VIS_SHUFFLE, 0);
	GEN("_encoded", VIS_ENCODED, 0);
	GEN("_rol_block_cbc", VIS_ROL_BLOCK, 1);
	GEN("_add_key_cbc", VIS_ADD_KEY, 1);
	GEN("_rol_full_block_cbc", VIS_ROL_FULL_BLOCK, 1);
	GEN("_noize_cbc", VIS_NOIZE, 1);
	GEN("_feistel_cbc", VIS_FEISTEL, 1);
	GEN("_shuffle_cbc", VIS_SHUFFLE, 1);
	GEN("_encoded_cbc", VIS_ENCODED, 1);

#undef GEN

	return res;
}

int main(int argc, char** argv)
{
	crc32_init();

	if (argc == 4)
	{
		uint8_t* data;
		size_t size;
		int res;

		size = IMAGE_WIDTH * IMAGE_HEIGHT;
		data = (uint8_t*)malloc(size);
		if (data == NULL)
		{
			return 1;
		}

		res = read_file(argv[1], data, size);
		if (res == 0)
		{
			res = write_png(argv[2], data, (size_t)atoi(argv[3]), size /
				(size_t)atoi(argv[3]));
		}
		free(data);
		return res;
	}

	if ((argc == 2) && (strcmp(argv[1], "all") == 0))
	{
		return generate_set("portrait") | generate_set("teapot");
	}

	(void)printf("Usage:\n");
	(void)printf("  docs/visualizer all\n");
	(void)printf("  docs/visualizer <input.data> <output.png> <width>\n");
	return 1;
}
