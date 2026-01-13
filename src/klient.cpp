#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <csignal>
#include "../include/zamowienie.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, msgid_zam, msgid_zwrot;
int msgid_logger, semid_logger;

volatile sig_atomic_t sig_flag = 0;

void handler(int sig) {
    if (sig == SIGRTMIN || sig == SIGINT) {
        sig_flag = 1;
    }
}

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, Zamowienie *zam) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        int table_sem_id = sem_create(ftok(".", 'M'), table_count, 0666);
        sem_op(table_sem_id, zwrot->nr_stolika, zam->liczba_osob);
    }
    wyslij_log(msgid_logger, "Klient opuscil restauracje, stolik nr: " + (zwrot ? std::to_string(zwrot->nr_stolika) : "brak zamowienia") + ", rozmiar grupy: " + std::to_string(zam->liczba_osob));
}

void zwrot_naczyn(ZamowienieZwrot *zwrot) {
    sleep(2);
    wyslij_log(msgid_logger, "Klient oddal naczynia, stolik nr: " + std::to_string(zwrot->nr_stolika));
}

void zamowienie(Zamowienie *zam, ZamowienieZwrot *zwrot) {
    semid = sem_create(ftok(".", 'K'), 2, 0666);

    sem_op(semid, 0, -1, &sig_flag);
    if (sig_flag) {
        sem_op(semid, 0, 1);
        return;
    }

    if (!zwrot) {
        opuszczenie_lokalu(nullptr, zam);
        sem_op(semid, 0, 1);
        return;
    }

    sem_op(semid, 1, 1);

    msg_zamowienie msg{};
    msg.mtype = ZAMOWIENIE;
    msg.zam = *zam;
    msgid_zam = msg_create(ftok(".", 'Z'), 0666);

    msg_send(msgid_zam, &msg, sizeof(Zamowienie), 0);
    wyslij_log(msgid_logger, "Klient zlozyl zamowienie: " + std::to_string(zam->liczba_osob) +
                             " osob, pozycja: " + std::to_string(zam->nr_pozycji_menu) +
                             ", napoj: " + std::to_string(zam->nr_napoju));

    msg_zwrot zw_msg{};
    if (msg_recv(msgid_zam, &msg, sizeof(Zamowienie), ZAMOWIENIE, 0, &sig_flag) == -1) {
        return;
    }

    *zwrot = zw_msg.zwrot;

    sem_op(semid, 0, 1);
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGRTMIN, &sa, nullptr);

    srand(time(nullptr));
    msgid_logger = msg_create(ftok(".", 'L'), 0666);

    Zamowienie zam{};
    zam.liczba_osob = rand() % 4 + 1;

    if (0.05 < rand() / RAND_MAX) {
        zamowienie(&zam, nullptr);
        return 0;
    }

    zam.nr_pozycji_menu = rand() % MENU_POSILKI + 1;
    zam.nr_napoju = rand() % MENU_NAPOJE + 1;

    wyslij_log(msgid_logger, "Klient utworzony, rozmiar grupy: " + std::to_string(zam.liczba_osob));

    ZamowienieZwrot zwrot{};
    zamowienie(&zam, &zwrot);
    if (sig_flag) {
        opuszczenie_lokalu(nullptr, &zam);
        return 0;
    }
    if (zwrot.nr_stolika < 0) {
        wyslij_log(msgid_logger, "Klient nie znalazl stolika i wyszedl.");
        return 0;
    }

    wyslij_log(msgid_logger, "Klient odebral zamowienie, stolik nr: " + std::to_string(zwrot.nr_stolika));

    sleep(zam.nr_pozycji_menu);
    wyslij_log(msgid_logger, "Klient skonczyl jesc danie.");

    sleep(zam.nr_napoju);
    wyslij_log(msgid_logger, "Klient skonczyl pic napoj.");

    zwrot_naczyn(&zwrot);

    opuszczenie_lokalu(&zwrot, &zam);

    sleep(1);

    return 0;
}
