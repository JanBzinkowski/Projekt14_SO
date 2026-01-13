#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, semid_gk, semid_prac, msgid_zam, msgid_logger;

volatile sig_atomic_t exception_flag = 0;
volatile sig_atomic_t exception_flag_fire = 0;

void handler(int sig) {
    exception_flag = 1;
    if (sig == SIGRTMIN)
        exception_flag_fire = 1;
}

void zamowienie(SharedMem *shared_mem_flags, Table *table_array) {
    msg_zamowienie msg{};
    if (msg_recv(msgid_zam, &msg, sizeof(Zamowienie), ZAMOWIENIE, 0, &exception_flag) == -1) {
        return;
    }

    wyslij_log(msgid_logger, "Kasjer rozpoczyna obsluge klienta, grupa " + std::to_string(msg.zam.liczba_osob) + " osob");

    key_t sem_key = ftok(".", 'M');
    int table_sem_id = sem_create(sem_key, table_count, 0666);

    bool znaleziono_stolik = false;
    int index;
    do {
        index = 0;
        for (; index < table_count; index++) {
            int free = sem_getval(table_sem_id, index);
            if (free >= msg.zam.liczba_osob && table_array[index].zarezerwowany == false &&
                (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == msg.zam.liczba_osob)) {
                sem_op(table_sem_id, index, -msg.zam.liczba_osob);
                wyslij_log(msgid_logger, "Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");
                znaleziono_stolik = true;
                break;
            }
        }
    } while (!znaleziono_stolik && !exception_flag);

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
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGRTMIN, &sa, nullptr);

    shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
    auto *base = static_cast<char *>(shm_attach(shmid, 0));

    auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
    auto *table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));

    semid = sem_create(ftok(".", 'K'), 2, IPC_CREAT | 0666);

    sem_set(semid, 0, 1);
    sem_set(semid, 1, 0);
    msgid_zam = msg_create(ftok(".", 'Z'), IPC_CREAT | 0666);
    msgid_logger = msg_create(ftok(".", 'L'), 0666);
    semid_prac = sem_create(ftok(".", 'W'), 1, 0666 | IPC_CREAT);
    semid_gk = sem_create(ftok(".", 'G'), 2, 0666);

    wyslij_log(msgid_logger, "Kasjer rozpoczyna prace");

    while (!shared_mem_flags->end_program) {
        sem_op(semid, 1, -1, &exception_flag);
        if (exception_flag) {
            break;
        }
        zamowienie(shared_mem_flags, table_array);
    }

    if (exception_flag_fire) {
        wyslij_log(msgid_logger, "Kasjer czeka az klienci sie ewakuuja");
    }
    else {
        wyslij_log(msgid_logger, "Kasjer czeka az klienci opuszcza lokal");
    }

    sem_op(semid_gk, 1, -1);

    wyslij_log(msgid_logger, "Kasjer zamyka kase");

    wyslij_log(msgid_logger, "Kasjer konczy prace");

    shm_detach(shared_mem_flags);
    shm_detach(table_array);

    sem_remove(semid);
    sem_remove(semid_prac);
    return 0;
}
