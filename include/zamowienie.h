#ifndef SO_ZAMOWIENIE_H
#define SO_ZAMOWIENIE_H

#define ZAMOWIENIE 1
#define ZAMOWIENIE_PRACOWNIK 2

//Liczba pozycji menu napojów/posiłków (nr. pozycji menu odpowiada czasowi spozywania posilku/napoju)

#define MENU_NAPOJE 4
#define MENU_POSILKI 4

struct Zamowienie {
	int8_t nr_pozycji_menu = 0;
	int8_t nr_napoju = 0;
};

struct ZlozenieZamowienia {
	pid_t pid = -1;
	int8_t liczba_osob = 0;
	Zamowienie zamowienie1;
	Zamowienie zamowienie2;
	Zamowienie zamowienie3;
	Zamowienie zamowienie4;
};

struct ZamowieniePracownik {
	int nr_stolika = -1;
	pid_t pid = -1;
	Zamowienie zamowienie1;
	Zamowienie zamowienie2;
	Zamowienie zamowienie3;
	Zamowienie zamowienie4;
};

struct ZamowienieZwrot {
	int16_t nr_stolika = -1;
};

struct msg_zamowienie {
	long mtype = ZAMOWIENIE;
	ZlozenieZamowienia zam;
};

struct msg_pracownik {
	long mtype = ZAMOWIENIE_PRACOWNIK;
	ZamowieniePracownik zwrot;
};

struct msg_zwrot {
	long mtype = 1;
	ZamowienieZwrot zwrot;
};

#endif //SO_ZAMOWIENIE_H
