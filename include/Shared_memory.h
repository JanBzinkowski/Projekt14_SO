#ifndef SO_SHARED_MEMORY_H
#define SO_SHARED_MEMORY_H

struct SharedMem {
	bool new_customers = true;
	bool end_program = false;
	bool new_tables = false;
	size_t tables_array_size;
};

#endif //SO_SHARED_MEMORY_H
