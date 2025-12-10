#ifndef SO_TABLES_H
#define SO_TABLES_H
#include <semaphore.h>

struct Table {
	sem_t wolne_miejsca;
	char rozmiar_grupy;
};

#endif //SO_TABLES_H
