#ifndef SO_TABLES_H
#define SO_TABLES_H
#include <semaphore.h>

struct Table {
	sem_t wolne_miejsca;
	int8_t rozmiar_grupy = 0;
};

#endif //SO_TABLES_H
