#include <stdio.h>
#include <string.h>
#include <ulib.h>
#include <mboxbuf.h>
#include <malloc.h>
#include <error.h>

const int mbox_slots = 128;

void mbox_test(void)
{
	int mbox_id = mbox_init(1);
	assert(mbox_id >= 0);

	struct mboxbuf __buf, *buf = &__buf;
	buf->size = 4096;
	buf->len = buf->size;
	buf->data = (void *)0xF0000000;
	assert(mbox_send(mbox_id, buf) < 0);

	buf->data = malloc(sizeof(char) * buf->size);
	assert(buf->data != NULL);

	char *data = (char *)(buf->data);
	int i;
	for (i = 0; i < buf->size; i++) {
		data[i] = (char)i;
	}
	buf->len = buf->size;

	unsigned int timeout = 100, saved_msec = gettime_msec();

	assert(mbox_send(mbox_id, buf) == 0);
	assert(mbox_send_timeout(mbox_id, buf, timeout) == -E_TIMEOUT);
	assert((unsigned int)(gettime_msec() - saved_msec) >= timeout);

	size_t saved_size = buf->size;
	buf->size = 100;
	assert(mbox_recv(mbox_id, buf) != 0);

	buf->size = saved_size - 1;
	assert(mbox_recv(mbox_id, buf) != 0);

	buf->size = saved_size;
	memset(buf->data, 0, sizeof(char) * buf->size);
	assert(mbox_recv(mbox_id, buf) == 0);

	assert(buf->size == saved_size && buf->len == saved_size);
	data = (char *)(buf->data);
	for (i = 0; i < buf->size; i++) {
		assert(data[i] == (char)i);
	}

	saved_msec = gettime_msec();
	assert(mbox_recv_timeout(mbox_id, buf, timeout) == -E_TIMEOUT);
	assert((unsigned int)(gettime_msec() - saved_msec) >= timeout);

	assert(mbox_free(mbox_id) == 0);
	assert(mbox_send(mbox_id, buf) != 0);

	exit(0);
}

int main(void)
{
	int pid, ret;
	if ((pid = fork()) == 0) {
		mbox_test();
	}
	assert(pid > 0 && waitpid(pid, &ret) == 0 && ret == 0);
	cprintf("mboxtest pass.\n");
	return 0;
}
