#ifndef SO_TABLES_H
#define SO_TABLES_H

#include <cstdint>

//X* jest typem stolika *-osobwego. Przy próbie modyfikacji ilości stolików należy ustawić je na twardo (modyfikując odpowiednie zmienne x* w pliku Tables.cpp)
#define X1 5
#define X2 5
#define X3 5
#define X4 5


extern int table_count;
extern int table_count_max;


struct Table {
	int8_t max_osob = 0;
	int8_t typ_grupy = 0;
	bool zarezerwowany_pzez_kierownika = false;
};

#endif //SO_TABLES_H
