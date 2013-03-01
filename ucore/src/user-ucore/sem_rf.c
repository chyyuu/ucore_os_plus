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

sem_t sem_count, sem_write, sem_console;
int *pCount;

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
	if (check_sem_value_sub(sem_count, 1) != 0
	    || check_sem_value_sub(sem_write, 1) != 0) {
		failed();
	}
	if (*pCount != 0) {
		failed();
	}
}

void init(void)
{
	if ((sem_console = sem_init(1)) < 0) {
		failed();
	}
	if ((sem_count = sem_init(1)) < 0 || (sem_write = sem_init(1)) < 0) {
		failed();
	}
	if ((pCount = shmem_malloc(sizeof(int))) == NULL) {
		failed();
	}
	*pCount = 0;
}

void reader(int id, int time)
{
	scprintf("reader %d: (pid:%d) arrive\n", id, getpid());
	sem_wait(sem_count);
	if (*pCount == 0) {
		sem_wait(sem_write);
	}
	(*pCount)++;
	sem_post(sem_count);

	scprintf("    reader_rf %d: (pid:%d) start %d\n", id, getpid(), time);
	sleep(time);
	scprintf("    reader_rf %d: (pid:%d) end %d\n", id, getpid(), time);

	sem_wait(sem_count);
	(*pCount)--;
	if (*pCount == 0) {
		sem_post(sem_write);
	}
	sem_post(sem_count);
}

void writer(int id, int time)
{
	scprintf("writer %d: (pid:%d) arrive\n", id, getpid());
	sem_wait(sem_write);

	scprintf("    writer_rf %d: (pid:%d) start %d\n", id, getpid(), time);
	sleep(time);
	scprintf("    writer_rf %d: (pid:%d) end %d\n", id, getpid(), time);

	sem_post(sem_write);
}

void read_test_rf(void)
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
	cprintf("read_test_rf ok.\n");
}

void write_test_rf(void)
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
	cprintf("write_test_rf ok.\n");
}

void read_write_test_rf(void)
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
	cprintf("read_write_test_rf ok.\n");
}

int main(void)
{
	init();
	read_test_rf();
	write_test_rf();
	read_write_test_rf();

	cprintf("sem_rf pass..\n");
	return 0;
}
