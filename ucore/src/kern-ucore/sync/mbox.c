#include <types.h>
#include <mmu.h>
#include <pmm.h>
#include <vmm.h>
#include <ipc.h>
#include <proc.h>
#include <slab.h>
#include <sem.h>
#include <mbox.h>
#include <mboxbuf.h>
#include <wait.h>
#include <list.h>
#include <error.h>
#include <assert.h>
#include <clock.h>
#include <string.h>

struct msg_seg {
	struct msg_seg *next;
};

struct msg_msg {
	int pid;
	unsigned int bytes;
	struct msg_seg *next;
	list_entry_t msg_link;
};

#define le2msg(le, member)              \
    to_struct((le), struct msg_msg, member)

enum mbox_state {
	CLOSED = 0,
	OPENED = 1,
	CLOSING = 2,
};

struct msg_mbox {
	int id;
	int inuse;
	enum mbox_state state;
	unsigned int max_slots, slots;
	list_entry_t msg_link;
	wait_queue_t senders;
	wait_queue_t receivers;
};

#define le2mbox(le, member)             \
    to_struct((le), struct msg_mbox, member)

#define MAX_MBOX_NUM                8192
#define MBOX_P_PAGE                 (PGSIZE / sizeof(struct msg_mbox))
#define MAX_MBOX_PAGES              ((MAX_MBOX_NUM + MBOX_P_PAGE - 1) / MBOX_P_PAGE)
#define MAX_MSG_DATALEN             (512 - sizeof(struct msg_msg))

static struct msg_mbox *mbox_map[MAX_MBOX_PAGES];
static list_entry_t free_mbox_list;
static semaphore_t sem_mbox_map;

void mbox_init(void)
{
	int i;
	for (i = 0; i < MAX_MBOX_PAGES; i++) {
		mbox_map[i] = NULL;
	}
	sem_init(&sem_mbox_map, 1);
	list_init(&free_mbox_list);
	static_assert(MBOX_P_PAGE != 0);
}

static struct msg_mbox *get_mbox(int id)
{
	if (id >= 0 && id < MAX_MBOX_NUM) {
		int i = id / MBOX_P_PAGE, j = id % MBOX_P_PAGE;
		if (mbox_map[i] != NULL) {
			struct msg_mbox *mbox = mbox_map[i] + j;
			if (mbox->state == OPENED) {
				return mbox;
			}
		}
	}
	return NULL;
}

static void mbox_free(struct msg_mbox *mbox)
{
	assert(mbox->state == CLOSING && mbox->inuse == 0);
	assert(list_empty(&(mbox->msg_link)));
	assert(wait_queue_empty(&(mbox->senders)));
	assert(wait_queue_empty(&(mbox->receivers)));
	mbox->state = CLOSED;
	mbox->max_slots = mbox->slots = 0;
	list_add_before(&(free_mbox_list), &(mbox->msg_link));
}

static void add_msg(struct msg_mbox *mbox, struct msg_msg *msg, bool append)
{
	assert(mbox->state == OPENED);
	mbox->slots++;
	list_entry_t *list = &(mbox->msg_link), *le = &(msg->msg_link);
	if (append) {
		list_add_before(list, le);
	} else {
		list_add_after(list, le);
	}
	wakeup_first(&(mbox->receivers), WT_MBOX_RECV, 1);
}

static int
pick_msg(struct msg_mbox *mbox, size_t max_bytes, struct msg_msg **msg_store)
{
	assert(mbox->state == OPENED && mbox->slots > 0);
	assert(!list_empty(&(mbox->msg_link)));
	struct msg_msg *msg = le2msg(list_next(&(mbox->msg_link)), msg_link);
	if (max_bytes < msg->bytes) {
		return -E_TOO_BIG;
	}
	mbox->slots--, *msg_store = msg;
	list_del(&(msg->msg_link));
	wakeup_first(&(mbox->senders), WT_MBOX_SEND, 1);
	return 0;
}

static struct msg_mbox *new_mbox(unsigned int max_slots)
{
	bool intr_flag;
	struct msg_mbox *mbox = NULL;
	local_intr_save(intr_flag);
	if (list_empty(&free_mbox_list)) {
		int i, id;
		for (i = 0; i < MAX_MBOX_PAGES; i++) {
			if (mbox_map[i] == NULL) {
				break;
			}
		}
		if (i == MAX_MBOX_PAGES) {
			goto out;
		}
		local_intr_restore(intr_flag);

		struct Page *page = alloc_page();

		local_intr_save(intr_flag);
		if (page != NULL) {
			id = i * MBOX_P_PAGE;
			mbox = mbox_map[i] = (struct msg_mbox *)page2kva(page);
			for (i = 0; i < MBOX_P_PAGE; i++, id++, mbox++) {
				mbox->id = id, mbox->inuse = 0;
				mbox->state = CLOSED;
				mbox->max_slots = mbox->slots = 0;
				list_init(&(mbox->msg_link));
				wait_queue_init(&(mbox->senders));
				wait_queue_init(&(mbox->receivers));
				list_add_before(&(free_mbox_list),
						&(mbox->msg_link));
			}
		} else if (list_empty(&free_mbox_list)) {
			goto out;
		}
	}
	assert(!list_empty(&free_mbox_list));
	mbox = le2mbox(list_next(&free_mbox_list), msg_link);
	list_del_init(&(mbox->msg_link));
	mbox->state = OPENED;
	mbox->max_slots = max_slots;

out:
	local_intr_restore(intr_flag);
	return mbox;
}

int ipc_mbox_init(unsigned int max_slots)
{
	if (max_slots == 0 || max_slots > MAX_MSG_SLOTS) {
		return -E_INVAL;
	}
	int ret = -E_NO_MEM;
	struct msg_mbox *mbox;
	down(&sem_mbox_map);
	if ((mbox = new_mbox(max_slots)) != NULL) {
		ret = mbox->id;
	}
	up(&sem_mbox_map);
	return ret;
}

static void free_seg(struct msg_seg *seg)
{
	if (seg->next != NULL) {
		free_seg(seg->next);
	}
	kfree(seg);
}

static void free_msg(struct msg_msg *msg)
{
	if (msg->next != NULL) {
		free_seg(msg->next);
	}
	kfree(msg);
}

static struct msg_msg *load_msg(const void *src, size_t len)
{
	size_t alen, bytes = len;
	if ((alen = len) > MAX_MSG_DATALEN) {
		alen = MAX_MSG_DATALEN;
	}
	struct msg_msg *msg;
	if ((msg = kmalloc(sizeof(struct msg_msg) + alen)) == NULL) {
		return NULL;
	}

	struct msg_seg **segp = &(msg->next);

	void *dst = msg + 1;
	goto inside;

	while (len > 0) {
		if ((alen = len) > MAX_MSG_DATALEN) {
			alen = MAX_MSG_DATALEN;
		}
		struct msg_seg *seg;
		if ((seg = kmalloc(sizeof(struct msg_seg) + alen)) == NULL) {
			goto failed;
		}
		*segp = seg, segp = &(seg->next);
		dst = seg + 1;
inside:
		copy_from_user(current->mm, dst, src, alen, 0);
		len -= alen, src = ((char *)src) + alen;
		*segp = NULL;
	}

	msg->bytes = bytes;
	msg->pid = current->pid;
	return msg;

failed:
	free_msg(msg);
	return NULL;
}

static uint32_t
send_msg(struct msg_mbox *mbox, struct msg_msg *msg, timer_t * timer)
{
	uint32_t ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	mbox->inuse++;
	wait_t __wait, *wait = &__wait;
	while (mbox->max_slots <= mbox->slots) {
		assert(mbox->state == OPENED);
		wait_current_set(&(mbox->senders), wait, WT_MBOX_SEND);
		ipc_add_timer(timer);
		local_intr_restore(intr_flag);

		schedule();

		local_intr_save(intr_flag);
		ipc_del_timer(timer);
		wait_current_del(&(mbox->senders), wait);
		if (mbox->state != OPENED || wait->wakeup_flags != WT_MBOX_SEND) {
			if ((ret = wait->wakeup_flags) == WT_MBOX_SEND) {
				ret = WT_INTERRUPTED;
			}
			goto out;
		}
	}
	assert(mbox->state == OPENED && mbox->max_slots > mbox->slots);

	ret = 0, add_msg(mbox, msg, 1);

out:
	mbox->inuse--;
	if (mbox->state != OPENED) {
		assert(ret != 0 && mbox->state == CLOSING);
		if (mbox->inuse == 0) {
			mbox_free(mbox);
		}
	}
	local_intr_restore(intr_flag);
	return ret;
}

int ipc_mbox_send(int id, struct mboxbuf *buf, unsigned int timeout)
{
	if (get_mbox(id) == NULL) {
		return -E_INVAL;
	}

	struct msg_msg *msg;
	struct msg_mbox *mbox;
	struct mm_struct *mm = current->mm;
	struct mboxbuf __local_buf, *local_buf = &__local_buf;

	int ret = -E_INVAL;

	lock_mm(mm);
	{
		if (copy_from_user
		    (mm, local_buf, buf, sizeof(struct mboxbuf), 0)) {
			size_t len = local_buf->len;
			if (0 < len && len <= MAX_MSG_BYTES) {
				void *src = local_buf->data;
				if (user_mem_check(mm, (uintptr_t) src, len, 0)) {
					ret =
					    ((msg =
					      load_msg(src,
						       len)) !=
					     NULL) ? 0 : -E_NO_MEM;
				}
			}
		}
	}
	unlock_mm(mm);

	if (ret == 0) {
		ret = -E_INVAL;
		if ((mbox = get_mbox(id)) != NULL) {
			unsigned long saved_ticks;
			timer_t __timer, *timer =
			    ipc_timer_init(timeout, &saved_ticks, &__timer);

			uint32_t flags;
			if ((flags = send_msg(mbox, msg, timer)) == 0) {
				return 0;
			}
			assert(flags == WT_INTERRUPTED);
			ret = ipc_check_timeout(timeout, saved_ticks);
		}
		free_msg(msg);
	}
	return ret;
}

static void store_msg(struct msg_msg *msg, void *dst)
{
	size_t alen, len = msg->bytes;
	if ((alen = len) > MAX_MSG_DATALEN) {
		alen = MAX_MSG_DATALEN;
	}

	struct msg_seg *seg = msg->next;

	const void *src = msg + 1;
	goto inside;

	while (len > 0) {
		if ((alen = len) > MAX_MSG_DATALEN) {
			alen = MAX_MSG_DATALEN;
		}
		assert(seg != NULL);
		src = seg + 1, seg = seg->next;
inside:
		copy_to_user(current->mm, dst, src, alen);
		len -= alen, dst = ((char *)dst) + alen;
	}
}

static int
recv_msg(struct msg_mbox *mbox, size_t max_bytes, struct msg_msg **msg_store,
	 timer_t * timer)
{
	int ret = -1;
	bool intr_flag;
	local_intr_save(intr_flag);
	mbox->inuse++;
	wait_t __wait, *wait = &__wait;
	while (mbox->slots == 0) {
		assert(mbox->state == OPENED);
		wait_current_set(&(mbox->receivers), wait, WT_MBOX_RECV);
		ipc_add_timer(timer);
		local_intr_restore(intr_flag);

		schedule();

		local_intr_save(intr_flag);
		ipc_del_timer(timer);
		wait_current_del(&(mbox->receivers), wait);
		if (mbox->state != OPENED || wait->wakeup_flags != WT_MBOX_RECV) {
			goto out;
		}
	}
	assert(mbox->state == OPENED && mbox->slots > 0);
	assert(!list_empty(&(mbox->msg_link)));

	if ((ret = pick_msg(mbox, max_bytes, msg_store)) != 0) {
		wakeup_first(&(mbox->receivers), WT_MBOX_RECV, 1);
	}

out:
	mbox->inuse--;
	if (mbox->state != OPENED) {
		assert(ret != 0 && mbox->state == CLOSING);
		if (mbox->inuse == 0) {
			mbox_free(mbox);
		}
	}
	local_intr_restore(intr_flag);
	return ret;
}

int ipc_mbox_recv(int id, struct mboxbuf *buf, unsigned int timeout)
{
	if (get_mbox(id) == NULL) {
		return -E_INVAL;
	}

	size_t size;
	struct msg_msg *msg;
	struct msg_mbox *mbox;
	struct mm_struct *mm = current->mm;
	struct mboxbuf __local_buf, *local_buf = &__local_buf;

	int ret = -E_INVAL;

	lock_mm(mm);
	{
		if (copy_from_user
		    (mm, local_buf, buf, sizeof(struct mboxbuf), 1)) {
			if ((size = local_buf->size) > 0) {
				void *dst = local_buf->data;
				if (user_mem_check
				    (mm, (uintptr_t) dst, size, 1)) {
					ret = 0;
				}
			}
		}
	}
	unlock_mm(mm);

	if (ret != 0 || (mbox = get_mbox(id)) == NULL) {
		return -E_INVAL;
	}

	unsigned long saved_ticks;
	timer_t __timer, *timer =
	    ipc_timer_init(timeout, &saved_ticks, &__timer);
	if ((ret = recv_msg(mbox, size, &msg, timer)) != 0) {
		if (ret == -1) {
			return ipc_check_timeout(timeout, saved_ticks);
		}
		return ret;
	}

	ret = -E_INVAL;

	lock_mm(mm);
	{
		size_t len;
		local_buf->len = len = msg->bytes, local_buf->from = msg->pid;
		if (copy_to_user(mm, buf, local_buf, sizeof(struct mboxbuf))) {
			void *dst = local_buf->data;
			if (user_mem_check(mm, (uintptr_t) dst, len, 1)) {
				ret = 0, store_msg(msg, dst);
			}
		}
	}
	unlock_mm(mm);

	if (ret != 0 && (mbox = get_mbox(id)) != NULL) {
		bool local_intr;
		local_intr_save(local_intr);
		{
			add_msg(mbox, msg, 0);
		}
		local_intr_restore(local_intr);
		return ret;
	}
	free_msg(msg);
	return ret;
}

int ipc_mbox_free(int id)
{
	struct msg_mbox *mbox;
	if ((mbox = get_mbox(id)) == NULL) {
		return -E_INVAL;
	}
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		mbox->state = CLOSING;

		list_entry_t *list = &(mbox->msg_link), *le;
		while ((le = list_next(list)) != list) {
			list_del(le);
			free_msg(le2msg(le, msg_link));
		}
		wakeup_queue(&(mbox->senders), WT_INTERRUPTED, 1);
		wakeup_queue(&(mbox->receivers), WT_INTERRUPTED, 1);

		if (mbox->inuse == 0) {
			mbox_free(mbox);
		}
	}
	local_intr_restore(intr_flag);
	return 0;
}

int ipc_mbox_info(int id, struct mboxinfo *info)
{
	struct msg_mbox *mbox;
	if ((mbox = get_mbox(id)) == NULL) {
		return -E_INVAL;
	}

	struct mm_struct *mm = current->mm;
	struct mboxinfo __local_info, *local_info = &__local_info;

	local_info->slots = mbox->slots;
	local_info->max_slots = mbox->max_slots;
	local_info->inuse = (mbox->inuse != 0);
	local_info->has_sender = !wait_queue_empty(&(mbox->senders));
	local_info->has_receiver = !wait_queue_empty(&(mbox->receivers));

	int ret;

	lock_mm(mm);
	{
		ret =
		    (copy_to_user
		     (mm, info, local_info,
		      sizeof(struct mboxinfo))) ? 0 : -E_INVAL;
	}
	unlock_mm(mm);
	return ret;
}

void mbox_cleanup(void)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		int i, j;
		for (i = 0; i < MAX_MBOX_PAGES; i++) {
			struct msg_mbox *mbox;
			if ((mbox = mbox_map[i]) != NULL) {
				for (j = 0; j < MBOX_P_PAGE; j++, mbox++) {
					if (mbox->state != CLOSED) {
						break;
					}
				}
				if (j != MBOX_P_PAGE) {
					continue;
				}
				mbox = mbox_map[i];
				for (j = 0; j < MBOX_P_PAGE; j++, mbox++) {
					list_del(&(mbox->msg_link));
				}
				mbox = mbox_map[i], mbox_map[i] = NULL;
				free_page(kva2page(mbox));
			}
		}
	}
	local_intr_restore(intr_flag);
}
