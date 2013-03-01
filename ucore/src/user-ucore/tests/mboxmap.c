#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <mboxbuf.h>
#include <malloc.h>
#include <stdlib.h>

const int mod = 23;
const int max_data = 2048;
const int max_slots = 1024;

int send(int id, void *data, size_t len)
{
	struct mboxbuf buf;
	buf.data = data, buf.len = len;
	return mbox_send(id, &buf);
}

int recv(int id, void *data, size_t size, size_t * lenp)
{
	struct mboxbuf buf;
	buf.data = data, buf.size = size;
	int ret;
	if ((ret = mbox_recv(id, &buf)) == 0) {
		*lenp = buf.len;
	}
	return ret;
}

void filter_main(int *data, int mbox_data, int mbox[])
{
	int i, count[mod];
	size_t size = max_data * sizeof(int), len;
	while (recv(mbox_data, data, size, &len) == 0) {
		assert((len % sizeof(int)) == 0);
		memset(count, 0, sizeof(count));
		len /= sizeof(int);
		for (i = 0; i < len; i++) {
			count[data[i] % mod]++;
		}
		for (i = 0; i < mod; i++) {
			send(mbox[i], count + i, sizeof(int));
		}
	}
}

void select_main(int mbox, int pid)
{
	size_t len;
	int count = 0, ans;
	while (recv(mbox, &ans, sizeof(int), &len) == 0) {
		assert(len == sizeof(int));
		count += ans;
	}
	send_event(pid, count);
}

int wait_for_empty(int mbox)
{
	struct mboxinfo info;
	while (1) {
		if (mbox_info(mbox, &info) != 0) {
			return -1;
		}
		if (info.slots == 0) {
			return 0;
		}
		sleep(10);
	}
}

int wait_for_quit(int mbox_data, int count[], int mbox[], int pids[])
{
	int i, j, pid, event, ans[mod];
	memset(ans, 0, sizeof(ans));
	for (i = 0; i < mod; i++) {
		if (recv_event(&pid, &event) != 0) {
			return -1;
		}
		for (j = 0; j < mod; j++) {
			if (pids[j] == pid) {
				ans[j] = event;
				cprintf("-- recv count %02d: %08d\n", j, event);
			}
		}
	}

	int err = 0;
	for (i = 0; i < mod; i++) {
		if (count[i] != ans[i]) {
			err++;
			cprintf("wrong: %d, %d, %d.\n", i, count[i], ans[i]);
		}
	}
	return err;
}

int main(void)
{
	int i, j, k, mbox[mod], count[mod];
	for (i = 0; i < mod; i++) {
		mbox[i] = mbox_init(max_slots);
		assert(mbox[i] >= 0);
	}

	int mbox_data = mbox_init(max_slots);
	assert(mbox_data >= 0);

	size_t len, size;
	len = size = max_data * sizeof(int);

	int *data = malloc(size);
	assert(data != NULL);

	int s_pids[mod], this = getpid();
	memset(s_pids, 0, sizeof(s_pids));

	for (i = 0; i < mod; i++) {
		if ((s_pids[i] = fork()) == 0) {
			select_main(mbox[i], this);
			exit(0);
		}
		if (s_pids[i] < 0) {
			goto kill_selector;
		}
	}

	int f_pids[100], f_pids_num = sizeof(f_pids) / sizeof(f_pids[0]);
	memset(f_pids, 0, sizeof(f_pids));

	for (i = 0; i < f_pids_num; i++) {
		if ((f_pids[i] = fork()) == 0) {
			filter_main(data, mbox_data, mbox);
			exit(0);
		}
		if (f_pids[i] < 0) {
			goto kill_filter;
		}
	}

	cprintf("fork children ok.\n");

	memset(count, 0, sizeof(count));

	srand(1481);
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 512; j++) {
			for (k = 0; k < max_data; k++) {
				data[k] = rand();
				count[data[k] % mod]++;
			}
			if (send(mbox_data, data, len) != 0) {
				cprintf("send data error!!\n");
				goto kill_filter;
			}
		}
		cprintf("round %d\n", i);
	}

	for (i = 0; i < mod; i++) {
		cprintf("-- send count %02d: %08d\n", i, count[i]);
	}

	if (wait_for_empty(mbox_data) == 0) {
		mbox_free(mbox_data);
		int exit_code, err = 0;
		for (i = 0; i < f_pids_num; i++) {
			waitpid(f_pids[i], &exit_code);
		}
		for (i = 0; i < mod; i++) {
			if (wait_for_empty(mbox[i]) != 0) {
				err++;
			}
		}
		if (err == 0) {
			cprintf("wait children ok.\n");
			for (i = 0; i < mod; i++) {
				mbox_free(mbox[i]);
			}
			if (wait_for_quit(mbox_data, count, mbox, s_pids) == 0) {
				cprintf("mboxmap pass.\n");
				return 0;
			}
		}
	}

kill_filter:
	for (i = 0; i < f_pids_num; i++) {
		if (f_pids[i] > 0) {
			kill(f_pids[i]);
		}
	}

kill_selector:
	for (i = 0; i < mod; i++) {
		if (s_pids[i] > 0) {
			kill(s_pids[i]);
		}
	}
	panic("FAIL: T.T\n");
}
