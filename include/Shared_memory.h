#ifndef SO_SHARED_MEMORY_H
#define SO_SHARED_MEMORY_H

#include <cctype>

#define MAX_KLIENTOW 10000
#define MAX_KLIENTOW_W_RRESTAURACJI 120

struct SharedMem {
	int8_t new_customers = true;
	int8_t end_program = false;
	int8_t new_tables = false;
	int8_t all_customers_out = false;
	size_t tables_array_size = 0;
};

#endif //SO_SHARED_MEMORY_H
