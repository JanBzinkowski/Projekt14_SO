#include "../include/sem_ops.h"
#include <sys/sem.h>
#include <cstdlib>
#include <atomic>

void sem_wait(int semid, unsigned short int idx) {
	struct sembuf op = {idx, -1, 0};
	if (semop(semid, &op, 1) == -1) {
		perror("semop wait");
		exit(1);
	}
}

void sem_post(int semid, unsigned short int idx) {
	struct sembuf op = {idx, 1, 0};
	if (semop(semid, &op, 1) == -1) {
		perror("semop post");
		exit(1);
	}
}
