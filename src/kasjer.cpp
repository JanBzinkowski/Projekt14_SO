#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <vector>
#include <chrono>
#include <thread>

#include "../include/zamowienie.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"
#include "../include/wrappers.h"

int shmid, semid, semid_gk, semid_prac, msgid_zam, msgid_logger, table_sem_id;

volatile sig_atomic_t exception_flag = 0;


void handler(int const sig) {
    if (sig == SIGRTMIN || sig == SIGINT) {
        exception_flag = 1;
    }
}

bool sprawdz_stolik(int &index, Table *table_array, ZlozenieZamowienia const &zam) {
    if (zam.liczba_osob <= 2) {
        for (index = X1 + X2 - 1; index < table_count; index++) {
            if (sem_op(semid_prac, 1, -1, &exception_flag) == -1) {
                return false;
            }

            bool ok = !table_array[index].zarezerwowany && table_array[index].rozmiar_grupy == zam.liczba_osob;

            sem_op(semid_prac, 1, 1);

            if (!ok) {
                continue;
            }

            if (sem_op(table_sem_id, index, -zam.liczba_osob, nullptr, SEM_UNDO | IPC_NOWAIT) == -1) {
                continue;
            }

            wyslij_log(msgid_logger, "Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");

            return true;
        }
    }
    for (index = 0; index < table_count; index++) {
        if (sem_op(semid_prac, 1, -1, &exception_flag) == -1) {
            return false;
        }

        bool ok = !table_array[index].zarezerwowany && (table_array[index].rozmiar_grupy == 0);

        sem_op(semid_prac, 1, 1);

        if (!ok) {
            continue;
        }

        if (sem_op(table_sem_id, index, -zam.liczba_osob, nullptr, SEM_UNDO | IPC_NOWAIT) == -1) {
            continue;
        }

        wyslij_log(msgid_logger, "Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");

        return true;
    }
    return false;
}

void zamowienie(Table *table_array, std::vector<ZlozenieZamowienia> *kolejka, bool const nowa_wiadomosc = true) {
    msg_zamowienie msg{};
    ZlozenieZamowienia wybrane;
    bool ma_wybrane = false;

    if (nowa_wiadomosc) {
        if (msg_recv(msgid_zam, &msg, sizeof(msg.zam), ZAMOWIENIE, 0, &exception_flag) == -1) {
            return;
        }
        wybrane = msg.zam;
        ma_wybrane = true;
    }

    wyslij_log(msgid_logger, "Kasjer rozpoczyna obsluge klienta");

    int index = -1;

    if (ma_wybrane) {
        if (!sprawdz_stolik(index, table_array, wybrane)) {
            kolejka->push_back(wybrane);
        }
    }

    if (index == -1) {
        for (auto it = kolejka->begin(); it != kolejka->end(); ++it) {
            if (sprawdz_stolik(index, table_array, *it)) {
                wybrane = *it;
                kolejka->erase(it);
                break;
            }
        }
    }

    if (index == -1) {
        wyslij_log(msgid_logger, "Kasjer nie znal stolika dla grupy");
        return;
    }

    msg_pracownik pracownik{ZAMOWIENIE_PRACOWNIK, {index, wybrane.pid, wybrane.zamowienie1, wybrane.zamowienie2, wybrane.zamowienie3, wybrane.zamowienie4}};

    msg_send(msgid_zam, &pracownik, sizeof(pracownik.zwrot), 0);
    sem_op(semid_prac, 0, 1);

    wyslij_log(msgid_logger, "Kasjer skonczyl obsluge klienta. Przydzielony stolik: " + std::to_string(index));
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

    semid = sem_create(ftok(".", 'K'), 2, 0666);

    sem_op(semid, 0, 1);
    msgid_zam = msg_create(ftok(".", 'Z'), 0666);
    msgid_logger = msg_create(ftok(".", 'L'), 0666);
    semid_prac = sem_create(ftok(".", 'W'), 2, 0666);
    semid_gk = sem_create(ftok(".", 'G'), 3, 0666);
    table_sem_id = sem_create(ftok(".", 'M'), static_cast<int>(shared_mem_flags->tables_array_size / sizeof(Table)), 0666);

    wyslij_log(msgid_logger, "Kasjer rozpoczyna prace");

    std::vector<ZlozenieZamowienia> kolejka;

    using clock = std::chrono::steady_clock;
    auto last_handle = clock::now();

    while (!shared_mem_flags->end_program && exception_flag == 0 && !shared_mem_flags->all_customers_out) {
        if (!kolejka.empty() && sem_getval(semid, 1) == 0) {
            if (clock::now() - last_handle >= std::chrono::seconds(1)) {
                if (exception_flag == 1) {
                    break;
                }
                zamowienie(table_array, &kolejka, false);
                last_handle = clock::now();
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (sem_op(semid, 1, -1, &exception_flag) == -1) {
            break;
        }
        zamowienie(table_array, &kolejka);
        last_handle = clock::now();
    }

    if (exception_flag == 1) {
        wyslij_log(msgid_logger, "Kasjer czeka az klienci sie ewakuuja", 4);
    }
    else {
        wyslij_log(msgid_logger, "Kasjer czeka az klienci opuszcza lokal", 4);
    }


    sem_op(semid_gk, 2, -1);

    wyslij_log(msgid_logger, "Kasjer zamyka kase", 4);

    wyslij_log(msgid_logger, "Kasjer konczy prace", 4);

    shm_detach(base);
    return 0;
}
