#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <chrono>

#include "../include/Wrappers.h"
#include "../include/Shared_memory.h"

int msgid;

void handler(int sig) {
	//puusty handler aby unikąć przerywania w działaniu programu
}

std::string timestamp() {
	using namespace std::chrono;
	const auto now = system_clock::now();
	const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	std::time_t t = system_clock::to_time_t(now);
	std::tm tm_now{};
	localtime_r(&t, &tm_now);

	std::ostringstream oss;
	oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")
			<< '.' << std::setfill('0') << std::setw(3) << ms.count();
	return oss.str();
}

int main() {
	struct sigaction sa{};
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGRTMIN, &sa, nullptr);

	int shmid = shm_create(ftok(".", 'S'), sizeof(SharedMem), 0666);
	auto *shared_mem_flags = static_cast<SharedMem *>(shm_attach(shmid, 0));


	msgid = msg_create(ftok(".", 'L'), 0666);
	int logger_semid = sem_create(ftok(".", 'P'), 1, 0666);

	std::ofstream log_file("../log.txt", std::ios::trunc);
	if (!log_file.is_open()) {
		std::cerr << "Nie można otworzyć pliku ../log.txt" << std::endl;
		return 1;
	}

	std::cout << "Logger uruchomiony. Odbieranie wiadomości..." << std::endl;
	log_file << "Logger uruchomiony. Odbieranie wiadomości..." << std::endl;

	while (sem_getval(logger_semid, 0) > 0) {
		MsgText msg{};

		if (msg_recv(msgid, &msg, sizeof(msg.text), -1000, 0) > 0) {
			if (msg.mtype == 999) {
				break;
			}
			const auto line = "[" + timestamp() + "][mtype=" + std::to_string(msg.mtype) + "] " + msg.text;
			std::cout << line << std::endl;
			log_file << line << std::endl;
			log_file.flush();
		}
	}

	std::cout << "Logger zakończony." << std::endl;
	log_file << "Logger zakończony." << std::endl;
	log_file.close();

	shm_detach(shared_mem_flags);

	return 0;
}
