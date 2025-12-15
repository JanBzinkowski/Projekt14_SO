#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

//zamien ic wszystkie crr na perror

int main() {
    key_t shm_key = ftok(".", 'S');
    if (shm_key == -1) {
        perror("ftok");
        exit(1);
    }

    size_t table_size = sizeof(Table) * (table_count + X3 * 2);
    if (X3 == 0) {
        table_size += sizeof(Table);
    }
    size_t total_size = sizeof(SharedMem) + table_size;

    int shmid = shmget(shm_key, total_size, IPC_CREAT | 0666);
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

    shared_mem_flags->new_customers = true;
    shared_mem_flags->tables_array_size = table_size;

    key_t sem_key = ftok(".", 'M');
    if (sem_key == -1) {
        perror("ftok sem");
        exit(1);
    }

    int semid = semget(sem_key, table_count, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    int idx = 0;
    for (int i = 0; i < table_count; i++)
        semctl(semid, idx++, SETVAL, table_array[i].max_osob);

    std::vector<pid_t> pids;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        execl("./pracownik", "pracownik", NULL);
        perror("exec");
        exit(1);
    }
    pids.push_back(pid);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        execl("./generator_klientow", "generator_klientow", NULL);
        perror("exec");
        exit(1);
    }
    pids.push_back(pid);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        execl("./kierownik", "kierownik", NULL);
        perror("exec");
        exit(1);
    }
    pids.push_back(pid);

    for (const auto chpid: pids)
        waitpid(chpid, nullptr, 0);

    if (shmdt(base) == -1) {
        perror("shmdt");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        perror("shmctl");
        exit(1);
    }
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }

    return 0;
}
