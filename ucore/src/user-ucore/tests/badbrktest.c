#include <stdio.h>
#include <ulib.h>
#include <malloc.h>

struct slot {
	char data[4096];
	struct slot *next;
};

int main(void)
{
	int pid, exit_code;
	if ((pid = fork()) == 0) {
		struct slot *tmp, *head = NULL;
		int n = 0, m = 0;
		cprintf("I am child.\n");
		cprintf("I am going to eat out all the mem, MU HA HA!!.\n");
		while ((tmp =
			(struct slot *)malloc(sizeof(struct slot))) != NULL) {
			n++, m++;
			if (m == 1000) {
				cprintf("I ate %d slots.\n", m);
				m = 0;
			}
			tmp->next = head;
			head = tmp;
		}
		exit(0xdead);
	}
	assert(pid > 0);
	assert(waitpid(pid, &exit_code) == 0 && exit_code != 0xdead);
	cprintf("child is killed by kernel, en.\n");

	assert(wait() != 0);
	cprintf("badbrktest pass.\n");
	return 0;
}
