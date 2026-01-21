#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

volatile sig_atomic_t fire_flag = 0;
int msgid;

void handler(int sig) {
	if (sig == SIGRTMIN || sig == SIGINT) {
		fire_flag = 1;
	}
}

int main() {
	struct sigaction sa{};
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));


	msgid = msg_create(ftok(".", 'L'), 0666);
	int logger_semid = sem_create(ftok(".", 'P'), 1, 0666);

	std::ofstream log_file("../log.txt", std::ios::app);
	if (!log_file.is_open()) {
		std::cerr << "Nie można otworzyć pliku ../log.txt" << std::endl;
		return 1;
	}

	std::cout << "Logger uruchomiony. Odbieranie wiadomości..." << std::endl;
	log_file << "Logger uruchomiony. Odbieranie wiadomości..." << std::endl;

	while (!shared_mem_flags->end_program && fire_flag == 0 && sem_getval(logger_semid, 0) > 0) {
		MsgText msg{};
		if (msg_recv(msgid, &msg, sizeof(msg.text), -1, 0, &fire_flag) > 0) {
			if (msg.mtype == 999) {
				break;
			}
			std::cout << "[mtype=" << msg.mtype << "] " << msg.text << std::endl;
			log_file << "[mtype=" << msg.mtype << "] " << msg.text << std::endl;
			log_file.flush();
		}
	}

	std::cout << "Logger zakończony." << std::endl;
	log_file << "Logger zakończony." << std::endl;
	log_file.close();

	shm_detach(shared_mem_flags);

	return 0;
}
