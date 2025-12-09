#include <iostream>
#include <cstdlib.h>
#include <unistd.h>

int main() {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		execl("./pracownik", "pracownik", NULL);
		perror("exec");
		exit(1);
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		execl("./generator_klientow", "generator_klientow", NULL);
		perror("exec");
		exit(1);
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		execl("./kierownik", "kierownik", NULL);
		perror("exec");
		exit(1);
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		execl("./szef", "szef", NULL);
		perror("exec");
		exit(1);
	}
}
