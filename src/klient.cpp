#include <iostream>
#include <unistd.h>
#include <fstream>
#include <ctime>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

//obsługa sygnał 3

void opuszczenie_lokalu(Zamowienie *zamowienie_klienta, ZamowienieZwrot *zamowienie_inf_zwrotna) {
	if (zamowienie_klienta != nullptr) {
		int shared = shm_open("/shmem", O_RDWR, 0666);
		if (shared == -1) {
			perror("shared memory open");
			exit(1);
		}

		auto *shared_mem_flags = static_cast<SharedMem *>(mmap(nullptr, sizeof(SharedMem), PROT_READ, MAP_SHARED, shared, 0));
		auto *table_array = static_cast<Table *>(mmap(nullptr, shared_mem_flags->tables_array_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared, sizeof(SharedMem)));
		for (int i = 0; i < zamowienie_klienta->liczba_osob; i++) {
			sem_post(&table_array[zamowienie_inf_zwrotna->nr_stolika].wolne_miejsca);
		}
	}
	std::ofstream log("../log.txt", std::ios::app);
	if (!log.is_open()) {
		std::cerr << "Klient nie mogl otworzyc pliku \"log.txt\". Proces nr.: " << getpid() << std::endl;
	}
	log << "Opuszczam(y) restaurację. Proces nr.: " << getpid() << std::endl;
}

void zwrot_naczyn() {
	std::ofstream log("../log.txt", std::ios::app);
	if (!log.is_open()) {
		std::cerr << "Klient nie mogl otworzyc pliku \"log.txt\". Proces nr.: " << getpid() << std::endl;
	}
	log << "Odkładam(y) naczynia. Proces nr.: " << getpid() << std::endl;
}

void zamowienie(Zamowienie *zamowienie_klienta, ZamowienieZwrot *zamowienie_inf_zwrotna) {
	sem_t *kolejka = sem_open("kolejka_sem", 0);
	sem_t *informacja = sem_open("klient_czeka_sem", 0);
	sem_wait(kolejka);
	if (zamowienie_klienta == nullptr) {
		opuszczenie_lokalu(nullptr, nullptr);
		sem_post(kolejka);
		sem_close(kolejka);
		sem_close(informacja);
		return;
	}

	sem_post(informacja);

	int fd = open("../IPC/zamowienie", O_WRONLY);
	write(fd, zamowienie_klienta, sizeof(Zamowienie));
	close(fd);

	fd = open("../IPC/zamowienie_zwrot", O_RDONLY);
	read(fd, zamowienie_inf_zwrotna, sizeof(ZamowienieZwrot));
	close(fd);

	sem_post(kolejka);
	sem_close(kolejka);
	sem_close(informacja);
}

int main() {
	srand(time(nullptr));
	if (rand() / RAND_MAX <= 0.05) {
		zamowienie(nullptr, nullptr);
		sleep(1);
	}

	Zamowienie zam;
	ZamowienieZwrot zwrot;
	zam.liczba_osob = rand() % 4 + 1;
	zam.nr_napoju = rand() % 5 + 1;
	zam.nr_pozycji_menu = rand() % 10 + 1;

	zamowienie(&zam, &zwrot);
	if (zwrot.nr_stolika < 0) {
		return -1;
	}


	sleep(zam.nr_pozycji_menu);
	sleep(zam.nr_napoju);

	zwrot_naczyn();
	opuszczenie_lokalu(&zam, &zwrot);
	sleep(1);
}
