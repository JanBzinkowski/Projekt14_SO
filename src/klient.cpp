#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include <vector>
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

struct ThreadArgs {
    Zamowienie *zamowienie;
};

void *decyzja_menu(void *args) {
    auto zamowienie = (ThreadArgs *) args;
    zamowienie->zamowienie->nr_napoju = rand() % MENU_NAPOJE + 1;
    zamowienie->zamowienie->nr_pozycji_menu = rand() % MENU_POSILKI + 1;
    return nullptr;
}

void *jedzenie(void *args) {
    auto zamowienie = (ThreadArgs *) args;
    sleep(zamowienie->zamowienie->nr_pozycji_menu);
    wyslij_log(msgid_logger, "Klient skonczyl jesc danie.");

    sleep(zamowienie->zamowienie->nr_napoju);
    wyslij_log(msgid_logger, "Klient skonczyl pic napoj.");
    return nullptr;
}

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, ZlozenieZamowienia *zam) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        int table_sem_id = sem_create(ftok(".", 'M'), table_count, 0666);
        sem_op(table_sem_id, zwrot->nr_stolika, zam->liczba_osob);
    }
    wyslij_log(msgid_logger, "Grupa klientow opuscila restauracje, stolik nr: " + (zwrot ? std::to_string(zwrot->nr_stolika) : "brak zamowienia") + ", rozmiar grupy: " + std::to_string(zam->liczba_osob));
}

void zwrot_naczyn(ZamowienieZwrot *zwrot) {
    sleep(2);
    wyslij_log(msgid_logger, "Grupa klientow oddala naczynia, stolik nr: " + std::to_string(zwrot->nr_stolika));
}

void zamowienie(ZlozenieZamowienia *zam, ZamowienieZwrot *zwrot) {
    semid = sem_create(ftok(".", 'K'), 2, 0666);
    sem_op(semid, 1, 1);

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

    msg_zamowienie msg{};
    msg.mtype = ZAMOWIENIE;
    msg.zam = *zam;

    msg_send(msgid_zam, &msg, sizeof(ZlozenieZamowienia), 0);
    wyslij_log(msgid_logger, "Grupa klientow [" + std::to_string(getpid()) + "]zlozyla zamowienie: " + std::to_string(zam->liczba_osob) +
                             " osob, Klient 1: pozycja: " + std::to_string(zam->zamowienie1.nr_pozycji_menu) +
                             ", napoj: " + std::to_string(zam->zamowienie1.nr_napoju)
                             + (zam->liczba_osob >= 2
                                    ? (", Klient 2: pozycja: " + std::to_string(zam->zamowienie2.nr_pozycji_menu) +
                                       ", napoj: " + std::to_string(zam->zamowienie1.nr_napoju))
                                    : "")
                             + (zam->liczba_osob >= 3
                                    ? (", Klient 3: pozycja: " + std::to_string(zam->zamowienie3.nr_pozycji_menu) +
                                       ", napoj: " + std::to_string(zam->zamowienie1.nr_napoju))
                                    : "")
                             + (zam->liczba_osob == 4
                                    ? (", Klient 4: pozycja: " + std::to_string(zam->zamowienie4.nr_pozycji_menu) +
                                       ", napoj: " + std::to_string(zam->zamowienie1.nr_napoju))
                                    : ""));

    msg_zwrot zw_msg{};
    if (msg_recv(msgid_zam, &zw_msg, sizeof(ZamowienieZwrot), getpid(), 0, &sig_flag) == -1) {
        return;
    }
    zwrot->nr_stolika = zw_msg.zwrot.nr_stolika;

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
    msgid_zam = msg_create(ftok(".", 'Z'), 0666);
    int logger_semid = sem_create(ftok(".", 'P'), 1, 0666);
    sem_op(logger_semid, 0, 1);

    ZlozenieZamowienia zam{};
    zam.pid = getpid();
    zam.liczba_osob = rand() % 4 + 1;

    if (0.05 < rand() / RAND_MAX) {
        zamowienie(&zam, nullptr);
        return 0;
    }
    std::vector<pthread_t> tids;
    std::vector<ThreadArgs> thread_args{{&zam.zamowienie1}, {&zam.zamowienie2}, {&zam.zamowienie3}, {&zam.zamowienie4}};
    for (int i = 0; i < zam.liczba_osob; i++) {
        pthread_t tid_t;
        if (pthread_create(&tid_t, nullptr, decyzja_menu, &thread_args[i]) != 0) {
            ipc_die("pthread_create");
        }
        tids.push_back(tid_t);
    }

    for (const auto tid: tids) {
        pthread_join(tid, nullptr);
    }

    wyslij_log(msgid_logger, "Grupa klientow zostala utworzona, rozmiar grupy: " + std::to_string(zam.liczba_osob));

    ZamowienieZwrot zwrot{};
    zamowienie(&zam, &zwrot);
    if (sig_flag) {
        opuszczenie_lokalu(nullptr, &zam);
        return 0;
    }
    if (zwrot.nr_stolika < 0) {
        wyslij_log(msgid_logger, "Klient nie znalazl stolika i wyszedl. [" + std::to_string(getpid()) + "]");
        return 0;
    }

    wyslij_log(msgid_logger, "Klient odebral zamowienie, stolik nr: " + std::to_string(zwrot.nr_stolika) + "[" + std::to_string(getpid()) + "]");
    tids.erase(tids.begin(), tids.end());
    for (int i = 0; i < zam.liczba_osob; i++) {
        pthread_t tid_t;
        if (pthread_create(&tid_t, nullptr, decyzja_menu, &thread_args[i]) != 0) {
            ipc_die("pthread_create");
        }
        tids.push_back(tid_t);
    }

    for (const auto tid: tids) {
        pthread_join(tid, nullptr);
    }

    zwrot_naczyn(&zwrot);

    opuszczenie_lokalu(&zwrot, &zam);

    sleep(1);

    sem_op(logger_semid, 0, -1);

    return 0;
}
