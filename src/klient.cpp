#include <iostream>
#include <unistd.h>
#include <fstream>
#include <ctime>
#include <semaphore.h>
#include <fcntl.h>

#include "../../../../../usr/include/x86_64-linux-gnu/sys/stat.h"
#include "../include/zamowienie.h"

//obsługa sygnał 3

void opuszczenie_lokalu() {
	std::ofstream log("../log.txt", std::ios::app);
	if (!log.is_open()) {
		std::cerr << "Klient nie mogl otworzyc pliku \"log.txt\". Proces nr.: " << getpid() << std::endl;
	}
	log << "Opuszczam restaurację. Proces nr.: " << getpid() << std::endl;
}

void zamowienie(Zamowienie *zamowienie) {
	sem_t *kolejka = sem_open("kolejka_sem", 0);
	sem_wait(kolejka);
	if (!zamowienie) {
		opuszczenie_lokalu();
	}

	int zam = mkfifo("../IPC/zamowienie", 0666);
	if (zam == -1) {
		std::cerr << "Klient nie mogl utwozyc FIFO. Proces nr.: " << getpid() << std::endl;
	}

	int fd = open("../IPC/zamowienie", O_WRONLY);
	write(fd, zamowienie, sizeof(Zamowienie));
	close(fd);

	//ODCZYT ZWROTU ZAMOWIENIA od pracownika
		sem_post(kolejka);
	sem_close(kolejka);
	unlink("../IPC/zamowienie");
	sem_unlink("kolejka_sem");
}

int main() {
	srand(time(nullptr));
	if (rand() / RAND_MAX <= 0.05) {
		zamowienie(nullptr);
		sleep(1);
	}

	auto *zam = new Zamowienie();
	zam->liczba_osob = rand() % 4 + 1;
	zam->nr_napoju = rand() % 5 + 1;
	zam->nr_pozycji_menu = rand() % 10 + 1;

	zamowienie(zam);
	//zwrot naczyn

	opuszczenie_lokalu();
	sleep(1);
}
