#include <host_syscall.h>
#include <proc.h>
#include <memlayout.h>
#include <stdio.h>
#include <stub.h>
#include <trap.h>
#include <pmm.h>
#include <vmm.h>
#include <arch.h>
#include <kio.h>

/**
 * Carry out the syscall specified in the stub stack.
 * @param proc the PCB of the process to execute the stub call
 * @return the return value of the call
 */
static int host_syscall_in_child(struct proc_struct *proc)
{
	struct user_regs_struct regs;
	int err, pid = proc->arch.host->host_pid;

	err = syscall4(__NR_ptrace, PTRACE_GETREGS, pid, 0, (long)&regs);
	if (err < 0) {
		kprintf("host_syscall_in_child: cannot get regs\n");
		return -1;
	}

	/* 'stub_exec_syscall' is the entry of all stub calls. */
	regs.eip =
	    STUB_CODE + (unsigned long)stub_exec_syscall -
	    (unsigned long)&__syscall_stub_start;
	err = syscall4(__NR_ptrace, PTRACE_SETREGS, pid, 0, (long)&regs);
	if (err < 0) {
		kprintf("host_syscall_in_child: cannot set regs\n");
		return -1;
	}
	err = syscall4(__NR_ptrace, PTRACE_CONT, pid, 0, 0);
	if (err < 0) {
		kprintf("host_syscall_in_child: cannot continue the child\n");
		return -1;
	}
	err = wait_stub_done(pid);
	if (err < 0) {
		kprintf
		    ("host_syscall_in_child: wait stub done failed with err = %d\n",
		     err);
		return -1;
	}
	err = current_stub_frame(proc->arch.host->stub_stack)->eax;
	return err;
}

/**
 * Map a part of file to the container process when we are in the main one.
 * @param proc the PCB whose container process is going to be mapped
 * @param addr the address where to map the content (should be the beginning of a page)
 * @param length the size of the content (should be a multiple of the PGSIZE)
 * @param prot map access properties. see 'man mmap' for details
 * @param flags map flags. see 'man mmap' for details
 * @param fd the file descriptor of the file to be mapped
 * @param offset the offset of the content to be mapped in the file
 * @return same as host syscall 'mmap' (beginning address of the mapped area on success)
 */
int
host_mmap(struct proc_struct *proc,
	  void *addr, size_t length, int prot, int flags,
	  int fd, uint32_t offset)
{
	struct stub_stack *stub_stack = proc->arch.host->stub_stack;
	struct stub_frame *frame = (current_stub_frame(stub_stack));

	frame->eax = __NR_mmap;
	frame->ebx =
	    STUB_DATA + (unsigned long)frame->data -
	    (unsigned long)(stub_stack);
	struct mmap_arg_struct *args = (struct mmap_arg_struct *)frame->data;
	args->addr = (long)addr;
	args->len = length;
	args->prot = prot;
	args->flags = flags;
	args->fd = fd;
	args->offset = offset;
	return host_syscall_in_child(proc);
}

/**
 * Unmap a range of the container process's space when we are in the main one.
 * @param proc the PCB whose container process is going to be operated on
 * @param addr the beginning address of the area to be unmapped (should be the beginning of a page)
 * @param length the size of the area to be unmapped (should be a multiple of PGSIZE)
 */
int host_munmap(struct proc_struct *proc, void *addr, size_t length)
{
	struct stub_stack *stub_stack = proc->arch.host->stub_stack;
	struct stub_frame *frame = current_stub_frame(stub_stack);

	frame->eax = __NR_munmap;
	frame->ebx = (long)addr;
	frame->ecx = length;
	return host_syscall_in_child(proc);
}

/**
 * Write a word to the container process's vm when we are in the main one
 *     This is only used to 'touch' the page as written so the page table will be right.
 *     See 'copy_to_user' in arch/um/mm/vmm.c for details.
 * @param proc the PCB whose container process will be touched
 * @param addr the address of the word to be touched (should be valid in the container process)
 * @param data the data to be written (of no use now)
 * @return 0 on success, or a negative otherwise
 */
int host_assign(struct proc_struct *proc, uintptr_t addr, uint32_t data)
{
	if (!user_mem_check(proc->mm, addr, sizeof(uintptr_t), 1)) {
		kprintf("try assigning invalid address: 0x%x\n", addr);
		return -1;
	}

	/* Use stub call so that the page table will be modified properly */
	struct stub_stack *stub_stack = proc->arch.host->stub_stack;
	struct stub_frame *frame = current_stub_frame(stub_stack);
	frame->eax = -1;
	frame->ebx = addr;
	frame->ecx = data;

	return host_syscall_in_child(proc);
}

/**
 * Read a word from the container process's vm when we are in the main one
 *     This is only used to 'touch' the page as read so the page table will be right.
 *     See 'copy_from_user' in arch/um/mm/vmm.c for details.
 * @param proc the PCB whose container process will be touched
 * @param addr the address of the word to be touched (should be valid in the container process)
 * @param data the data to be read (of no use now)
 * @return 0 on success, or a negative otherwise
 */
int host_getvalue(struct proc_struct *proc, uintptr_t addr, uint32_t * data)
{
	if (!user_mem_check(proc->mm, addr, sizeof(uintptr_t), 0)) {
		return -1;
	}

	struct stub_stack *stub_stack = proc->arch.host->stub_stack;
	struct stub_frame *frame = current_stub_frame(stub_stack);
	frame->eax = 0;
	frame->ebx = addr;
	int ret = host_syscall_in_child(proc);
	if (data != NULL)
		*data = ret;
	return 0;
}

/**
 * Map a range of child's virtual memory to the main process.
 *     This is only used in sys_getcwd, for the 'buf' will be used as an io buffer,
 *     and it is impossible to use 'copy_to_user' everywhere we write to the io_buf.
 * @param proc the PCB whose container process is the source
 * @param addr the beginning address of the area to be mapped
 * @param len the size of the area
 * @param is_write whether it is required that the area is writable
 * @return 0 on success, or -1 otherwise
 */
int
host_map_user(struct proc_struct *proc, uintptr_t addr, size_t len,
	      int is_write)
{
	if (proc->mm == NULL)
		return 0;
	uintptr_t start = ROUNDDOWN(addr, PGSIZE), end =
	    ROUNDUP(addr + len, PGSIZE);
	pde_t *pgdir = proc->mm->pgdir;

	int i, ret;
	/* Prepare for common arguments of the syscall. */
	struct mmap_arg_struct args = {
		.len = PGSIZE,
		.prot = PROT_READ | PROT_WRITE | PROT_EXEC,
		.flags = MAP_SHARED | MAP_FIXED,
		.fd = ginfo->mem_fd,
	};
	for (i = start; i < end; i += PGSIZE) {
		/* Touch the page */
		if (is_write)
			ret = host_assign(proc, addr, 0);
		else
			ret = host_getvalue(proc, addr, NULL);
		if (ret < 0)
			return -1;

		/* Find the page in the tmp file and map it to the proper address in the main process. */
		pte_t *ptep = get_pte(pgdir, addr, 0);
		assert(ptep != NULL && (*ptep & PTE_P));
		args.addr = i;
		args.offset = PTE_ADDR(*ptep);
		syscall1(__NR_mmap, (long)&args);
	}

	return 0;
}
