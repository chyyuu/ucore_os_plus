#include <memmap.h>
#include <stdio.h>

/* Store the final physical memory map */
static unsigned int memmap_count;
static uintptr_t memmap_addr[MEMMAP_SCALE];

static unsigned int mmnode_count;
static uintptr_t mmnode_addr[MEMMAP_SCALE];
static int mmnode_flag[MEMMAP_SCALE];

/* return the first boundary index that has value larger than addr */
static inline int memmap_bin_search(uintptr_t addr)
{
	int l = 0, r = memmap_count - 1, m;
	while (l < r) {
		if (memmap_addr[m = ((l + r) >> 1)] > addr)
			r = m;
		else
			l = m + 1;
	}

	return l;
}

int memmap_test(uintptr_t start, uintptr_t end)
{
	unsigned int
	    i = memmap_bin_search(start), j = memmap_bin_search(end - 1);
	return (i & 1) && (i == j);
}

int memmap_enumerate(int num, uintptr_t * start, uintptr_t * end)
{
	if ((num + 1) << 1 > memmap_count)
		return -1;

	*start = memmap_addr[num << 1];
	*end = memmap_addr[(num << 1) + 1];

	return 0;
}

void memmap_reset(void)
{
	memmap_count = 0;
	mmnode_count = 0;
}

int memmap_append(uintptr_t start, uintptr_t end, int flag)
{
	mmnode_addr[mmnode_count] = start;
	mmnode_flag[mmnode_count] = flag;
	++mmnode_count;

	mmnode_addr[mmnode_count] = end;
	mmnode_flag[mmnode_count] = -flag;
	++mmnode_count;

	return 0;
}

void memmap_process(int verbose)
{
	unsigned int i, j, k;
	/* Sort the mmap node table with O(n^2) sorting */
	for (i = 0; i != mmnode_count; ++i) {
		j = i;
		for (k = i + 1; k != mmnode_count; ++k) {
			if (mmnode_addr[k] < mmnode_addr[j])
				j = k;
		}
		uintptr_t t;
		int f;
		t = mmnode_addr[i];
		mmnode_addr[i] = mmnode_addr[j];
		mmnode_addr[j] = t;
		f = mmnode_flag[i];
		mmnode_flag[i] = mmnode_flag[j];
		mmnode_flag[j] = f;
	}

	/* Make memory layout table */
	int vaild_level, reclaim_level;
	int last_vaild, v;

	vaild_level = 0;
	reclaim_level = 0;
	v = 0;
	memmap_count = 0;

	i = 0;
	while (1) {
		if (i >= mmnode_count)
			break;

		last_vaild = v;
		// cprintf("DEBUG: VL[%d] RL[%d] ADDR[%p] FLAG[%d]\n", vaild_level, reclaim_level, mmnode_addr[i], mmnode_flag[i]);
		while (1) {
			switch (mmnode_flag[i]) {
			case MEMMAP_FLAG_FREE:
				++vaild_level;
				break;
			case -MEMMAP_FLAG_FREE:
				--vaild_level;
				break;
			case MEMMAP_FLAG_RECLAIMABLE:
				++reclaim_level;
				break;
			case -MEMMAP_FLAG_RECLAIMABLE:
				--reclaim_level;
				break;
			default:
				if (mmnode_flag[i] > 0)
					--vaild_level;
				else
					++vaild_level;
				break;
			}

			++i;
			if (i >= mmnode_count)
				break;
			if (mmnode_addr[i - 1] != mmnode_addr[i])
				break;
		}

		if (vaild_level == 0)
			v = (reclaim_level > 0);
		else
			v = (vaild_level > 0);

		if (v != last_vaild) {
			memmap_addr[memmap_count] = mmnode_addr[i - 1];
			++memmap_count;
		}
	}

	if (verbose) {
		cprintf("Physical memory layout boundary:\n");
		for (i = 0; i != memmap_count; ++i) {
			cprintf(" [%d] = %p\n", i, memmap_addr[i]);
		}
	}
}
