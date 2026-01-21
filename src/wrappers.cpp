#include "../include/wrappers.h"
#include <cstring>

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

void sem_op(int semid, int semnum, int op, volatile sig_atomic_t *flag) {
	struct sembuf sb{};
	sb.sem_num = static_cast<unsigned short>(semnum);
	sb.sem_op = static_cast<short>(op);
	sb.sem_flg = 0;

	int ret;
	do {
		if (flag && *flag) {
			return;
		}
		ret = semop(semid, &sb, 1);
		if (ret == -1 && errno == EINTR) {
			return;
		}
	} while (ret == -1 && errno == EINTR);

	if (ret == -1)
		ipc_die("semop");
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
	ssize_t ret;
	do {
		if (sig_flag && *sig_flag) {
			return -1;
		}
		ret = msgrcv(msgid, msg, size, type, flags);
		if (ret == -1 && errno == EINTR) {
			return -1;
		}
	} while (ret == -1 && errno == EINTR);

	if (ret == -1)
		ipc_die("msgrcv");

	return ret;
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
	ssize_t ret;
	do {
		ret = read(fd, buf, count);
		if (ret == -1 && errno == EINTR && sig_flag && *sig_flag) {
			return -1;
		}
	} while (ret == -1 && errno == EINTR);

	if (ret == -1) {
		ipc_die("read");
	}
	return ret;
}
