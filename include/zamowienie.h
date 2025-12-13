#ifndef SO_ZAMOWIENIE_H
#define SO_ZAMOWIENIE_H

struct Zamowienie {
	int8_t liczba_osob = 0;
	int8_t nr_pozycji_menu = 0;
	int8_t nr_napoju = 0;
};

struct ZamowienieZwrot {
	int8_t nr_stolika = -1;
};

#endif //SO_ZAMOWIENIE_H
