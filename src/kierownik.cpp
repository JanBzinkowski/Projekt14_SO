#include <iostream>
#include <unistd.h>

#include "../../../../../usr/include/signal.h"
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

int msgid;

// możliwość wysłania sygnału do procesów po otrzymaniu sygnału z konsoli

void wyslij_sygnal(SharedMem *shared_mem_flags) {
	int a;
	Reserve r{};
	KierownikRezerwacja rezerwacja{};
	while (!shared_mem_flags->end_program) {
		std::cout << "Wybierz sygnal:\n1. Zwieksz liczbe stolikow\n2. Rezerwuj miejsca\n3. Pozar!" << std::endl;
		std::cin >> a;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			std::cout << "Wprowadzono niepoprawny znak, sprobuj ponownie.\n";
			continue;
		}
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
				while (!shared_mem_flags->end_program) {
					std::cout << "Ile X1 zarezerwowac?" << std::endl;
					std::cin >> r.x1;
					if (std::cin.fail()) {
						std::cin.clear();
						std::cin.ignore(10000, '\n');
						std::cout << "Wprowadzono niepoprawny znak, sprobuj ponownie.\n";
						continue;
					}
					if (r.x1 > X1) {
						std::cout << "Max stolikow X1 to " << X1 << std::endl;
					}
					else
						break;
				}
				while (!shared_mem_flags->end_program) {
					std::cout << "Ile X2 zarezerwowac?" << std::endl;
					std::cin >> r.x2;
					if (std::cin.fail()) {
						std::cin.clear();
						std::cin.ignore(10000, '\n');
						std::cout << "Wprowadzono niepoprawny znak, sprobuj ponownie.\n";
						continue;
					}
					if (r.x2 > X2) {
						std::cout << "Max stolikow X2 to " << X2 << std::endl;
					}
					else
						break;
				}
				while (!shared_mem_flags->end_program) {
					std::cout << "Ile X3 zarezerwowac?" << std::endl;
					std::cin >> r.x3;
					if (std::cin.fail()) {
						std::cin.clear();
						std::cin.ignore(10000, '\n');
						std::cout << "Wprowadzono niepoprawny znak, sprobuj ponownie.\n";
						continue;
					}
					if (r.x3 > X3) {
						std::cout << "Max stolikow X3 to " << X3 << std::endl;
					}
					else
						break;
				}
				while (!shared_mem_flags->end_program) {
					std::cout << "Ile X4 zarezerwowac?" << std::endl;
					std::cin >> r.x4;
					if (std::cin.fail()) {
						std::cin.clear();
						std::cin.ignore(10000, '\n');
						std::cout << "Wprowadzono niepoprawny znak, sprobuj ponownie.\n";
						continue;
					}
					if (r.x4 > X4) {
						std::cout << "Max stolikow X4 to " << X4 << std::endl;
					}
					else
						break;
				}
				rezerwacja = {REZERWACJE, r};
				msg_send(msgid, &rezerwacja, sizeof(KierownikRezerwacja), 0);
				kill(getppid(), SIGRTMIN + 2);
				break;

			case 3:
				kill(getppid(), SIGRTMIN);
				break;

			default:
				continue;
		}
	}
}

int main() {
	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));

	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);

	int semid = sem_create(ftok(".", 'G'), 1, 0666);

	msgid = msg_create(ftok(".", 'I'), 0666);

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
				shared_mem_flags->new_customers = false;
				shared_mem_flags->end_program = true;
				break;
			default:
				continue;
		}
	}

	shm_detach(base);
	shm_detach(shared_mem_flags);
}
