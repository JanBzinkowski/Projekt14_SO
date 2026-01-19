#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include "../include/wrappers.h"
#include "../include/Shared_memory.h"

int main() {
	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));


	int msgid = msg_create(ftok(".", 'L'), 0666);

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

	msg_remove(msgid);

	return 0;
}
