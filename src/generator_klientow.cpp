#include <iostream>
#include <unistd.h>
#include <csignal>

#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

int main() {
	srand(time(nullptr));
	key_t shm_key = ftok(".", 'S');
	if (shm_key == -1) {
		ipc_die("ftok");
	}

	int shmid = shm_create(shm_key, sizeof(SharedMem), 0);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));

	signal(SIGCHLD, SIG_IGN);
	int pid;

	while (!shared_mem_flags->end_program) {
		pid = fork();
		if (pid == -1) {
			perror("fork, generator klientow");
		}
		else if (pid == 0) {
			execl("./klient", "klient", NULL);
			perror("execl");
			exit(1);
		}
		else {
			sleep(rand() % 31);
		}
	}

	shm_detach(shared_mem_flags);
}
