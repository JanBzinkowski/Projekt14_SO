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

int shmid, semid, semid_gk, semid_prac, msgid_zam, msgid_logger, table_sem_id, logger_semid;

volatile sig_atomic_t exception_flag = 0;
volatile sig_atomic_t exception_flag_fire = 0;


void handler(int const sig) {
    if (sig == SIGRTMIN) {
        exception_flag_fire = 1;
        exception_flag = 1;
    }
}

bool sprawdz_stolik(int &index, Table *table_array, ZlozenieZamowienia const &zam) {
    bool znaleziono_stolik = false;
    for (index = 0; index < table_count; index++) {
        sem_op(semid_prac, 1, -1, &exception_flag);

        int const free = sem_getval(table_sem_id, index);

        if (free >= zam.liczba_osob && table_array[index].zarezerwowany == false &&
            (table_array[index].rozmiar_grupy == 0 || table_array[index].rozmiar_grupy == zam.liczba_osob)) {
            sem_op(table_sem_id, index, -zam.liczba_osob);
            wyslij_log(msgid_logger, "Kasjer znalazl stolik nr: " + std::to_string(index) + " dla grupy. Stolik " + std::to_string(table_array[index].max_osob) + " osobowy");
            znaleziono_stolik = true;
            sem_op(semid_prac, 1, 1);
            break;
        }
        sem_op(semid_prac, 1, 1);
    }
    return znaleziono_stolik;
}

void zamowienie(SharedMem *shared_mem_flags, Table *table_array, std::vector<ZlozenieZamowienia> *kolejka, bool const nowa_wiadomosc = true) {
    msg_zamowienie msg{};
    if (nowa_wiadomosc) {
        if (msg_recv(msgid_zam, &msg, sizeof(ZlozenieZamowienia), ZAMOWIENIE, 0, &exception_flag) == -1) {
            return;
        }
    }

    wyslij_log(msgid_logger, "Kasjer rozpoczyna obsluge klienta");

    bool znaleziono_stolik = false;
    int index;
    int pid = msg.zam.pid;
    do {
        if (nowa_wiadomosc) {
            if (!(znaleziono_stolik = sprawdz_stolik(index, table_array, msg.zam))) {
                kolejka->emplace_back(msg.zam.pid);
            }
        }
        for (auto const zamowienia: *kolejka) {
            pid = zamowienia.pid;
            if ((znaleziono_stolik = sprawdz_stolik(index, table_array, zamowienia))) {
                break;
            }
        }
    } while (!znaleziono_stolik && !exception_flag);

    msg_pracownik pracownik{};
    pracownik.mtype = ZAMOWIENIE_PRACOWNIK;
    pracownik.zwrot.nr_stolika = (index == table_count) ? -1 : index;

    if (pracownik.zwrot.nr_stolika != -1) {
        wyslij_log(msgid_logger, "Kasjer skonczyl obslugiwac klienta. Przydzielony stolik: stolik nr: " + std::to_string(pracownik.zwrot.nr_stolika));
        auto const it = std::find_if(kolejka->begin(), kolejka->end(), [pid](const ZlozenieZamowienia &z) {
            return z.pid == pid;
        });
        if (it == kolejka->end() && pid == msg.zam.pid) {
            pracownik.zwrot.pid = pid;
            pracownik.zwrot.zamowienie1 = msg.zam.zamowienie1;
            pracownik.zwrot.zamowienie2 = msg.zam.zamowienie2;
            pracownik.zwrot.zamowienie3 = msg.zam.zamowienie3;
            pracownik.zwrot.zamowienie4 = msg.zam.zamowienie4;

            pracownik.mtype = ZAMOWIENIE_PRACOWNIK;
            msg_send(msgid_zam, &pracownik, sizeof(ZamowieniePracownik), 0);
            sem_op(semid_prac, 0, 1);
        }
        else if (it == kolejka->end()) {
            wyslij_log(msgid_logger, "Blad odbierania zamowienia");
        }
        else {
            pracownik.zwrot.pid = pid;
            pracownik.zwrot.zamowienie1 = it->zamowienie1;
            pracownik.zwrot.zamowienie2 = it->zamowienie2;
            pracownik.zwrot.zamowienie3 = it->zamowienie3;
            pracownik.zwrot.zamowienie4 = it->zamowienie4;

            msg_send(msgid_zam, &pracownik, sizeof(ZamowieniePracownik), 0);
            sem_op(semid_prac, 0, 1);

            kolejka->erase(it);
        }
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

    semid = sem_create(ftok(".", 'K'), 2, 0666);

    sem_op(semid, 0, 1);
    msgid_zam = msg_create(ftok(".", 'Z'), 0666);
    msgid_logger = msg_create(ftok(".", 'L'), 0666);
    semid_prac = sem_create(ftok(".", 'W'), 2, 0666);
    semid_gk = sem_create(ftok(".", 'G'), 3, 0666);
    logger_semid = sem_create(ftok(".", 'P'), 1, 0666);
    table_sem_id = sem_create(ftok(".", 'M'), static_cast<int>(shared_mem_flags->tables_array_size / sizeof(Table)), 0666);
    sem_op(logger_semid, 0, 1);

    wyslij_log(msgid_logger, "Kasjer rozpoczyna prace");

    std::vector<ZlozenieZamowienia> kolejka;

    using clock = std::chrono::steady_clock;
    auto last_handle = clock::now();

    while (!shared_mem_flags->end_program) {
        if (!kolejka.empty() && sem_getval(semid, 1) == 0) {
            if (clock::now() - last_handle >= std::chrono::seconds(1)) {
                zamowienie(shared_mem_flags, table_array, &kolejka, false);
                last_handle = clock::now();
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        sem_op(semid, 1, -1, &exception_flag);
        if (exception_flag)
            break;

        zamowienie(shared_mem_flags, table_array, &kolejka);
        last_handle = clock::now();
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

    sem_op(logger_semid, 0, -1);

    shm_detach(base);
    return 0;
}
