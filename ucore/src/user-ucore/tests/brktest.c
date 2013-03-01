#include <stdio.h>
#include <ulib.h>
#include <malloc.h>

struct slot {
	char data[4096];
	struct slot *next;
};

int main(void)
{
	struct slot *tmp, *head = NULL;
	int n = 0, rounds = 10;
	cprintf("I am going to eat out all the mem, MU HA HA!!.\n");
	while (rounds > 0
	       && (tmp = (struct slot *)malloc(sizeof(struct slot))) != NULL) {
		if ((++n) % 1000 == 0) {
			cprintf("I ate %d slots.\n", n);
			rounds--;
		}
		tmp->next = head;
		head = tmp;
		head->data[0] = (char)n;
	}
	cprintf("I ate (at least) %d byte memory.\n", n * sizeof(struct slot));
	print_pgdir();

	int error = 0;
	while (head != NULL) {
		if (head->data[0] != (char)(n--)) {
			error++;
		}
		tmp = head->next;
		free(head);
		head = tmp;
	}
	cprintf("I free all the memory.(%d)\n", error);
	cprintf("brktest pass.\n");
	return 0;
}
