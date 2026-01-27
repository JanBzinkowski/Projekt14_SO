#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include "../include/zamowienie.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

int shmid, semid, msgid_zam, genid;
int msgid_logger;

volatile sig_atomic_t sig_flag = 0;

void handler(int sig) {
    if (sig == SIGRTMIN || sig == SIGINT) {
        sig_flag = 1;
    }
}

static uint64_t make_seed() {
    std::random_device rd;
    uint64_t seed = ((uint64_t) rd() << 32) ^ (uint64_t) rd();
    seed ^= (uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count();
    seed ^= (uint64_t) getpid();
    seed ^= (uint64_t) (uintptr_t) pthread_self();
    return seed;
}

static std::mt19937_64 &rng() {
    thread_local static std::mt19937_64 gen(make_seed());
    return gen;
}

static int rand_range(int a, int b) {
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng());
}

static double rand_double() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng());
}

struct ThreadArgs {
    Zamowienie *zamowienie;
    bool czy_zwraca = false;
    ZamowienieZwrot *zwrot = nullptr;
};


void zwrot_naczyn(ZamowienieZwrot *zwrot) {
    sleep(2);
    wyslij_log(msgid_logger, "Grupa klientow oddala naczynia, stolik nr: " + std::to_string(zwrot->nr_stolika), 2);
}


void *decyzja_menu(void *args) {
    auto zamowienie = (ThreadArgs *) args;
    zamowienie->zamowienie->nr_napoju = static_cast<int8_t>(rand_range(1, MENU_NAPOJE));
    zamowienie->zamowienie->nr_pozycji_menu = static_cast<int8_t>(rand_range(1, MENU_POSILKI));
    return nullptr;
}

void *jedzenie(void *args) {
    auto zamowienie = (ThreadArgs *) args;
    sleep(zamowienie->zamowienie->nr_pozycji_menu + zamowienie->zamowienie->nr_napoju);

    if (sig_flag == 0) {
        wyslij_log(msgid_logger, "Klient skonczyl swoj posilek", 2);
    }
    if (zamowienie->czy_zwraca && sig_flag == 0) {
        zwrot_naczyn(zamowienie->zwrot);
    }
    return nullptr;
}

void opuszczenie_lokalu(ZamowienieZwrot *zwrot, ZlozenieZamowienia *zam, Table *table, SharedMem *mem_flags) {
    if (zwrot && zwrot->nr_stolika >= 0) {
        int table_sem_id = sem_create(ftok(".", 'M'), mem_flags->max_table_count, 0666);
        sem_op(table_sem_id, zwrot->nr_stolika, zam->liczba_osob);
        int semid_prac = sem_create(ftok(".", 'W'), 2, 0666);
        if (sem_op(semid_prac, 1, -1, &sig_flag) == -1) {
            return;
        }
        if (sem_getval(table_sem_id, zwrot->nr_stolika) == table[zwrot->nr_stolika].max_osob) {
            table[zwrot->nr_stolika].typ_grupy = 0;
        }
        sem_op(semid_prac, 1, 1);
    }
    if (sig_flag == 0) {
        wyslij_log(msgid_logger, "Grupa klientow opuscila restauracje, stolik nr: " + (zwrot ? std::to_string(zwrot->nr_stolika) : "brak zamowienia") + ", rozmiar grupy: " + std::to_string(zam->liczba_osob), 2);
    }
    sem_op(genid, 3, 1);
}

void zamowienie(ZlozenieZamowienia *zam, ZamowienieZwrot *zwrot) {
    sem_op(semid, 1, 1);

    msg_zamowienie msg{};
    msg.mtype = ZAMOWIENIE;
    msg.zam = *zam;
    if (sem_op(semid, 0, -1, &sig_flag) == -1) {
        return;
    }
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
                                    : ""), 2);

    msg_zwrot zw_msg{};

    if (msg_recv(msgid_zam, &zw_msg, sizeof(ZamowienieZwrot), getpid(), 0, &sig_flag) == -1) {
        return;
    }
    *zwrot = zw_msg.zwrot;
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGRTMIN, &sa, nullptr);

    shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count_max, 0666);
    auto *base = static_cast<char *>(shm_attach(shmid, 0));
    auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
    auto *table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));

    msgid_logger = msg_create(ftok(".", 'L'), 0666);
    msgid_zam = msg_create(ftok(".", 'Z'), 0666);
    semid = sem_create(ftok(".", 'K'), 2, 0666);
    genid = sem_create(ftok(".", 'G'), 4, 0666);

    if (sem_op(genid, 3, -1, &sig_flag) == -1) {
        return 0;
    }

    ZlozenieZamowienia zam{};
    zam.pid = getpid();
    zam.liczba_osob = static_cast<int8_t>(rand_range(1, 4));

    if (0.05 > rand_double()) {
        opuszczenie_lokalu(nullptr, &zam, table_array, shared_mem_flags);
        shm_detach(base);
        return 0;
    }
    std::vector<pthread_t> tids;
    std::vector<ThreadArgs> thread_args{{&zam.zamowienie1, true}, {&zam.zamowienie2}, {&zam.zamowienie3}, {&zam.zamowienie4}};
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

    wyslij_log(msgid_logger, "Grupa klientow zostala utworzona, rozmiar grupy: " + std::to_string(zam.liczba_osob), 2);

    ZamowienieZwrot zwrot;
    zwrot.nr_stolika = -1;
    zamowienie(&zam, &zwrot);
    thread_args[1].zwrot = &zwrot;
    if (sig_flag) {
        opuszczenie_lokalu(nullptr, &zam, table_array, shared_mem_flags);
        shm_detach(base);
        return 0;
    }
    if (zwrot.nr_stolika < 0) {
        if (sig_flag == 0) {
            wyslij_log(msgid_logger, "Klient nie znalazl stolika i wyszedl. [" + std::to_string(getpid()) + "]", 2);
        }
        shm_detach(base);
        return 0;
    }

    wyslij_log(msgid_logger, "Klient odebral zamowienie, stolik nr: " + std::to_string(zwrot.nr_stolika) + "[" + std::to_string(getpid()) + "]", 2);
    tids.erase(tids.begin(), tids.end());
    for (int i = 0; i < zam.liczba_osob; i++) {
        pthread_t tid_t;
        if (pthread_create(&tid_t, nullptr, jedzenie, &thread_args[i]) != 0) {
            ipc_die("pthread_create");
        }
        tids.push_back(tid_t);
    }

    for (const auto tid: tids) {
        pthread_join(tid, nullptr);
    }

    opuszczenie_lokalu(&zwrot, &zam, table_array, shared_mem_flags);

    shm_detach(base);
    return 0;
}
