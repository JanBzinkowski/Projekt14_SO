#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <cstring>

#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

std::vector<pid_t> pids;

void handler(int sig) {
    if (sig == SIGINT) {
        for (pid_t pid: pids) {
            kill(pid, SIGINT);
        }
    }
    else if (sig == SIGRTMIN) {
        for (pid_t pid: pids) {
            kill(pid, SIGRTMIN);
        }
    }
    else if (sig == SIGRTMIN + 1) {
        kill(pids[2], SIGRTMIN + 1);
    }
    else if (sig == SIGRTMIN + 2) {
        kill(pids[2], SIGRTMIN + 2);
    }
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGRTMIN, &sa, nullptr);
    sigaction(SIGRTMIN + 1, &sa, nullptr);
    sigaction(SIGRTMIN + 2, &sa, nullptr);

    int msgid_logger = msg_create(ftok(".", 'L'), 0666 | IPC_CREAT);
    int logger_semid = sem_create(ftok(".", 'P'), 1, IPC_CREAT | 0666);
    int msgid_kierownik = msg_create(ftok(".", 'I'), 0666 | IPC_CREAT);
    int gen_semid = sem_create(ftok(".", 'G'), 4, 0666 | IPC_CREAT);
    int kasa_semid = sem_create(ftok(".", 'K'), 2, IPC_CREAT | 0666);
    int msgid_zam = msg_create(ftok(".", 'Z'), IPC_CREAT | 0666);
    int semid_prac = sem_create(ftok(".", 'W'), 2, 0666 | IPC_CREAT);
    sem_op(semid_prac, 1, 1);

    int max_x3 = (X3 == 0) ? 1 : (X3 * 2);
    table_count_max = X1 + X2 + max_x3 + X4;

    size_t table_size = sizeof(Table) * table_count_max;
    size_t total_size = sizeof(SharedMem) + table_size;

    int shmid = shm_create(ftok(".", 'S'), total_size, IPC_CREAT | 0666);
    void *base = shm_attach(shmid, 0);

    auto *shared_mem_flags = (SharedMem *) base;
    auto *table_array = (Table *) ((char *) base + sizeof(SharedMem));

    shared_mem_flags->all_customers_out = false;
    shared_mem_flags->new_customers = true;
    shared_mem_flags->end_program = false;
    shared_mem_flags->new_tables = false;
    shared_mem_flags->tables_array_size = table_size;

    int semid = sem_create(ftok(".", 'M'), static_cast<int>(table_size / sizeof(Table)), IPC_CREAT | 0666);

    for (int i = 0; i < X1; i++) {
        table_array[i].max_osob = 1;
        table_array[i].zarezerwowany_pzez_kierownika = false;
        table_array[i].typ_gruoy = 0;
    }
    for (int i = X1; i < X1 + X2; i++) {
        table_array[i].max_osob = 2;
        table_array[i].zarezerwowany_pzez_kierownika = false;
        table_array[i].typ_gruoy = 0;
    }
    for (int i = X1 + X2; i < X1 + X2 + X3; i++) {
        table_array[i].max_osob = 3;
        table_array[i].zarezerwowany_pzez_kierownika = false;
        table_array[i].typ_gruoy = 0;
    }
    for (int i = X1 + X2 + X3; i < X1 + X2 + X3 + X4; i++) {
        table_array[i].max_osob = 4;
        table_array[i].zarezerwowany_pzez_kierownika = false;
        table_array[i].typ_gruoy = 0;
    }

    for (int i = 0; i < table_count; i++)
        sem_op(semid, i, table_array[i].max_osob);

    pid_t pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        sem_op(logger_semid, 0, 1);
        execl("./generator_klientow", "generator_klientow", NULL);
        ipc_die("exec generator_klientow");
    }
    pids.push_back(pid);
    std::cerr << "Uruchomiono generator klientow: [" + std::to_string(pid) + "]" << std::endl;

    pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        sem_op(logger_semid, 0, 1);
        execl("./kasjer", "kasjer", NULL);
        ipc_die("exec kasjer");
    }
    pids.push_back(pid);
    std::cerr << "Uruchomiono kasjera: [" + std::to_string(pid) + "]" << std::endl;

    pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        sem_op(logger_semid, 0, 1);
        execl("./pracownik", "pracownik", NULL);
        ipc_die("exec pracownik");
    }
    pids.push_back(pid);
    std::cerr << "Uruchomiono pracownika: [" + std::to_string(pid) + "]" << std::endl;

    pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        execl("./logger", "logger", NULL);
        ipc_die("exec logger");
    }
    pids.push_back(pid);
    std::cerr << "Uruchomiono logger: [" + std::to_string(pid) + "]" << std::endl;

    for (const auto chpid: pids) {
        pid_t ret;
        do {
            ret = waitpid(chpid, nullptr, 0);
        } while (ret == -1 && errno == EINTR);

        if (chpid != pids[3]) {
            sem_op(logger_semid, 0, -1);
        }
        std::cerr << "zakonczono: [" + std::to_string(chpid) + "]" << std::endl;
    }

    shm_detach(base);
    shm_remove(shmid);

    sem_remove(semid_prac);
    sem_remove(logger_semid);
    sem_remove(semid);
    sem_remove(kasa_semid);
    sem_remove(gen_semid);

    msg_remove(msgid_kierownik);
    msg_remove(msgid_logger);
    msg_remove(msgid_zam);

    return 0;
}
