#include <stdio.h>
#include <ulib.h>
#include <malloc.h>

struct slot {
	char data[4096];
};

static struct slot **slots;

int main(void)
{
	struct slot *tmp;
	int n = 0, rounds = 10;
	slots = malloc(sizeof(struct slot *) * 1000 * rounds);
	cprintf("I am going to eat out all the mem, MU HA HA!!.\n");
	while (rounds > 0
	       && (tmp = (struct slot *)malloc(sizeof(struct slot))) != NULL) {
		slots[n] = tmp;
		tmp->data[0] = (char)n;
		if ((++n) % 1000 == 0) {
			cprintf("I ate %d slots.\n", n);
			rounds--;
		}
	}
	cprintf("I ate (at least) %d byte memory.\n", n * sizeof(struct slot));
	print_pgdir();

	int error = 0;
	while (n > 0) {
		tmp = slots[--n];
		if (tmp->data[0] != (char)(n)) {
			error++;
		}
		free(tmp);
	}
	free(slots);
	cprintf("I free all the memory.(%d)\n", error);
	cprintf("brktest pass.\n");
	return 0;
}
