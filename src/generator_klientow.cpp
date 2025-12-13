#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctime>
#include <csignal>

#include "../include/Shared_memory.h"

int main() {
	srand(time(nullptr));
	int shared = shm_open("/shmem", O_RDWR, 0666);
	if (shared == -1) {
		perror("shared memory open");
		exit(1);
	}

	auto *shared_mem_flags = static_cast<SharedMem *>(mmap(nullptr, sizeof(SharedMem), PROT_READ, MAP_SHARED, shared, 0));

	signal(SIGCHLD, SIG_IGN);
	int pid;

	while (shared_mem_flags->end_program == false) {
		pid = fork();
		if (pid == -1) {
			perror("fork, generator klientow");
		}
		else if (pid == 0) {
			execl("./klient", "klient", NULL);
			perror("execl failed");
			exit(1);
		}
		else {
			sleep(rand() % 100);
		}
	}
}
