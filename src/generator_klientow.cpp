#include <iostream>
#include <unistd.h>
#include <csignal>
#include <pthread.h>
#include <thread>
#include <vector>
#include <sys/wait.h>
#include <algorithm>
#include <fcntl.h>

#include "../../../../../usr/include/c++/11/filesystem"
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/zamowienie.h"

volatile sig_atomic_t fire_sig_flag = 0;
pthread_mutex_t pids_mutex = PTHREAD_MUTEX_INITIALIZER;

int semid, msgid_logger;

std::vector<pid_t> pids;

struct ThreadArgs {
	SharedMem *flags;
};

void sigchld_handler(int) {}

void handler(int sig) {
	if (sig == SIGINT || sig == SIGRTMIN) {
		fire_sig_flag = 1;
		pthread_mutex_lock(&pids_mutex);
		for (auto pid: pids) {
			kill(pid, SIGINT);
		}
		pthread_mutex_unlock(&pids_mutex);
	}
}

void *watek(void *arg) {
	//auto args = static_cast<ThreadArgs *>(arg);
	int status;
	while (true) {
		pid_t zakonczony = waitpid(-1, &status, 0);
		if (zakonczony > 0) {
			pthread_mutex_lock(&pids_mutex);
			for (size_t i = 0; i < pids.size(); ++i) {
				if (pids[i] == zakonczony) {
					pids[i] = pids.back();
					pids.pop_back();
					break;
				}
			}
			pthread_mutex_unlock(&pids_mutex);
			continue;
		}

		if (zakonczony == -1) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}
	}
	return nullptr;
}

int main() {
	struct sigaction sa{};
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	struct sigaction sa_child{};
	sa_child.sa_handler = sigchld_handler;
	sigemptyset(&sa_child.sa_mask);
	sa_child.sa_flags = 0;

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGRTMIN, &sa, nullptr);
	sigaction(SIGCHLD, &sa_child, nullptr);


	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));

	semid = sem_create(ftok(".", 'G'), 4, 0666);
	sem_set(semid, 0, 1);
	sem_set(semid, 1, MAX_KLIENTOW);
	sem_set(semid, 2, 0);
	sem_set(semid, 3, MAX_KLIENTOW_W_RRESTAURACJI);

	msgid_logger = msg_create(ftok(".", 'L'), 0666);

	pid_t pid;
	pthread_t tid;
	ThreadArgs thread_args{shared_mem_flags};
	if (pthread_create(&tid, nullptr, watek, &thread_args) != 0) {
		ipc_die("pthread_create");
	}

	while (!shared_mem_flags->end_program && fire_sig_flag == 0) {
		if (sem_op(semid, 0, -1, &fire_sig_flag) == -1) {
			break;
		}
		while (shared_mem_flags->new_customers && fire_sig_flag == 0) {
			//sleep(rand() % 10 + 1);
			if (sem_op(semid, 1, -1, &fire_sig_flag) == -1) {
				break;
			}
			pid = fork();
			if (pid == -1) {
				perror("fork, generator klientow");
			}
			else if (pid == 0) {
				execl("./klient", "klient", NULL);
				perror("execl");
				_exit(1);
			}
			else {
				pthread_mutex_lock(&pids_mutex);
				pids.push_back(pid);
				pthread_mutex_unlock(&pids_mutex);
				wyslij_log(msgid_logger, "Utworzono klienta: [" + std::to_string(pid) + "]", 4);
			}
		}
	}
	for (int i = 0; i < pids.size(); ++i) {
		pid_t zakonczony;
		pthread_mutex_lock(&pids_mutex);
		do {
			zakonczony = waitpid(-1, nullptr, 0);
		} while (errno == EINTR);
		auto it = std::find(pids.begin(), pids.end(), zakonczony);
		if (it != pids.end()) {
			pids.erase(it);
		}
		pthread_mutex_unlock(&pids_mutex);
	}
	pthread_join(tid, nullptr);
	sem_op(semid, 2, 2);
	shared_mem_flags->all_customers_out = true;
	wyslij_log(msgid_logger, "Klienci opuscili lokal, generator klientow konczy dzialanie", 4);

	shm_detach(shared_mem_flags);
	return 0;
}
