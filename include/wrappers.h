#ifndef SO_SEM_OPS_H
#define SO_SEM_OPS_H

#include <cstdlib>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string>


struct semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

struct MyMsg {
	long mtype;
	int data;
};

struct MsgText {
	long mtype;
	char text[256];
};

void ipc_die(const char *msg);

void sem_set(int semid, int semnum, int val);

int sem_create(key_t key, int nsems, int flags);

void sem_op(int semid, int semnum, int op);

void sem_remove(int semid);

int sem_getval(int semid, int semnum);

int shm_create(key_t key, size_t size, int flags);

void *shm_attach(int shmid, int flags);

void shm_detach(void *addr);

void shm_remove(int shmid);

int msg_create(key_t key, int flags);

void msg_send(int msgid, void *msg, size_t size, int flags);

ssize_t msg_recv(int msgid, void *msg, size_t size, long type, int flags);

void msg_remove(int msgid);

void wyslij_log(int logger_id, const std::string &tekst);

#endif //SO_SEM_OPS_H
