#include <iostream>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <cstdlib>
#include <ctime>

#include "../include/zamowienie.h"
#include "../include/Tables.h"
#include "../include/sem_ops.h"

int shmid, semid, msgid_zam, msgid_zwrot;

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, Zamowienie *zam) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        key_t sem_key = ftok(".", 'M');
        int table_sem_id = semget(sem_key, table_count, 0666);
        if (table_sem_id == -1) {
            perror("semget");
            exit(1);
        }

        for (int i = 0; i < zam->liczba_osob; i++)
            sem_post(table_sem_id, zwrot->nr_stolika);
    }
}

void zwrot_naczyn() {}

void zamowienie(Zamowienie *zam, ZamowienieZwrot *zwrot) {
    key_t sem_key = ftok(".", 'K');
    semid = semget(sem_key, 2, 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    sem_wait(semid, 0);

    if (!zam) {
        opuszczenie_lokalu(nullptr, nullptr);
        sem_post(semid, 0);
        return;
    }

    sem_post(semid, 1);

    msg_zamowienie msg;
    msg.mtype = ZAMOWIENIE;
    msg.zam = *zam;

    key_t msg_key = ftok(".", 'Z');
    msgid_zam = msgget(msg_key, 0666);
    if (msgid_zam == -1) {
        perror("msgget zam");
        exit(1);
    }

    if (msgsnd(msgid_zam, &msg, sizeof(Zamowienie), 0) == -1) {
        perror("msgsnd zam");
        exit(1);
    }

    key_t msg_key_zw = ftok(".", 'W');
    msgid_zwrot = msgget(msg_key_zw, 0666);
    if (msgid_zwrot == -1) {
        perror("msgget zwrot");
        exit(1);
    }

    msg_zwrot zw_msg;
    if (msgrcv(msgid_zwrot, &zw_msg, sizeof(ZamowienieZwrot), ZAMOWIENIE_ZWROT, 0) == -1) {
        perror("msgrcv zwrot");
        exit(1);
    }

    *zwrot = zw_msg.zwrot;

    sem_post(semid, 0);
}

int main() {
    srand(time(nullptr));

    if (rand() / double(RAND_MAX) <= 0.05) {
        zamowienie(nullptr, nullptr);
        sleep(1);
    }

    Zamowienie zam;
    zam.liczba_osob = rand() % 4 + 1;
    zam.nr_pozycji_menu = rand() % 10 + 1;
    zam.nr_napoju = rand() % 5 + 1;

    ZamowienieZwrot zwrot;
    zamowienie(&zam, &zwrot);
    if (zwrot.nr_stolika < 0)
        return -1;

    sleep(zam.nr_pozycji_menu);
    sleep(zam.nr_napoju);

    zwrot_naczyn();
    opuszczenie_lokalu(&zwrot, &zam);
    sleep(1);

    return 0;
}
