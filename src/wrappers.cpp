#include "../include/wrappers.h"
#include <cstring>
#include <iostream>

void ipc_die(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int shm_create(key_t key, size_t size, int flags) {
	if (key == -1)
		ipc_die("ftok shm_key");
	int shmid = shmget(key, size, flags);
	if (shmid == -1)
		ipc_die("shmget");
	return shmid;
}

void *shm_attach(int shmid, int flags) {
	void *addr = shmat(shmid, nullptr, flags);
	if (addr == (void *) -1)
		ipc_die("shmat");
	return addr;
}

void shm_detach(void *addr) {
	if (shmdt(addr) == -1)
		ipc_die("shmdt");
}

void shm_remove(int shmid) {
	if (shmctl(shmid, IPC_RMID, nullptr) == -1)
		ipc_die("shmctl(IPC_RMID)");
}

int sem_create(key_t key, int nsems, int flags) {
	if (key == -1)
		ipc_die("ftok shm_key");
	int semid = semget(key, nsems, flags);
	if (semid == -1)
		ipc_die("semget");
	return semid;
}

void sem_set(int semid, int semnum, int val) {
	union semun arg{};
	arg.val = val;
	if (semctl(semid, semnum, SETVAL, arg) == -1)
		ipc_die("semctl SETVAL");
}

int sem_op(int semid, int semnum, int op, volatile sig_atomic_t *flag, short int sem_flag) {
	sembuf sb{
		static_cast<unsigned short>(semnum),
		static_cast<short>(op),
		sem_flag
	};

	while (true) {
		if (flag && *flag) {
			return -1;
		}

		if (semop(semid, &sb, 1) == 0) {
			return 0;
		}

		if (errno == EINTR) {
			continue;
		}

		if (sem_flag & IPC_NOWAIT) {
			return -1;
		}

		ipc_die("semop");
	}
}


void sem_remove(int semid) {
	if (semctl(semid, 0, IPC_RMID) == -1)
		ipc_die("semctl IPC_RMID");
}

int sem_getval(int semid, int semnum) {
	int val = semctl(semid, semnum, GETVAL, 0);
	if (val == -1)
		ipc_die("semctl GETVAL");
	return val;
}

int msg_create(key_t key, int flags) {
	if (key == -1)
		ipc_die("ftok shm_key");
	int msgid = msgget(key, flags);
	if (msgid == -1)
		ipc_die("msgget");
	return msgid;
}

void msg_send(int msgid, void *msg, size_t size, int flags) {
	int ret;
	do {
		ret = msgsnd(msgid, msg, size, flags);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1)
		ipc_die("msgsnd");
}

ssize_t msg_recv(int msgid, void *msg, size_t size, long type, int flags, volatile sig_atomic_t *sig_flag) {
	while (true) {
		if (sig_flag && *sig_flag) {
			return -1;
		}

		ssize_t ret = msgrcv(msgid, msg, size, type, flags);
		if (ret >= 0) {
			return ret;
		}

		if (errno == EINTR) {
			continue;
		}

		if (flags & IPC_NOWAIT) {
			return -1;
		}

		ipc_die("msgrcv");
	}
}

void msg_remove(int msgid) {
	if (msgctl(msgid, IPC_RMID, nullptr) == -1)
		ipc_die("msgctl IPC_RMID");
}

void wyslij_log(int logger_id, const std::string &tekst, long msgtype) {
	if (tekst.empty() && msgtype == 1)
		return;

	MsgText msg{};
	msg.mtype = msgtype;
	strncpy(msg.text, tekst.c_str(), sizeof(msg.text) - 1);
	msg.text[sizeof(msg.text) - 1] = '\0';

	msg_send(logger_id, &msg, sizeof(msg.text), 0);
}

ssize_t pipe_recv(int fd, void *buf, size_t count, volatile sig_atomic_t *sig_flag) {
	while (true) {
		if (sig_flag && *sig_flag) {
			return -1;
		}

		ssize_t ret = read(fd, buf, count);
		if (ret >= 0) {
			return ret;
		}

		if (errno == EINTR) {
			continue;
		}

		ipc_die("read");
	}
}
