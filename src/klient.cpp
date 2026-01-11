#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../include/zamowienie.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, msgid_zam, msgid_zwrot;
int msgid_logger, semid_logger;

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, Zamowienie *zam) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        key_t sem_key = ftok(".", 'M');
        int table_sem_id = sem_create(sem_key, table_count, 0666);
        sem_op(table_sem_id, zwrot->nr_stolika, zam->liczba_osob);
    }
    wyslij_log(msgid_logger, "Klient opuscil restauracje, stolik nr: " + (zwrot? "brak zamowienia" : std::to_string(zwrot->nr_stolika)) + ", rozmiar grupy: " + std::to_string(zam->liczba_osob));
}

void zwrot_naczyn(ZamowienieZwrot *zwrot) {
    sleep(2);
    wyslij_log(msgid_logger, "Klient oddal naczynia, stolik nr: " + std::to_string(zwrot->nr_stolika));
}

void zamowienie(Zamowienie *zam, ZamowienieZwrot *zwrot) {
    key_t sem_key = ftok(".", 'K');
    semid = sem_create(sem_key, 2, 0666);

    sem_op(semid, 0, -1);

    if (!zwrot) {
        opuszczenie_lokalu(nullptr, zam);
        sem_op(semid, 0, 1);
        return;
    }

    sem_op(semid, 1, 1);

    msg_zamowienie msg{};
    msg.mtype = ZAMOWIENIE;
    msg.zam = *zam;

    key_t msg_key = ftok(".", 'Z');
    msgid_zam = msg_create(msg_key, 0666);

    msg_send(msgid_zam, &msg, sizeof(Zamowienie), 0);
    wyslij_log(msgid_logger, "Klient zlozyl zamowienie: " + std::to_string(zam->liczba_osob) +
                             " osob, pozycja: " + std::to_string(zam->nr_pozycji_menu) +
                             ", napoj: " + std::to_string(zam->nr_napoju));

    msg_zwrot zw_msg{};
    msg_recv(msgid_zwrot, &zw_msg, sizeof(ZamowienieZwrot), ZAMOWIENIE_ZWROT, 0);

    *zwrot = zw_msg.zwrot;

    sem_op(semid, 0, 1);
}

int main() {
    srand(time(nullptr));

    key_t msg_key_log = ftok(".", 'L');
    msgid_logger = msg_create(msg_key_log, 0666);

    Zamowienie zam{};
    zam.liczba_osob = rand() % 4 + 1;

    if (0.05<rand()/RAND_MAX) {
        zamowienie(&zam, nullptr);
    }

    zam.nr_pozycji_menu = rand() % MENU_POSILKI + 1;
    zam.nr_napoju = rand() % MENU_NAPOJE + 1;

    wyslij_log(msgid_logger, "Klient utworzony, rozmiar grupy: " + std::to_string(zam.liczba_osob));

    ZamowienieZwrot zwrot{};
    zamowienie(&zam, &zwrot);

    if (zwrot.nr_stolika < 0) {
        wyslij_log(msgid_logger, "Klient nie znal stolika i wyszedl.");
        return -1;
    }

    wyslij_log(msgid_logger, "Klient odebral zamowienie, stolik nr: " + std::to_string(zwrot.nr_stolika));

    sleep(zam.nr_pozycji_menu);
    wyslij_log(msgid_logger, "Klient skonczyl jesc danie.");

    sleep(zam.nr_napoju);
    wyslij_log(msgid_logger, "Klient skonczyl napoj.");

    zwrot_naczyn(&zwrot);

    opuszczenie_lokalu(&zwrot, &zam);

    sleep(1);
    return 0;
}
