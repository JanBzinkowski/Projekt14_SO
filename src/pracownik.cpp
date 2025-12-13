#include <iostream>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <atomic>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

//obsługa sygnał 1-3

int shared;
sem_t *kolejka;
sem_t *klient;
int fifo_zwrot;
int fifo_zamowienie;

void zamowienie(SharedMem *shared_mem_flags) {
	Zamowienie zam;
	int fd = open("../IPC/zamowienie", O_RDONLY);
	read(fd, &zam, sizeof(Zamowienie));
	close(fd);

	auto *table_array = static_cast<Table *>(mmap(nullptr, shared_mem_flags->tables_array_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared, sizeof(SharedMem)));

	int array_size = shared_mem_flags->tables_array_size / sizeof(Table);
	int index = 0;
	for (index; index < array_size; index++) {
		int free;
		sem_getvalue(&table_array[index].wolne_miejsca, &free);
		if (free >= zam.liczba_osob && (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == zam.liczba_osob)) {
			for (int i = 0; i < zam.liczba_osob; i++) {
				sem_wait(&table_array[index].wolne_miejsca);
			}
			break;
		}
	}
	ZamowienieZwrot zwrot;
	if (index == array_size) {
		zwrot.nr_stolika = -1;
	}
	else {
		zwrot.nr_stolika = index;
	}

	fd = open("../IPC/zamowienie_zwrot", O_WRONLY);
	write(fd, &zwrot, sizeof(ZamowienieZwrot));
	close(fd);
}

int main() {
	fifo_zwrot = mkfifo("../IPC/zamowienie_zwrot", 0666);
	if (fifo_zwrot == -1) {
		std::cerr << "Pracownik nie mogl utwozyc FIFO. Proces nr.: " << getpid() << std::endl;
	}

	fifo_zamowienie = mkfifo("../IPC/zamowienie", 0666);
	if (fifo_zamowienie == -1) {
		std::cerr << "Pracownik nie mogl utwozyc FIFO. Proces nr.: " << getpid() << std::endl;
	}

	shared = shm_open("/shmem", O_RDWR, 0666);
	if (shared == -1) {
		perror("shared memory open");
		exit(1);
	}
	auto *shared_mem_flags = static_cast<SharedMem *>(mmap(nullptr, sizeof(SharedMem), PROT_READ, MAP_SHARED, shared, 0));

	kolejka = sem_open("kolejka_sem", O_CREAT | O_EXCL, 0666, 1);
	if (kolejka == SEM_FAILED) {
		std::cerr << "Nie mozna otworzyc semafora kolejki." << std::endl;
	}

	klient = sem_open("klient_czeka_sem", O_CREAT | O_EXCL, 0666, 0);
	if (klient == SEM_FAILED) {
		std::cerr << "Nie mozna otworzyc semafora informacji o oczekiwaniu klienta na zamowienie." << std::endl;
	}

	while (shared_mem_flags->end_program == false) {
		sem_wait(klient);
		zamowienie(shared_mem_flags);
	}

	sem_close(kolejka);
	sem_close(klient);
	sem_unlink("kolejka_sem");
	sem_unlink("klient_czeka_sem");
	unlink("../IPC/zamowienie");
	unlink("../IPC/zamowienie_zwrot");
}
