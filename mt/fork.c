#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

int main() {
	int sems_id;
	sems_id = semget(IPC_PRIVATE, 1, 0600 | IPC_CREAT);
	union semun sem_arg;
	sem_arg.val = 0;
	semctl(sems_id, 0, SETVAL, sem_arg);
	struct sembuf sembuf;

	int ret = fork();
	if (ret != 0) { // parent
		for (int i = 0; i < 10; ++i) {
			sembuf.sem_num = 0;
			sembuf.sem_op = -1;
			sembuf.sem_flg = 0;
			semop(sems_id, &sembuf, 1);
			printf("consumed: %d\n", i);
		}
	} else { // child
		for (int i = 0; i < 10; ++i) {
			sembuf.sem_num = 0;
			sembuf.sem_op = 1;
			sembuf.sem_flg = 0;
			semop(sems_id, &sembuf, 1);
			printf("produced: %d\n", i);
		}
	}
}
