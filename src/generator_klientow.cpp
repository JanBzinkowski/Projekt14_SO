#include <iostream>
#include <unistd.h>
#include <csignal>

#include "../../../../../usr/include/c++/11/thread"
#include "../../../../../usr/include/c++/11/vector"
#include "../../../../../usr/include/x86_64-linux-gnu/sys/wait.h"
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

volatile sig_atomic_t fire_sig_flag = 0;
volatile sig_atomic_t child_flag = 0;

int semid;

void sigchld_handler(int) {
	child_flag = 1;
	sem_op(semid, 2, 1);
}

void child_remove(std::vector<int> &pids) {
	while (true) {
		pid_t pid = waitpid(-1, nullptr, WNOHANG);
		if (pid <= 0) {
			child_flag = 0;
			break;
		}
		pids.erase(std::remove(pids.begin(), pids.end(), pid), pids.end());
	}
}

int main() {
	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));

	signal(SIGCHLD, sigchld_handler);

	semid = sem_create(ftok(".", 'G'), 3, 0666 | IPC_CREAT);
	sem_set(semid, 0, 1);

	std::vector<int> pids;
	int pid;
	while (!shared_mem_flags->end_program) {
		sem_op(semid, 0, -1);
		while (shared_mem_flags->new_customers) {
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
				pids.push_back(pid);
				if (child_flag) {
					sem_op(semid, 2, -1);
					child_remove(pids);
				}
				sleep(rand() % 30 + 1);
			}
		}
		sem_op(semid, 2, -1);
		child_remove(pids);
	}
	while (!pids.empty()) {
		sem_op(semid, 2, -1);
		child_remove(pids);
	}
	sem_op(semid, 1, 2);
	shared_mem_flags->all_customers_out = true;

	shm_detach(shared_mem_flags);
	sem_remove(semid);
	return 0;
}
