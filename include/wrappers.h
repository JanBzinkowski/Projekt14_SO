#ifndef SO_SEM_OPS_H
#define SO_SEM_OPS_H

#include <cstdlib>
#include <sys/types.h>
#include <csignal>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string>

#define EXTRA 1
#define REZERWACJE 2


struct semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

struct MsgText {
	long mtype;
	char text[256];
};

struct Reserve {
	int x1;
	int x2;
	int x3;
	int x4;
};

struct KierownikRezerwacja {
	long mtype;
	Reserve reserved;
};

struct KierownikStoly {
	long mtype;
	bool extra = true;
};

void ipc_die(const char *msg);

void sem_set(int semid, int semnum, int val);

int sem_create(key_t key, int nsems, int flags);

void sem_op(int semid, int semnum, int op, volatile sig_atomic_t *flag = nullptr);

void sem_remove(int semid);

int sem_getval(int semid, int semnum);

int shm_create(key_t key, size_t size, int flags);

void *shm_attach(int shmid, int flags);

void shm_detach(void *addr);

void shm_remove(int shmid);

int msg_create(key_t key, int flags);

void msg_send(int msgid, void *msg, size_t size, int flags);

ssize_t msg_recv(int msgid, void *msg, size_t size, long type, int flags, volatile sig_atomic_t *sig_flag = nullptr);

void msg_remove(int msgid);

void wyslij_log(int logger_id, const std::string &tekst);

#endif //SO_SEM_OPS_H
