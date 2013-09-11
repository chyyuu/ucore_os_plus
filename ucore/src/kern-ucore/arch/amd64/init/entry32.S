#include <mmu.h>
#include <memlayout.h>

#define MULTIBOOT_PAGE_ALIGN    (1<<0)
#define MULTIBOOT_MEMORY_INFO   (1<<1)
#define MULTIBOOT_EXTRA_INFO   (1<<16)

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_EXTRA_INFO)
#define CHECKSUM  -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.text
.code32
   jmp kern_entry32
.align 4
multiboot_header:
   .long MULTIBOOT_HEADER_MAGIC
   .long MULTIBOOT_HEADER_FLAGS
   .long CHECKSUM
   .long multiboot_header
   .long multiboot_header
   .long (edata - KERNBASE)
   .long (end - KERNBASE)
   .long kern_entry32

.align 4
.global kern_entry32
kern_entry32:
    #save boot param
    movl %eax, %edi
    movl %ebx, %esi
    movl $bootparam, %eax
    movl %edi, 0x0(%eax)
    movl %esi, 0x4(%eax)

    call savecmdline
    # Detection of CPUID:
    #  Basically, detecting wheterh CPUID is supported is covered here.
    #  CPUID is supported when the ID-bit in the FLAGS-register can be flipped.
    pushf                                           # Store FLAGS-register to stack
    popl %eax                                       # Restore the A-register
    movl %eax, %ecx                                 # Copy the A-register to C-register
    xorl $FL_ID, %eax                               # Flip the ID-bit
    pushl %eax                                      # Store the A-register
    popf                                            # Restore the FLAGS-register

    pushf                                           # Store the FLAGS-register
    popl %eax                                       # Restore the A-register
    pushl %ecx                                      # Store the C-register
    popf                                            # Restore the FLAGS-register
    xor %eax, %ecx                                  # Do a XOR-operation
    jz spin_nocpuid

    # Detect x86 or x86_64
    movl $0x80000000, %eax                          # Set the A-register to $0x80000000
    cpuid                                           # CPU identification
    cmpl $0x80000001, %eax                          # Compare the A-register with $0x80000001
    jb spin_nolongm                                 # It is less, there is no long mode.

    movl $0x80000001, %eax                          # Set the A-register to $0x80000001
    cpuid                                           # CPU identification
    test $0x20000000, %edx                          # Test if the LM-bit is set in the D-register
    jz spin_nolongm                                 # They aren't, there is no long mode.

    # Build page table for long-mode
    cld

    # step1: set pgd entry, mark pud as (PTE_W | PTE_P)
    movl $pgd, %edi
    movl $pud, %eax
    orl $0x3, %eax
    movl %eax, (%edi)
    movl %eax, 0x800(%edi)
	movl %eax, 0x900(%edi)

    # clear and set pud, mark pmd as (PTE_W | PTE_P)
    movl $pud, %edi
    movl $pmd, %eax
    orl $0x3, %eax
    movl %eax, (%edi)
    addl $4096, %eax
    movl %eax, 0x08(%edi)
    addl $4096, %eax
    movl %eax, 0x10(%edi)
    addl $4096, %eax
    movl %eax, 0x18(%edi)

    # set pmd, Mark each entry as (PTE_W | PTE_P | PTE_PS)
    movl $pmd, %edi
    movl $0x83, %ebx
    movl $0x800, %ecx                               # 512 entries

    # map 2M * 512 * 4 = 4G memory
    # this should be enough for all IO mapped mem
loop:
    movl %ebx, (%edi)
    addl $0x8, %edi
    addl $0x200000, %ebx
    subl $0x1, %ecx
    cmpl $0x0, %ecx
    jne loop

    # Prepare for long-mode, set (CR4_PGE | CR4_PAE)
    movl $0x20, %eax
    movl %eax, %cr4

    # set cr3
    movl $pgd, %eax
    movl %eax, %cr3

    # enable long-mode
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x00000100, %eax
    wrmsr

    # Active long-mode
    movl %cr0, %eax
    orl $0x80000001, %eax
    movl %eax, %cr0

    movl $bootparam, %esi

    movl $gdtdesc, %edi
    lgdt (%edi)
    ljmp $KERNEL_CS, $longmode_entry

spin:
    jmp spin

spin_nocpuid:
    jmp spin_nocpuid

spin_nolongm:
    jmp spin_nolongm

.global apstart
apstart:
    # Prepare for long-mode, set (CR4_PGE | CR4_PAE)
    movl $0x20, %eax
    movl %eax, %cr4

    # set cr3
    movl $pgd, %eax
    movl %eax, %cr3

    # enable long-mode
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x00000100, %eax
    wrmsr

    # Active long-mode
    movl %cr0, %eax
    orl $0x80000001, %eax
    movl %eax, %cr0

    movl $gdtdesc, %edi
    lgdt (%edi)
    ljmp $KERNEL_CS, $longmode_ap_entry

.code32        
savecmdline:
        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx

        // Check for for cmdline in flags
        movl (%esi), %eax
        and $(1<<2), %eax
        jz 2f

        // Save null-terminated cmdline
        movl 0x10(%esi), %eax
        movl $cmdline, %ebx
        movl $255, %ecx
1:      movb (%eax), %dl
        movb %dl, (%ebx)
        cmp $0, %dl
        je 2f
        inc %eax
        inc %ebx
        loop 1b

2:      popl %eax
        popl %ebx
        popl %ecx
        popl %edx
        ret


.code64
longmode_ap_entry:
    # Set up the protected-mode data segment registers
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    # Jump to kern_entry64
    movq $KERNBASE, %rax
    addq $kern_ap_entry64 - KERNBASE, %rax
    jmp *%rax


# %esi = bootparam phy addr
# long-mode code here
.code64
longmode_entry:
    # Set up the protected-mode data segment registers
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    movq $KERNBASE, %rax
    addq %rax, %rsi
    movl 0x00(%rsi), %edi
    movl 0x04(%rsi), %esi

    # Jump to kern_entry64
    movq $KERNBASE, %rax
    addq $kern_entry64 - KERNBASE, %rax
    jmp *%rax

.data
.align 4
gdt:
    SEG_NULL()
    SEG_CODE(STA_X | STA_R)
    SEG_DATA(STA_W)

gdtdesc:
    .word 0x2f
    .quad gdt

.global cmdline
cmdline:
	.space 256

bootparam:
	.long 0
	.long 0

.align PGSIZE
pgd:
    .rept 1024
    .long 0x0
    .endr
pud:
    .rept 1024
    .long 0x0
    .endr
pmd:
    .rept (1024*4)
    .long 0x0
    .endr

