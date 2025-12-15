#ifndef SO_ZAMOWIENIE_H
#define SO_ZAMOWIENIE_H

#define ZAMOWIENIE 1
#define ZAMOWIENIE_ZWROT 2

struct Zamowienie {
	int8_t liczba_osob = 0;
	int8_t nr_pozycji_menu = 0;
	int8_t nr_napoju = 0;
};

struct ZamowienieZwrot {
	int16_t nr_stolika = -1;
};

struct msg_zamowienie {
	long mtype = ZAMOWIENIE;
	Zamowienie zam;
};

struct msg_zwrot {
	long mtype = ZAMOWIENIE_ZWROT;
	ZamowienieZwrot zwrot;
};

#endif //SO_ZAMOWIENIE_H
