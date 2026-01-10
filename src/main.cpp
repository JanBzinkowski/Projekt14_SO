#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <sys/ipc.h>
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

int main() {
    key_t shm_key = ftok(".", 'S');
    if (shm_key == -1) {
        ipc_die("ftok");
    }

    size_t table_size = sizeof(Table) * (table_count + X3 * 2);
    if (X3 == 0) {
        table_size += sizeof(Table);
    }
    size_t total_size = sizeof(SharedMem) + table_size;

    int shmid = shm_create(shm_key, total_size, IPC_CREAT | 0666);
    void *base = shm_attach(shmid, 0);

    auto *shared_mem_flags = (SharedMem *) base;
    auto *table_array = (Table *) ((char *) base + sizeof(SharedMem));

    shared_mem_flags->new_customers = true;
    shared_mem_flags->tables_array_size = table_size;

    key_t sem_key = ftok(".", 'M');
    if (sem_key == -1) {
        ipc_die("ftok sem");
    }

    int semid = sem_create(sem_key, table_count, IPC_CREAT | 0666);

    for (int i = 0; i < table_count; i++)
        sem_set(semid, i, table_array[i].max_osob);

    std::vector<pid_t> pids;

    pid_t pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        execl("./pracownik", "pracownik", NULL);
        ipc_die("exec pracownik");
    }
    pids.push_back(pid);

    pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        execl("./generator_klientow", "generator_klientow", NULL);
        ipc_die("exec generator_klientow");
    }
    pids.push_back(pid);

    pid = fork();
    if (pid < 0)
        ipc_die("fork");
    if (pid == 0) {
        execl("./kierownik", "kierownik", NULL);
        ipc_die("exec kierownik");
    }
    pids.push_back(pid);

    for (const auto chpid: pids)
        waitpid(chpid, nullptr, 0);

    shm_detach(base);
    shm_remove(shmid);
    sem_remove(semid);

    return 0;
}
