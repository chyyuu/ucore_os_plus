#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

extern void fb_init(void);
extern void fb_write(unsigned char ch);
extern void fb_init_mmu(void);

#endif /* FRAMEBUFFER_H */
