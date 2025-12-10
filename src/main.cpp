#include <iostream>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

int main() {
	int X1, X2, X3, X4;
	X1 = 3;
	X2 = 4;
	X3 = 2;
	X4 = 2;

	const int shared = shm_open("/shmem", O_CREAT | O_RDWR, 0666);
	if (shared == -1) {
		perror("shared memory open");
		exit(1);
	}
	const size_t table_size = sizeof(Table) * (X1 + X2 + X3 + X4);
	const size_t total_mem_size = table_size + sizeof(SharedMem);
	ftruncate(shared, total_mem_size);
	auto *shared_mem_flags = static_cast<SharedMem *>(mmap(nullptr, sizeof(SharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shared, 0));
	if (shared_mem_flags == MAP_FAILED) {
		perror("mmap shared_mem");
		exit(1);
	}

	shared_mem_flags->new_customers = true;

	auto *table_array = static_cast<Table *>(mmap(nullptr, table_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared, sizeof(SharedMem)));
	if (table_array == MAP_FAILED) {
		perror("mmap tables");
		exit(1);
	}

	int idx = 0;
	for (int i = 0; i < X1; i++) {
		sem_init(&table_array[idx].wolne_miejsca, 1, 1);
		table_array[idx].rozmiar_grupy = 0;
		idx++;
	}
	for (int i = 0; i < X2; i++) {
		sem_init(&table_array[idx].wolne_miejsca, 1, 2);
		table_array[idx].rozmiar_grupy = 0;
		idx++;
	}
	for (int i = 0; i < X3; i++) {
		sem_init(&table_array[idx].wolne_miejsca, 1, 3);
		table_array[idx].rozmiar_grupy = 0;
		idx++;
	}
	for (int i = 0; i < X4; i++) {
		sem_init(&table_array[idx].wolne_miejsca, 1, 4);
		table_array[idx].rozmiar_grupy = 0;
		idx++;
	}

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

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		execl("./szef", "szef", NULL);
		perror("exec");
		exit(1);
	}

	pids.push_back(pid);

	for (const auto chpid: pids) {
		waitpid(chpid, nullptr, 0);
	}

	for (int i = 0; i < X1 + X2 + X3 + X4; i++) {
		sem_destroy(&table_array[i].wolne_miejsca);
	}
	munmap(shared_mem_flags, sizeof(SharedMem));
	munmap(table_array, sizeof(Table) * (X1 + X2 + X3 + X4));
	close(shared);
	shm_unlink("/shmem");
}
