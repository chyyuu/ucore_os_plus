#include <stdio.h>
#include <ulib.h>
#include <error.h>

void test1(void)
{
	int pid, parent = getpid();
	if ((pid = fork()) == 0) {
		int event, sum = 0;
		while (recv_event(&pid, &event) == 0 && parent == pid) {
			if (event == -1) {
				cprintf("child1 Hmmm!\n");
				sleep(100);
				cprintf("child1 quit\n");
				exit(-1);
			}
			cprintf("child1 receive %08x from %d\n", event, pid);
			sum += event;
		}
		panic("FAIL: T.T\n");
	}
	assert(pid > 0);
	int i = 10;
	while (send_event(pid, i) == 0) {
		i--;
		sleep(50);
	}
	cprintf("test1 pass.\n");
}

void test2(void)
{
	int pid;
	if ((pid = fork()) == 0) {
		cprintf("child2 is spinning...\n");
		while (1) ;
	}
	assert(pid > 0);
	if (send_event_timeout(pid, 0xbee, 100) == -E_TIMEOUT) {
		cprintf("test2 pass.\n");
	}
	kill(pid);
}

void test3(void)
{
	int pid;
	if ((pid = fork()) == 0) {
		int event;
		if (recv_event_timeout(NULL, &event, 100) == -E_TIMEOUT) {
			cprintf("test3 pass.\n");
		}
		exit(0);
	}
	assert(pid > 0);
	assert(waitpid(pid, NULL) == 0);
}

int main(void)
{
	test1();
	test2();
	test3();
	cprintf("eventtest pass.\n");
	return 0;
}
