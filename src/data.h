#pragma once
#include <stddef.h>

// Returns pointer to embedded input.bin data.
const unsigned char* data_input_ptr(void);
// Returns size (bytes) of embedded input.bin.
size_t data_input_size(void);
// Copies up to 'len' bytes starting at 'offset' (0 = first byte of input.bin)
// into 'dst'. Returns number of bytes copied, 0 if offset >= asset size,
// or -1 on error (e.g. dst == NULL).
int data_input_copy(size_t offset, size_t len, void *dst);
