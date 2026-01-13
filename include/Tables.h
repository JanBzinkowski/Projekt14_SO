#ifndef SO_TABLES_H
#define SO_TABLES_H

#include <semaphore.h>

//X* jest typem stolika *-osobwego. Przy próbie modyfikacji ilości stolików należy ustawić je na twardo (modyfikując odpowiednie zmienne x* w pliku Tables.cpp)
#define X1 x1
#define X2 x2
#define X3 x3
#define X4 x4

extern int table_count;

extern int x1, x2, x3, x4;

struct Table {
	int8_t max_osob = 0;
	int8_t rozmiar_grupy = 0;
	bool zarezerwowany = false;
};

#endif //SO_TABLES_H
