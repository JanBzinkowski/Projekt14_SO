#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, msgid_zam, msgid_zwrot;
int msgid_logger, semid_logger;

void wyslij_log(const std::string &tekst) {
    if (tekst.empty())
        return;

    MsgText msg{};
    msg.mtype = 1;
    strncpy(msg.text, tekst.c_str(), sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';

    msg_send(msgid_logger, &msg, sizeof(msg.text), 0);
    sem_op(semid_logger, 0, 1);
}

void zamowienie(SharedMem *shared_mem_flags, Table *table_array) {
    msg_zamowienie msg{};
    msg_recv(msgid_zam, &msg, sizeof(Zamowienie), 0, 0);

    wyslij_log("Kasjer rozpoczyna obsluge klienta, grupa " + std::to_string(msg.zam.liczba_osob) + " osob");

    key_t sem_key = ftok(".", 'M');
    int table_sem_id = sem_create(sem_key, table_count, 0666);

    int index = 0;
    for (; index < table_count; index++) {
        int free = sem_getval(table_sem_id, index);
        if (free >= msg.zam.liczba_osob && table_array[index].zarezerwowany == false &&
            (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == msg.zam.liczba_osob)) {
            sem_op(table_sem_id, index, -msg.zam.liczba_osob);
            wyslij_log("Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");
            break;
        }
    }

    msg_zwrot zw{};
    zw.mtype = ZAMOWIENIE_ZWROT;
    zw.zwrot.nr_stolika = (index == table_count) ? -1 : index;

    msg_send(msgid_zwrot, &zw, sizeof(ZamowienieZwrot), 0);

    if (zw.zwrot.nr_stolika != -1) {
        wyslij_log("Kasjer wydal zamowienie, stolik nr: " + std::to_string(zw.zwrot.nr_stolika));
    }
    else {
        wyslij_log("Kasjer nie znal stolika dla grupy");
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

    key_t msg_key_zw = ftok(".", 'W');
    msgid_zwrot = msg_create(msg_key_zw, IPC_CREAT | 0666);

    key_t msg_key_log = ftok(".", 'L');
    msgid_logger = msg_create(msg_key_log, 0666);

    key_t sem_key_log = ftok(".", 'LS');
    semid_logger = sem_create(sem_key_log, 1, 0666);
    sem_set(semid_logger, 0, 0);

    wyslij_log("Kasjer rozpoczyna prace");

    while (!shared_mem_flags->end_program) {
        sem_op(semid, 1, -1);
        zamowienie(shared_mem_flags, table_array);
    }

    wyslij_log("Kasjer konczy prace");

    shm_detach(base);
    return 0;
}
