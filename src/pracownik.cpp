#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, semid_gk, msgid_zam, msgid_logger, msgid_kierownik;

volatile sig_atomic_t sig_flag = 0;
volatile sig_atomic_t fire_sig_flag = 0;

void handler(int sig) {
	if (sig == SIGRTMIN || sig == SIGINT) {
		fire_sig_flag = 1;
	}
	if (sig == SIGRTMIN + 1) {
		sig_flag = 1;
	}
	if (sig == SIGRTMIN + 2) {
		sig_flag = 2;
	}
}

void rezerwacje(KierownikRezerwacja &rezerwacja, Table *table, SharedMem *mem_flags) {
	sem_op(semid, 1, -1, &fire_sig_flag);
	for (int i = 0; i < rezerwacja.reserved.x1 && i < X1; i++)
		table[i].zarezerwowany = true;

	for (int i = X1; i < X1 + rezerwacja.reserved.x2 && i < X1 + X2; i++)
		table[i].zarezerwowany = true;

	if (!mem_flags->new_tables) {
		int start3 = X1 + X2;
		for (int i = start3; i < start3 + rezerwacja.reserved.x3 && i < start3 + X3; i++)
			table[i].zarezerwowany = true;

		int start4 = X1 + X2 + X3;
		for (int i = start4; i < start4 + rezerwacja.reserved.x4 && i < start4 + X4; i++)
			table[i].zarezerwowany = true;
	}
	else {
		int old3 = X3 / 2;
		int start3_old = X1 + X2;
		for (int i = start3_old; i < start3_old + std::min(rezerwacja.reserved.x3, old3); i++)
			table[i].zarezerwowany = true;

		int start4 = start3_old + old3;
		for (int i = start4; i < start4 + std::min(rezerwacja.reserved.x4, X4); i++)
			table[i].zarezerwowany = true;

		int new3_count = old3 + (X3 % 2);
		int start3_new = start4 + X4;
		for (int i = start3_new; i < start3_new + std::min(rezerwacja.reserved.x3 - old3, new3_count); i++) {
			table[i].zarezerwowany = true;
		}
	}
	sem_op(semid, 1, 1);
}


void exra(Table * &table) {
	if (X3 == 0) {
		X3 = 1;
	}
	for (int i = table_count; i < table_count + X3; i++) {
		table[i].max_osob = 3;
		table[i].zarezerwowany = false;
		table[i].rozmiar_grupy = 0;
	}
	table_count += X3;
}

void zamowienie() {
	msg_pracownik msg{};
	if (msg_recv(msgid_zam, &msg, sizeof(ZamowieniePracownik), ZAMOWIENIE_PRACOWNIK, 0, &fire_sig_flag) == -1) {
		return;
	}

	wyslij_log(msgid_logger, "Pracownik rozpoczyna obsluge klienta. Przydzielony stolik: stolik nr. " + std::to_string(msg.zwrot.nr_stolika));

	msg_zwrot zw{};
	zw.mtype = msg.zwrot.pid;
	zw.zwrot.nr_stolika = msg.zwrot.nr_stolika;

	msg_send(msgid_zam, &zw, sizeof(ZamowienieZwrot), 0);

	if (zw.zwrot.nr_stolika != -1) {
		wyslij_log(msgid_logger, "Pracownik wydal zamowienie, stolik nr: " + std::to_string(zw.zwrot.nr_stolika));
	}
	else {
		wyslij_log(msgid_logger, "Pracownik nie wydał zamowienia, brak miejsca w restauracji");
	}
}

int main() {
	struct sigaction sa{};
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGRTMIN, &sa, nullptr);
	sigaction(SIGRTMIN + 1, &sa, nullptr);
	sigaction(SIGRTMIN + 2, &sa, nullptr);

	shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));
	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
	auto *table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));

	msgid_zam = msg_create(ftok(".", 'Z'), 0666);
	msgid_logger = msg_create(ftok(".", 'L'), 0666);
	semid = sem_create(ftok(".", 'W'), 2, 0666);
	semid_gk = sem_create(ftok(".", 'G'), 3, 0666);
	msgid_kierownik = msg_create(ftok(".", 'I'), 0666);
	int logger_semid = sem_create(ftok(".", 'P'), 1, 0666);
	sem_op(logger_semid, 0, 1);

	wyslij_log(msgid_logger, "Pracownik rozpoczyna prace");

	while (!shared_mem_flags->end_program && !shared_mem_flags->all_customers_out) {
		sem_op(semid, 0, -1, &fire_sig_flag);
		if (fire_sig_flag == 1) {
			wyslij_log(msgid_logger, "Pracownik czekna na ewakuacje klientow");
			break;
		}
		else if (sig_flag == 1) {
			KierownikStoly st{};
			msg_recv(msgid_kierownik, &st, sizeof(KierownikStoly), EXTRA, 0, &fire_sig_flag);
		}
		else if (sig_flag == 2) {
			KierownikRezerwacja re{};
			msg_recv(msgid_kierownik, &re, sizeof(KierownikStoly), REZERWACJE, 0, &fire_sig_flag);
			rezerwacje(re, table_array, shared_mem_flags);
		}

		zamowienie();
	}

	sem_op(semid_gk, 1, -1, &fire_sig_flag);

	wyslij_log(msgid_logger, "Pracownik konczy prace");

	sem_op(logger_semid, 0, -1);

	shm_detach(base);
	return 0;
}
