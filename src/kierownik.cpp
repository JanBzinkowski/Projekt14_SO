#include <iostream>
#include <unistd.h>
#include <csignal>

#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/zamowienie.h"

int msgid_kierownik, msgid_logger, logger_semid, gen_semid, kasa_semid, msgid_zam, semid_prac;

pid_t mainprog_pid;

void handler(int sig) {
	if (sig == SIGINT || sig == SIGRTMIN) {
		kill(mainprog_pid, SIGRTMIN);
	}
	else if (sig == SIGRTMIN + 1) {
		kill(mainprog_pid, SIGRTMIN + 1);
	}
	else if (sig == SIGRTMIN + 2) {
		kill(mainprog_pid, SIGRTMIN + 2);
	}
}

void unlock_sem(int semid, int nsems) {
	for (int i = 0; i < nsems; ++i) {
		semctl(semid, i, SETVAL, 20000);
	}
}

void unlock_all() {
	unlock_sem(logger_semid, 1);
	unlock_sem(gen_semid, 3);
	unlock_sem(kasa_semid, 2);
	unlock_sem(semid_prac, 2);

	MsgText msg_logger{999, "0"};

	msgsnd(msgid_logger, &msg_logger, sizeof(msg_logger.text), IPC_NOWAIT);

	msg_zamowienie msg_zam;
	msg_zam.mtype = ZAMOWIENIE;
	msgsnd(msgid_zam, &msg_zam, sizeof(ZlozenieZamowienia), IPC_NOWAIT);

	msg_pracownik msg_pr;
	msg_zam.mtype = ZAMOWIENIE_PRACOWNIK;
	msgsnd(msgid_zam, &msg_pr, sizeof(ZamowieniePracownik), IPC_NOWAIT);

	KierownikStoly stoly = {EXTRA, false};
	msgsnd(msgid_kierownik, &stoly, sizeof(bool), IPC_NOWAIT);

	KierownikRezerwacja rezerwacja{REZERWACJE, {0, 0, 0, 0}};
	msgsnd(msgid_kierownik, &rezerwacja, sizeof(Reserve), IPC_NOWAIT);
}

void wyslij_sygnal(SharedMem *shared_mem_flags) {
	struct sigaction sa{};
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGRTMIN, &sa, nullptr);
	sigaction(SIGRTMIN + 1, &sa, nullptr);
	sigaction(SIGRTMIN + 2, &sa, nullptr);

	int a;
	bool endl_loop = false;
	Reserve r{};
	KierownikRezerwacja rezerwacja{};
	while (!shared_mem_flags->end_program && !endl_loop) {
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
					endl_loop = true;
					break;
				}
				shared_mem_flags->new_tables = true;
				kill(mainprog_pid, SIGRTMIN + 1);
				sem_op(semid_prac, 0, 1);
				endl_loop = true;
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
					else {
						break;
					}
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
					else {
						break;
					}
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
					else {
						break;
					}
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
					else {
						break;
					}
				}
				rezerwacja = {REZERWACJE, r};
				msg_send(msgid_kierownik, &rezerwacja, sizeof(KierownikRezerwacja), 0);
				kill(mainprog_pid, SIGRTMIN + 2);
				sem_op(semid_prac, 0, 1);
				endl_loop = true;
				break;

			case 3:
				kill(mainprog_pid, SIGRTMIN);
				unlock_all();
				endl_loop = true;
				break;

			default:
				continue;
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		return 1;
	}
	else {
		mainprog_pid = static_cast<pid_t>(std::stoi(argv[1]));
	}
	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));

	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
	msgid_logger = msg_create(ftok(".", 'L'), 0666);
	logger_semid = sem_create(ftok(".", 'P'), 1, 0666);
	msgid_kierownik = msg_create(ftok(".", 'I'), 0666);
	gen_semid = sem_create(ftok(".", 'G'), 3, 0666);
	kasa_semid = sem_create(ftok(".", 'K'), 2, 0666);
	msgid_zam = msg_create(ftok(".", 'Z'), 0666);
	semid_prac = sem_create(ftok(".", 'W'), 2, 0666);

	int a;
	while (!shared_mem_flags->end_program) {
		std::cout << "Witaj kierowniku. Co chcialbys zrobic?\n1. Wydac sygnal\n2. " << (shared_mem_flags->new_customers ? "Wylaczyc" : "Wlaczyc") << " tworzenie nowych klientow\n3. Zamknac restauracje" << std::endl;
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
				sem_op(gen_semid, 0, 1);
				break;
			case 3:
				shared_mem_flags->new_customers = false;
				shared_mem_flags->end_program = true;
				unlock_all();
				break;
			default:
				continue;
		}
	}

	shm_detach(base);
}
