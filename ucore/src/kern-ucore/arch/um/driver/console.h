#ifndef __KERN_DRIVER_CONSOLE_H__
#define __KERN_DRIVER_CONSOLE_H__

void cons_init(void);
void cons_putc(int c);
int cons_getc(void);
void serial_intr(void);
void kbd_intr(void);

/* ADDED BY ETERNALNIGHT */
/* Close tty and reset terminal is needed on linux. */
void cons_dtor(void);

/* The input buffer size */
#define CONSOLE_BUFFER_SIZE 10

#endif /* !__KERN_DRIVER_CONSOLE_H__ */
