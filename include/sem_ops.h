#ifndef SO_SEM_OPS_H
#define SO_SEM_OPS_H

void sem_wait(int semid, unsigned short int idx);
void sem_post(int semid, unsigned short int idx);

#endif //SO_SEM_OPS_H