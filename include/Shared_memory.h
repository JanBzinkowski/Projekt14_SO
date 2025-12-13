#ifndef SO_SHARED_MEMORY_H
#define SO_SHARED_MEMORY_H

#include <vector>

struct SharedMem {
	int8_t new_customers = true;
	int8_t end_program = false;
	int8_t new_tables = false;
	size_t tables_array_size = 0;
};

#endif //SO_SHARED_MEMORY_H
