#include <iostream>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <vector>
#include <sys/wait.h>
#include <algorithm>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <cerrno>

#include "../include/wrappers.h"
#include "../include/Shared_memory.h"
#include "../include/Tables.h"

Table *table_array;

volatile sig_atomic_t fire_sig_flag = 0;
std::mutex pids_mutex;
std::atomic<bool> stop_thread{false};

int semid, msgid_logger;

std::vector<pid_t> pids;

void sigchld_handler(int) {}

void handler(int sig) {
	if (sig == SIGINT || sig == SIGRTMIN) {
		fire_sig_flag = 1;
		std::lock_guard<std::mutex> lock(pids_mutex);
		for (auto pid: pids) {
			kill(pid, SIGINT);
		}
	}
}

void watek() {
	wyslij_log(msgid_logger, "Watek do czyszczenia procesow zombie uruchomiony.", 4);

	while (!stop_thread) {
		int status = 0;

		pid_t pid = waitpid(-1, &status, 0);
		if (pid > 0) {
			std::lock_guard<std::mutex> lock(pids_mutex);

			auto it = std::find(pids.begin(), pids.end(), pid);
			if (it != pids.end()) {
				wyslij_log(msgid_logger, "Zabito proces [" + std::to_string(pid) + "]", 4);
				pids.erase(it);
				sem_op(semid, 1, 1);
			}
		}
		else if (pid == -1 && errno == ECHILD) {
			break;
		}
		else if (pid == -1 && errno != EINTR) {
			perror("waitpid error");
			break;
		}

		for (int i = 0; i < table_count_max; i++) {
			std::cout << "Stolik " << i << " ma " << sem_getval(semid, i) << " wolnych miejsc." << std::endl;
			std::cout << "-----------------------" << std::endl;
			std::cout << "Max osób: " << table_array[i].max_osob << std::endl;
			std::cout << "Typ siedzącj grupy: " << table_array[i].typ_grupy << std::endl;
			std::cout << "Czy zarezerwowany?: " << table_array[i].zarezerwowany_pzez_kierownika << std::endl;
		}
	}

	wyslij_log(msgid_logger, "Watek do czyszczenia procesow zombie zakonczony.", 4);
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
	//signal(SIGCHLD, SIG_IGN);


	//int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem)+table_count_max*sizeof(Table), 0666);
	//auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));
	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem) + sizeof(Table) * table_count_max, 0666);
	auto *base = static_cast<char *>(shm_attach(shmid, 0));

	auto *shared_mem_flags = reinterpret_cast<SharedMem *>(base);
	table_array = reinterpret_cast<Table *>(base + sizeof(SharedMem));


	semid = sem_create(ftok(".", 'G'), 4, 0666);
	sem_set(semid, 0, 1);
	sem_set(semid, 1, MAX_KLIENTOW);
	sem_set(semid, 2, 0);
	sem_set(semid, 3, MAX_KLIENTOW_W_RRESTAURACJI);

	msgid_logger = msg_create(ftok(".", 'L'), 0666);

	pid_t pid;
	std::thread tid(watek);

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
				std::lock_guard<std::mutex> lock(pids_mutex);
				pids.push_back(pid);

				wyslij_log(msgid_logger, "Utworzono klienta: [" + std::to_string(pid) + "]", 4);
			}
		}
	}
	for (int i = 0; i < pids.size(); ++i) {
		pid_t zakonczony;
		std::lock_guard<std::mutex> lock(pids_mutex);
		do {
			zakonczony = waitpid(-1, nullptr, 0);
		} while (errno == EINTR);
		auto it = std::find(pids.begin(), pids.end(), zakonczony);
		if (it != pids.end()) {
			pids.erase(it);
		}
	}
	{
		std::lock_guard<std::mutex> lock(pids_mutex);
		int r;
		for (auto pid: pids) {
			do {
				r = waitpid(-1, nullptr, 0);
			} while (r == -1 && errno == EINTR);
		}
	}
	stop_thread = true;
	if (tid.joinable())
		tid.join();

	sem_op(semid, 2, 2);
	shared_mem_flags->all_customers_out = true;
	wyslij_log(msgid_logger, "Klienci opuscili lokal, generator klientow konczy dzialanie", 4);
	shm_detach(shared_mem_flags);
	return 0;
}
