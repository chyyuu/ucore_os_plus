
#ifndef MAILBOX_H
#define MAILBOX_H

extern unsigned int readmailbox(unsigned int channel);
extern void writemailbox(unsigned int channel, unsigned int data);

#endif /* MAILBOX_H */
