#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

int main() {
	key_t shm_key = ftok(".", 'S');
	if (shm_key == -1) {
		perror("ftok");
		return 1;
	}

	int shmid = shm_create(shm_key, sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));

	key_t msg_key = ftok(".", 'L');
	if (msg_key == -1) {
		perror("ftok msg");
		return 1;
	}

	int msgid = msg_create(msg_key, 0666 | IPC_CREAT);

	std::ofstream log_file("../log.txt", std::ios::app);
	if (!log_file.is_open()) {
		std::cerr << "Nie można otworzyć pliku ../log.txt" << std::endl;
		return 1;
	}

	std::cout << "Logger uruchomiony. Odbieranie wiadomości..." << std::endl;

	while (!shared_mem_flags->end_program) {
		MsgText msg{};
		if (msg_recv(msgid, &msg, sizeof(msg.text), 0, 0) > 0) {
			std::cout << "[mtype=" << msg.mtype << "] " << msg.text << std::endl;
			log_file << "[mtype=" << msg.mtype << "] " << msg.text << std::endl;
			log_file.flush();
		}
	}

	std::cout << "Logger zakończony." << std::endl;
	log_file.close();
	shm_detach(shared_mem_flags);

	return 0;
}
