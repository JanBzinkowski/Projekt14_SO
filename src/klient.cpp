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

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, Zamowienie *zam) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        key_t sem_key = ftok(".", 'M');
        int table_sem_id = sem_create(sem_key, table_count, 0666);

        for (int i = 0; i < zam->liczba_osob; i++)
            sem_op(table_sem_id, zwrot->nr_stolika, 1);
    }
}

void zwrot_naczyn() {}

void zamowienie(Zamowienie *zam, ZamowienieZwrot *zwrot) {
    key_t sem_key = ftok(".", 'K');
    semid = sem_create(sem_key, 2, 0666);

    sem_op(semid, 0, -1);

    if (!zam) {
        opuszczenie_lokalu(nullptr, nullptr);
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
    wyslij_log("Klient zlozyl zamowienie: " + std::to_string(zam->liczba_osob) +
               " osob, pozycja: " + std::to_string(zam->nr_pozycji_menu) +
               ", napoj: " + std::to_string(zam->nr_napoju));

    key_t msg_key_zw = ftok(".", 'W');
    msgid_zwrot = msg_create(msg_key_zw, 0666);

    msg_zwrot zw_msg{};
    msg_recv(msgid_zwrot, &zw_msg, sizeof(ZamowienieZwrot), ZAMOWIENIE_ZWROT, 0);

    *zwrot = zw_msg.zwrot;

    sem_op(semid, 0, 1);
}

int main() {
    srand(time(nullptr));

    key_t msg_key_log = ftok(".", 'L');
    msgid_logger = msg_create(msg_key_log, 0666);

    key_t sem_key_log = ftok(".", 'LS');
    semid_logger = sem_create(sem_key_log, 1, 0666);
    sem_set(semid_logger, 0, 0);

    Zamowienie zam{};
    zam.liczba_osob = rand() % 4 + 1;
    zam.nr_pozycji_menu = rand() % 10 + 1;
    zam.nr_napoju = rand() % 5 + 1;

    wyslij_log("Klient utworzony, rozmiar grupy: " + std::to_string(zam.liczba_osob));

    ZamowienieZwrot zwrot{};
    zamowienie(&zam, &zwrot);

    if (zwrot.nr_stolika < 0) {
        wyslij_log("Klient nie znal stolika i wyszedl.");
        return -1;
    }

    wyslij_log("Klient odebral zamowienie, stolik nr: " + std::to_string(zwrot.nr_stolika));

    sleep(zam.nr_pozycji_menu);
    wyslij_log("Klient skonczyl jesc danie nr: " + std::to_string(zam.nr_pozycji_menu));

    sleep(zam.nr_napoju);
    wyslij_log("Klient skonczyl napoj nr: " + std::to_string(zam.nr_napoju));

    zwrot_naczyn();
    wyslij_log("Klient oddal naczynia, stolik nr: " + std::to_string(zwrot.nr_stolika));

    opuszczenie_lokalu(&zwrot, &zam);
    wyslij_log("Klient opuscil restauracje, stolik nr: " + std::to_string(zwrot.nr_stolika));

    sleep(1);
    return 0;
}
