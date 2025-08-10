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

#ifndef ENCODEX_H
#define ENCODEX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stddef.h>

/** \brief Size of the key in bytes */
#define ENCODEX_KEY_SIZE_BYTES 32u

/** \brief Size of the memory block in bytes */
#define ENCODEX_BLOCK_SIZE_BYTES ENCODEX_KEY_SIZE_BYTES

/** \brief Encodes a single memory block with a given key.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               encrypted and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTES. This memory should be initialized
 *             with the encryption key. */
void encodex(uint8_t* block, const uint8_t* key);

/** \brief Decodes a signle memory block with a given key.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               decrypted and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key. The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTES. This memory should be initialized
 *             with the encryption key. */
void decodex(uint8_t* block, const uint8_t* key);

/** \brief Encodes a multiple memory blocks followed one-by-one with a given
 *         key using cypher block chaining algorithm.
 *  \param blocks Valid pointer to the blocks of memory. This memory would be
 *                encrypted and the new data would be written here instead of
 *                the old one. The size of the memory should be proportional
 *                to the ENCODEX_BLOCK_SIZE_BYTES.
 *  \param blocks_num Number of blocks stored in the memory provided by the
 *                    blocks parameter.
 *  \param key Valid pointer to the key. The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTES. This memory should be initialized
 *             with the encryption key. */
void encodex_cbc(uint8_t* blocks, size_t blocks_num, const uint8_t* key);

/** \bried Decodes a multiple memory blocks followed one-by-one with a given
 *         key using cypher block chaining algorithm.
 *  \param blocks Valid pointer to the blocks of memory. This memory would be
 *                decrypted and the new data would be written here instead of
 *                the old one. The size of the memory should be proportional
 *                to the ENCODEX_BLOCK_SIZE_BYTES.
 *  \param blocks_num Number of blocks stored on memory provided by the
 *                    blocks parameter.
 *  \param key Valid pointer to the key. The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTES. This memory should be initialized
 *             with the encryption key */
void decodex_cbc(uint8_t* blocks, size_t blocks_num, const uint8_t* key);

/** \brief Initializes the encoding and decoding stream context. Should be
 *         called before first call of encodex_cbc_stream or decodex_cbc_stream
 *         functions.
 *  \param key Valid pointer to the key, The size of the memory should be equal
 *             to ENCODEX_KEY_SIZE_BYTES. This memory should be initialized
 *             with the encryption key.
 *  \param seed Valid pointer to the context. This memory may be uninitialized
 *              and would be overwritten after this function call. */
void encodex_cbc_stream_init(const uint8_t* key, uint32_t* seed);

/** \brief Encodes a single block of the series with a given context using
 *         cypher block chaining algorithm.
 *  \warning This function should be called after the encodex_cbc_stream_init
 *           function call on the same context.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               encrypted and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key context. This function overwrites the
 *             memory by this pointer. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES.
 *  \param seed Valid pointer to the context. This function overwrites the
 *              memory by this pointer.*/
void encodex_cbc_stream(uint8_t* block, uint8_t* key, uint32_t* seed);

/** \brief Decodes a single block of the series with a given context using
 *         cypher block chaining algorithm.
 *  \warning This function should be called after the encodex_cbc_stream_init
 *           function call on the same context.
 *  \param block Valid pointer to the block of memory. This memory would be
 *               decrypted and the new data would be written here instead of
 *               the old one. The size of the memory should be equal to the
 *               ENCODEX_BLOCK_SIZE_BYTES.
 *  \param key Valid pointer to the key context. This function overwrites the
 *             memory by this pointer. The size of the memory should be equal
 *             to the ENCODEX_KEY_SIZE_BYTES.
 *  \param seed Valid pointer to the context. This function overwrites the
 *              memory by this pointer */
void decodex_cbc_stream(uint8_t* block, uint8_t* key, uint32_t* seed);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENCODEX_H */
