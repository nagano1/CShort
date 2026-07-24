#include "common.hpp"

#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

/**
 * Ensure that at least `needed` more bytes can be appended without
 * reallocating.  Doubles the capacity until it is large enough.
 * Returns true on success, false if realloc fails.
 */

static bool bdb_ensure_capacity(BinaryDataBuilder *b, size_t needed)
{
    size_t required = b->size + needed;
    if (required < b->size) {
        return false; /* overflow */
    }

    if (required <= b->capacity) {
        return true; /* already enough room */
    }

    /* Double the capacity until it covers the required size */
    size_t new_capacity = required + BINARY_DATA_BUILDER_DEFAULT_CAPACITY;
    uint8_t *new_data = (uint8_t *)realloc(b->data, new_capacity);
    if (!new_data) {
        return false; /* allocation failed; original buffer is still valid */
    }

    b->data     = new_data;
    b->capacity = new_capacity;
    return true;
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

bool BinaryDataBuilder::init(size_t initial_capacity)
{
    assert(data == nullptr && capacity == 0 && "BinaryDataBuilder is already initialized");
    if (initial_capacity == 0) {
        initial_capacity = BINARY_DATA_BUILDER_DEFAULT_CAPACITY;
    }

    data = (uint8_t *)malloc(initial_capacity);
    if (!data) {
        size     = 0;
        capacity = 0;
        return false;
    }

    size     = 0;
    capacity = initial_capacity;
    return true;
}

void BinaryDataBuilder::freeAll()
{
    free(data);
    data     = NULL;
    size     = 0;
    capacity = 0;
}

bool BinaryDataBuilder::append_byte(uint8_t byte)
{
    if (!bdb_ensure_capacity(this, 1)) {
        return false;
    }
    data[size++] = byte;
    return true;
}

bool BinaryDataBuilder::append_bytes(const uint8_t *src, size_t len)
{
    if (len == 0) {
        return true;
    }
    if (src == nullptr) {
        assert(false && "src is nullptr");
        return false;
    }
    
    if (!bdb_ensure_capacity(this, len)) {
        return false;
    }
    memcpy(data + size, src, len);
    size += len;
    return true;
}

bool BinaryDataBuilder::append_u16le(uint16_t value)
{
    uint8_t buf[2] = {
        (uint8_t)(value      ),   /* low  byte */
        (uint8_t)(value >> 8 )    /* high byte */
    };
    return append_bytes(buf, sizeof(buf));
}

bool BinaryDataBuilder::append_u32le(uint32_t value)
{
    uint8_t buf[4] = {
        (uint8_t)(value      ),
        (uint8_t)(value >>  8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24)
    };
    return append_bytes(buf, sizeof(buf));
}

bool BinaryDataBuilder::append_u64le(uint64_t value)
{
    uint8_t buf[8] = {
        (uint8_t)(value      ),
        (uint8_t)(value >>  8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24),
        (uint8_t)(value >> 32),
        (uint8_t)(value >> 40),
        (uint8_t)(value >> 48),
        (uint8_t)(value >> 56)
    };
    return append_bytes(buf, sizeof(buf));
}

void BinaryDataBuilder::reset()
{
    size = 0; /* capacity and buffer pointer are preserved */
}