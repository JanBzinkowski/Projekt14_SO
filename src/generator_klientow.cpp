#include <iostream>
#include <unistd.h>
#include <csignal>
#include <pthread.h>
#include <thread>
#include <vector>
#include <sys/wait.h>
#include <algorithm>
#include <fcntl.h>

#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/zamowienie.h"

volatile sig_atomic_t fire_sig_flag = 0;
pthread_mutex_t pids_mutex = PTHREAD_MUTEX_INITIALIZER;

int semid, msgid_logger;

std::vector<pid_t> pids;

int sig_pipe[2];

struct ThreadArgs {
	SharedMem *flags;
};

void sigchld_handler(int) {
	uint8_t b = 1;
	ssize_t r = write(sig_pipe[1], &b, 1);
	(void) r;
}

void handler(int sig) {
	if (sig == SIGRTMIN) {
		fire_sig_flag = 1;
		uint8_t b = 1;
		ssize_t r = write(sig_pipe[1], &b, 1);
		(void) r;
	}
	if (sig == SIGINT) {
		pthread_mutex_lock(&pids_mutex);
		for (auto pid: pids) {
			kill(pid, SIGINT);
		}
		pthread_mutex_unlock(&pids_mutex);
	}
}

void child_remove() {
	while (true) {
		pid_t pid = waitpid(-1, nullptr, WNOHANG);
		if (pid <= 0) {
			break;
		}
		pids.erase(std::remove(pids.begin(), pids.end(), pid), pids.end());
		sem_op(semid, 2, 1);
		wyslij_log(msgid_logger, "Usunięto klienta");
	}
}

void *watek(void *arg) {
	auto args = (ThreadArgs *) arg;
	uint8_t b;

	while (true) {
		pthread_mutex_lock(&pids_mutex);
		bool stop = args->flags->end_program && pids.empty();
		pthread_mutex_unlock(&pids_mutex);

		if (stop) {
			break;
		}

		ssize_t r = pipe_recv(sig_pipe[0], &b, 1, &fire_sig_flag);

		if ((r == 0 || r == -1)) {
			break;
		}

		pthread_mutex_lock(&pids_mutex);
		child_remove();
		pthread_mutex_unlock(&pids_mutex);
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

	if (pipe(sig_pipe) == -1) {
		ipc_die("pipe");
	}
	fcntl(sig_pipe[1], F_SETFL, O_NONBLOCK);

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGCHLD, &sa_child, nullptr);

	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));

	semid = sem_create(ftok(".", 'G'), 3, 0666);
	sem_op(semid, 0, 1);
	sem_op(semid, 1, MAX_KLIENTOW);

	msgid_logger = msg_create(ftok(".", 'L'), 0666);

	pid_t pid;

	ThreadArgs thread_args{shared_mem_flags};

	pthread_t tid;
	if (pthread_create(&tid, nullptr, watek, &thread_args) != 0) {
		ipc_die("pthread_create");
	}

	while (!shared_mem_flags->end_program && fire_sig_flag == 0) {
		if (sem_op(semid, 0, -1, &fire_sig_flag) == -1) {
			break;
		}
		while (shared_mem_flags->new_customers && fire_sig_flag == 0) {
			sleep(rand() % 10 + 1);
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
				exit(1);
			}
			else {
				pthread_mutex_lock(&pids_mutex);
				pids.push_back(pid);
				pthread_mutex_unlock(&pids_mutex);
			}
		}
	}
	uint8_t b = 1;
	write(sig_pipe[1], &b, 1);
	pthread_join(tid, nullptr);
	sem_op(semid, 2, 2);
	shared_mem_flags->all_customers_out = true;
	wyslij_log(msgid_logger, "Klienci opuscili lokal, generator klientow konczy dzialanie", 4);

	close(sig_pipe[0]);
	close(sig_pipe[1]);

	shm_detach(shared_mem_flags);
	sem_remove(semid);
	return 0;
}
