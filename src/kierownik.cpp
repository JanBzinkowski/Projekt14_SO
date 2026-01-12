#include <iostream>
#include <unistd.h>

#include "../../../../../usr/include/signal.h"
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"


// możliwość wysłania sygnału do procesów po otrzymaniu sygnału z konsoli

void wyslij_sygnal(SharedMem *shared_mem_flags) {
	int a;
	while (shared_mem_flags->end_program) {
		std::cout << "Wybierz sygnal:\n1. Zwieksz liczbe stolikow\n2. Rezerwuj miejsca\n3. Pozar!" << std::endl;
		std::cin >> a;
		switch (a) {
			case 1:
				if (shared_mem_flags->new_tables) {
					std::cout << "Liczba stolikow zostala juz zwiekszona" << std::endl;
					break;
				}
				shared_mem_flags->new_tables = true;
				kill(getppid(), SIGRTMIN + 1);
				break;
			case 2:
				kill(getppid(), SIGRTMIN + 2);
				break;
			case 3:
				kill(getppid(), SIGRTMIN);
				break;
		}
	}
}

int main() {
	key_t shm_key = ftok(".", 'S');
	int shmid = shm_create(shm_key, sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));

	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);

	key_t sem_key = ftok(".", 'GK');
	if (sem_key == -1) {
		ipc_die("ftok");
	}

	int semid = sem_create(sem_key, 1, 0666);

	int a;
	while (!shared_mem_flags->end_program) {
		std::cout << "Witaj kierowniku. Co chcialbys zrobic?\n1. Wydac sygnal\n2. " << (shared_mem_flags->new_customers ? "Wylaczyc" : "Wlaczyc") << "tworzenie nowych klientow\n3. Zamknac restauracje" << std::endl;
		std::cin >> a;
		switch (a) {
			case 1:
				wyslij_sygnal(shared_mem_flags);
				break;
			case 2:
				if (shared_mem_flags->new_customers) {
					shared_mem_flags->new_customers = false;
					break;
				}
				shared_mem_flags->new_customers = true;
				sem_op(semid, 0, 1);
				break;
			case 3:
				shared_mem_flags->end_program = true;
				kill(getppid(), SIGRTMIN + 3);
				break;
		}
	}


	shm_detach(base);
	shm_detach(shared_mem_flags);
}
