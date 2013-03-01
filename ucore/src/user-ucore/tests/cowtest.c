#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <malloc.h>

struct slot {
	char data[4096];
	struct slot *next;
};

struct slot *expand(int num)
{
	struct slot *tmp, *head = NULL;
	while (num > 0) {
		tmp = (struct slot *)malloc(sizeof(struct slot));
		tmp->next = head;
		head = tmp;
		num--;
	}
	return head;
}

struct slot *head;

void sweeper(void)
{
	struct slot *p = head;
	while (p != NULL) {
		p->data[0] = (char)0xEF;
		p = p->next;
	}
	p = head;
	while (p != NULL) {
		assert(p->data[0] == (char)0xEF);
		p = p->next;
	}
	exit(0xbeaf);
}

int main(void)
{
	head = expand(6000);

	int pid, exit_code;
	if ((pid = fork()) == 0) {
		sweeper();
	}
	assert(pid > 0);
	cprintf("fork ok.\n");

	assert(waitpid(pid, &exit_code) == 0 && exit_code == 0xbeaf);
	cprintf("cowtest pass.\n");
	return 0;
}
