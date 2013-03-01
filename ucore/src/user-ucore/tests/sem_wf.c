#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#define scprintf(...)               \
    do {                            \
        sem_wait(sem_console);      \
        cprintf(__VA_ARGS__);       \
        sem_post(sem_console);      \
    } while (0)

sem_t sem_read, sem_write, sem_console;
sem_t sem_x, sem_y, sem_z;
int *pCountR, *pCountW;

void failed(void)
{
	cprintf("FAIL: T.T\n");
	exit(-1);
}

int check_sem_value_sub(sem_t sem_id, int value)
{
	int value_store;
	if (sem_get_value(sem_id, &value_store) != 0 || value_store != value) {
		return -1;
	}
	return 0;
}

void check_sem_value(void)
{
	if (check_sem_value_sub(sem_read, 1) != 0
	    || check_sem_value_sub(sem_write, 1) != 0) {
		failed();
	}
	if (check_sem_value_sub(sem_x, 1) != 0
	    || check_sem_value_sub(sem_y, 1) != 0
	    || check_sem_value_sub(sem_z, 1) != 0) {
		failed();
	}
	if (*pCountR != 0 || *pCountW != 0) {
		failed();
	}
}

void init(void)
{
	if ((sem_console = sem_init(1)) < 0) {
		failed();
	}
	if ((sem_read = sem_init(1)) < 0 || (sem_write = sem_init(1)) < 0) {
		failed();
	}
	if ((sem_x = sem_init(1)) < 0 || (sem_y = sem_init(1)) < 0
	    || (sem_z = sem_init(1)) < 0) {
		failed();
	}
	if ((pCountR = shmem_malloc(sizeof(int))) == NULL
	    || (pCountW = shmem_malloc(sizeof(int))) == NULL) {
		failed();
	}
	*pCountR = *pCountW = 0;
}

void reader(int id, int time)
{
	scprintf("reader %d: (pid:%d) arrive\n", id, getpid());
	sem_wait(sem_z);
	sem_wait(sem_read);
	sem_wait(sem_x);
	(*pCountR)++;
	if (*pCountR == 1) {
		sem_wait(sem_write);
	}
	sem_post(sem_x);
	sem_post(sem_read);
	sem_post(sem_z);

	scprintf("    reader_rf %d: (pid:%d) start %d\n", id, getpid(), time);
	sleep(time);
	scprintf("    reader_rf %d: (pid:%d) end %d\n", id, getpid(), time);

	sem_wait(sem_x);
	(*pCountR)--;
	if (*pCountR == 0) {
		sem_post(sem_write);
	}
	sem_post(sem_x);
}

void writer(int id, int time)
{
	scprintf("writer %d: (pid:%d) arrive\n", id, getpid());
	sem_wait(sem_y);
	(*pCountW)++;
	if (*pCountW == 1) {
		sem_wait(sem_read);
	}
	sem_post(sem_y);
	sem_wait(sem_write);

	scprintf("    writer_rf %d: (pid:%d) start %d\n", id, getpid(), time);
	sleep(time);
	scprintf("    writer_rf %d: (pid:%d) end %d\n", id, getpid(), time);

	sem_post(sem_write);
	sem_wait(sem_y);
	(*pCountW)--;
	if (*pCountW == 0) {
		sem_post(sem_read);
	}
	sem_post(sem_y);
}

void read_test_wf(void)
{
	cprintf("---------------------------------\n");
	check_sem_value();
	srand(0);
	int i, total = 10, time;
	for (i = 0; i < total; i++) {
		time = (unsigned int)rand() % 3;
		if (fork() == 0) {
			yield();
			reader(i, 100 + time * 10);
			exit(0);
		}
	}

	for (i = 0; i < total; i++) {
		if (wait() != 0) {
			failed();
		}
	}
	cprintf("read_test_wf ok.\n");
}

void write_test_wf(void)
{
	cprintf("---------------------------------\n");
	check_sem_value();
	srand(100);
	int i, total = 10, time;
	for (i = 0; i < total; i++) {
		time = (unsigned int)rand() % 3;
		if (fork() == 0) {
			yield();
			writer(i, 100 + time * 10);
			exit(0);
		}
	}

	for (i = 0; i < total; i++) {
		if (wait() != 0) {
			failed();
		}
	}
	cprintf("write_test_wf ok.\n");
}

void read_write_test_wf(void)
{
	cprintf("---------------------------------\n");
	check_sem_value();
	srand(200);
	int i, total = 10, time;
	for (i = 0; i < total; i++) {
		time = (unsigned int)rand() % 3;
		if (fork() == 0) {
			yield();
			if (time == 0) {
				writer(i, 100 + time * 10);
			} else {
				reader(i, 100 + time * 10);
			}
			exit(0);
		}
	}

	for (i = 0; i < total; i++) {
		if (wait() != 0) {
			failed();
		}
	}
	cprintf("read_write_test_wf ok.\n");
}

int main(void)
{
	init();
	read_test_wf();
	write_test_wf();
	read_write_test_wf();

	cprintf("sem_wf pass..\n");
	return 0;
}
