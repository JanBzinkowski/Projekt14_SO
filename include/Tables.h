#ifndef SO_TABLES_H
#define SO_TABLES_H
#include <semaphore.h>

#define X1 1
#define X2 2
#define X3 3
#define X4 4

extern int table_count;

struct Table {
	int8_t max_osob = 0;
	int8_t rozmiar_grupy = 0;
};

#endif //SO_TABLES_H
