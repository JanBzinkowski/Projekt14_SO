#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, msgid_zam;
int msgid_logger;

void zamowienie() {
	msg_pracownik msg{};
	msg_recv(msgid_zam, &msg, sizeof(ZamowieniePracownik), ZAMOWIENIE_PRACOWNIK, 0);

	wyslij_log(msgid_logger, "Pracownik rozpoczyna obsluge klienta. Przydzielony stolik: stolik nr. " + std::to_string(msg.zwrot.nr_stolika));

	msg_zwrot zw{};
	zw.mtype = ZAMOWIENIE_ZWROT;
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
	key_t shm_key = ftok(".", 'S');
	shmid = shm_create(shm_key, sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));

	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);

	key_t msg_key_zam = ftok(".", 'Z');
	msgid_zam = msg_create(msg_key_zam, 0666);

	key_t msg_key_log = ftok(".", 'L');
	msgid_logger = msg_create(msg_key_log, 0666);

	key_t sem_key = ftok(".", 'WZ');
	semid = sem_create(sem_key, 1, 0666);

	wyslij_log(msgid_logger, "Pracownik rozpoczyna prace");

	while (!shared_mem_flags->end_program) {
		sem_op(semid, 0, -1);
		zamowienie();
	}

	wyslij_log(msgid_logger, "Pracownik konczy prace");

	shm_detach(shared_mem_flags);
	return 0;
}
