#include <iostream>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <cstdlib>
#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/sem_ops.h"

int shmid, semid, msgid_zam, msgid_zwrot;

void zamowienie(SharedMem *shared_mem_flags, Table *table_array) {
    msg_zamowienie msg;
    if (msgrcv(msgid_zam, &msg, sizeof(Zamowienie), 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    key_t sem_key = ftok(".", 'M');
    int table_sem_id = semget(sem_key, table_count, 0666);
    if (table_sem_id == -1) {
        perror("semget");
        exit(1);
    }

    int index = 0;
    for (; index < table_count; index++) {
        int free = semctl(table_sem_id, index, GETVAL);
        if (free >= msg.zam.liczba_osob &&
            (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == msg.zam.liczba_osob)) {
            for (int i = 0; i < msg.zam.liczba_osob; i++)
                sem_wait(table_sem_id, index);
            break;
        }
    }

    msg_zwrot zw;
    zw.mtype = ZAMOWIENIE_ZWROT;
    zw.zwrot.nr_stolika = (index == table_count) ? -1 : index;

    if (msgsnd(msgid_zwrot, &zw, sizeof(ZamowienieZwrot), 0) == -1) {
        perror("msgsnd zwrot");
        exit(1);
    }
}

int main() {
    key_t shm_key = ftok(".", 'S');
    shmid = shmget(shm_key, sizeof(SharedMem) + sizeof(Table) * table_count, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    void *base = shmat(shmid, nullptr, 0);
    if (base == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    auto *shared_mem_flags = (SharedMem *) base;
    auto *table_array = (Table *) ((char *) base + sizeof(SharedMem));

    key_t sem_key = ftok(".", 'K');
    semid = semget(sem_key, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    semctl(semid, 0, SETVAL, 1);
    semctl(semid, 1, SETVAL, 0);

    key_t msg_key_zam = ftok(".", 'Z');
    msgid_zam = msgget(msg_key_zam, IPC_CREAT | 0666);
    if (msgid_zam == -1) {
        perror("msgget zam");
        exit(1);
    }

    key_t msg_key_zw = ftok(".", 'W');
    msgid_zwrot = msgget(msg_key_zw, IPC_CREAT | 0666);
    if (msgid_zwrot == -1) {
        perror("msgget zwrot");
        exit(1);
    }

    while (!shared_mem_flags->end_program) {
        sem_wait(semid, 1);
        zamowienie(shared_mem_flags, table_array);
    }

    shmdt(base);
    return 0;
}
