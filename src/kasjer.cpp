#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, semid_prac, msgid_zam;
int msgid_logger;


void zamowienie(SharedMem *shared_mem_flags, Table *table_array) {
    msg_zamowienie msg{};
    msg_recv(msgid_zam, &msg, sizeof(Zamowienie), ZAMOWIENIE, 0);

    wyslij_log(msgid_logger, "Kasjer rozpoczyna obsluge klienta, grupa " + std::to_string(msg.zam.liczba_osob) + " osob");

    key_t sem_key = ftok(".", 'M');
    int table_sem_id = sem_create(sem_key, table_count, 0666);

    int index = 0;
    for (; index < table_count; index++) {
        int free = sem_getval(table_sem_id, index);
        if (free >= msg.zam.liczba_osob && table_array[index].zarezerwowany == false &&
            (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == msg.zam.liczba_osob)) {
            sem_op(table_sem_id, index, -msg.zam.liczba_osob);
            wyslij_log(msgid_logger, "Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");
            break;
        }
    }

    msg_pracownik pracownik{};
    pracownik.mtype = ZAMOWIENIE_PRACOWNIK;
    pracownik.zwrot.nr_stolika = (index == table_count) ? -1 : index;
    pracownik.zwrot.nr_napoju = msg.zam.liczba_osob;
    pracownik.zwrot.nr_pozycji_menu = msg.zam.nr_pozycji_menu;

    msg_send(msgid_zam, &pracownik, sizeof(ZamowieniePracownik), 0);

    sem_op(semid_prac, 0, 1);

    if (pracownik.zwrot.nr_stolika != -1) {
        wyslij_log(msgid_logger, "Kasjer skonczyl obslugiwac klienta. Przydzielony stolik: stolik nr: " + std::to_string(pracownik.zwrot.nr_stolika));
    }
    else {
        wyslij_log(msgid_logger, "Kasjer nie znal stolika dla grupy");
    }
}

int main() {
    key_t shm_key = ftok(".", 'S');
    shmid = shm_create(shm_key, sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
    auto *base = static_cast<char *>(shm_attach(shmid, 0));

    auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
    auto *table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));

    key_t sem_key = ftok(".", 'K');
    semid = sem_create(sem_key, 2, IPC_CREAT | 0666);

    sem_set(semid, 0, 1);
    sem_set(semid, 1, 0);

    key_t msg_key_zam = ftok(".", 'Z');
    msgid_zam = msg_create(msg_key_zam, IPC_CREAT | 0666);

    key_t msg_key_log = ftok(".", 'L');
    msgid_logger = msg_create(msg_key_log, 0666);

    key_t sem_key_prac = ftok(".", 'WZ');
    semid_prac = sem_create(sem_key_prac, 1, 0666);

    wyslij_log(msgid_logger, "Kasjer rozpoczyna prace");

    while (!shared_mem_flags->end_program) {
        sem_op(semid, 1, -1);
        zamowienie(shared_mem_flags, table_array);
    }

    wyslij_log(msgid_logger, "Kasjer konczy prace");

    shm_detach(base);
    return 0;
}
