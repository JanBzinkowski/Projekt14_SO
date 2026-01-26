#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, semid_gk, msgid_zam, msgid_logger, msgid_kierownik, table_sem_id;

volatile sig_atomic_t sig_flag = 0;
volatile sig_atomic_t fire_sig_flag = 0;

void handler(int sig) {
	if (sig == SIGRTMIN) {
		fire_sig_flag = 1;
		sig_flag = 3;
	}
	if (sig == SIGINT) {
		sig_flag = 4;
	}
	if (sig == SIGRTMIN + 1) {
		sig_flag = 1;
	}
	if (sig == SIGRTMIN + 2) {
		sig_flag = 2;
	}
}

void rezerwacje(KierownikRezerwacja &rezerwacja, Table *table, SharedMem *mem_flags) {
	if (sem_op(semid, 1, -1, &fire_sig_flag)) {
		return;
	}

	for (int i = 0; i < rezerwacja.reserved.x1 && i < X1; i++)
		table[i].zarezerwowany_pzez_kierownika = true;

	for (int i = X1; i < X1 + rezerwacja.reserved.x2 && i < X1 + X2; i++)
		table[i].zarezerwowany_pzez_kierownika = true;

	int start3 = X1 + X2;

	if (!mem_flags->new_tables) {
		for (int i = start3; i < start3 + rezerwacja.reserved.x3 && i < start3 + X3; i++)
			table[i].zarezerwowany_pzez_kierownika = true;
	}
	else {
		int added3 = (X3 == 0 ? 1 : X3);

		int reserved_base3 = std::min(rezerwacja.reserved.x3, X3);
		for (int i = start3; i < start3 + reserved_base3; i++)
			for (int i = start3; i < start3 + reserved_base3; i++)
				table[i].zarezerwowany_pzez_kierownika = true;

		int reserved_extra3 = rezerwacja.reserved.x3 - reserved_base3;
		if (reserved_extra3 > 0) {
			int start3_extra = mem_flags->table_count - added3;
			for (int i = start3_extra; i < start3_extra + std::min(reserved_extra3, added3); i++)
				table[i].zarezerwowany_pzez_kierownika = true;
		}
	}

	int start4 = X1 + X2 + X3;
	for (int i = start4; i < start4 + rezerwacja.reserved.x4 && i < start4 + X4; i++)
		table[i].zarezerwowany_pzez_kierownika = true;

	sem_op(semid, 1, 1);
}


void extra(Table * &table, SharedMem * &mem_flags) {
	int old_table_count = mem_flags->table_count;
	mem_flags->table_count += (X3 == 0 ? 1 : X3);
	for (int i = old_table_count; i < mem_flags->table_count; i++) {
		table[i].max_osob = 3;
		table[i].zarezerwowany_pzez_kierownika = false;
		table[i].typ_grupy = 0;
		sem_op(table_sem_id, i, 3);
	}
}

void zamowienie() {
	msg_pracownik msg{};
	if (msg_recv(msgid_zam, &msg, sizeof(msg.zwrot), ZAMOWIENIE_PRACOWNIK, 0, &sig_flag) == -1) {
		return;
	}

	if (msg.zwrot.nr_stolika < 0) {
		return;
	}

	wyslij_log(msgid_logger, "Pracownik rozpoczyna obsluge klienta. Przydzielony stolik: stolik nr. " + std::to_string(msg.zwrot.nr_stolika), 3);

	msg_zwrot zw{};
	zw.mtype = msg.zwrot.pid;
	zw.zwrot.nr_stolika = msg.zwrot.nr_stolika;

	msg_send(msgid_zam, &zw, sizeof(zw.zwrot), 0);

	if (zw.zwrot.nr_stolika != -1) {
		wyslij_log(msgid_logger, "Pracownik wydal zamowienie, stolik nr: " + std::to_string(zw.zwrot.nr_stolika), 3);
	}
	else {
		wyslij_log(msgid_logger, "Pracownik nie wydał zamowienia, brak miejsca w restauracji", 3);
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

	shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count_max, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));
	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
	auto *table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));

	msgid_zam = msg_create(ftok(".", 'Z'), 0666);
	msgid_logger = msg_create(ftok(".", 'L'), 0666);
	semid = sem_create(ftok(".", 'W'), 2, 0666);
	semid_gk = sem_create(ftok(".", 'G'), 4, 0666);
	msgid_kierownik = msg_create(ftok(".", 'I'), 0666);
	table_sem_id = sem_create(ftok(".", 'M'), shared_mem_flags->max_table_count, 0666);

	wyslij_log(msgid_logger, "Pracownik rozpoczyna prace", 3);

	while (!shared_mem_flags->end_program && fire_sig_flag == 0 && !shared_mem_flags->all_customers_out) {
		if (sem_op(semid, 0, -1, &sig_flag) == -1) {
			if (sig_flag == 4) {
				wyslij_log(msgid_logger, "Pracownik czeka na wyjscie wszystkich klientow", 3);
				break;
			}
			if (sig_flag == 3) {
				wyslij_log(msgid_logger, "Pracownik czeka na ewakuacje klientow", 3);
				break;
			}
		}
		if (sig_flag == 1) {
			sig_flag = 0;
			extra(table_array, shared_mem_flags);
			wyslij_log(msgid_logger, "Pracownik doniosl stoly", 3);
		}
		else if (sig_flag == 2) {
			sig_flag = 0;
			KierownikRezerwacja re{};
			if (msg_recv(msgid_kierownik, &re, sizeof(re.reserved), REZERWACJE, 0, &fire_sig_flag) == -1) {
				break;
			}
			rezerwacje(re, table_array, shared_mem_flags);
			wyslij_log(msgid_logger, "Pracownik zarezerwowal stoly", 3);
		}
		zamowienie();
	}

	wyslij_log(msgid_logger, "Pracownik czeka az klienci opuszcza lokal", 5);
	sem_op(semid_gk, 2, -1);
	wyslij_log(msgid_logger, "Pracownik konczy prace", 5);

	shm_detach(base);

	return 0;
}
