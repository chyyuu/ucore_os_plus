#ifndef __USER_LIBS_ULIB_H__
#define __USER_LIBS_ULIB_H__

#include <types.h>

void __warn(const char *file, int line, const char *fmt, ...);
void __panic(const char *file, int line, const char *fmt, ...)
    __attribute__ ((noreturn));

#define warn(...)                                       \
    __warn(__FILE__, __LINE__, __VA_ARGS__)

#define panic(...)                                      \
    __panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                                       \
    do {                                                \
        if (!(x)) {                                     \
            panic("assertion failed: %s", #x);          \
        }                                               \
    } while (0)

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)                                \
    switch (x) { case 0: case (x): ; }

int fprintf(int fd, const char *fmt, ...);

void exit(int error_code) __attribute__ ((noreturn));
int fork(void);
int forks(void);
int wait(void);
int waitpid(int pid, int *store);
void yield(void);
int sleep(unsigned int time);
int kill(int pid);
unsigned int gettime_msec(void);
int getpid(void);
void print_pgdir(void);
int mmap(uintptr_t * addr_store, size_t len, uint32_t mmap_flags);
int munmap(uintptr_t addr, size_t len);
int shmem(uintptr_t * addr_store, size_t len, uint32_t mmap_flags);
int clone(uint32_t clone_flags, uintptr_t stack, int (*fn) (void *), void *arg);
sem_t sem_init(int value);
int sem_post(sem_t sem_id);
int sem_wait(sem_t sem_id);
int sem_wait_timeout(sem_t sem_id, unsigned int timeout);
int sem_free(sem_t sem_id);
int sem_get_value(sem_t sem_id, int *value_store);
int send_event(int pid, int event);
int send_event_timeout(int pid, int event, unsigned int timeout);
int recv_event(int *pid_store, int *event_store);
int recv_event_timeout(int *pid_store, int *event_store, unsigned int timeout);

struct mboxbuf;
struct mboxinfo;

int mbox_init(unsigned int max_slots);
int mbox_send(int id, struct mboxbuf *buf);
int mbox_send_timeout(int id, struct mboxbuf *buf, unsigned int timeout);
int mbox_recv(int id, struct mboxbuf *buf);
int mbox_recv_timeout(int id, struct mboxbuf *buf, unsigned int timeout);
int mbox_free(int id);
int mbox_info(int id, struct mboxinfo *info);

int __exec(const char *name, const char **argv, const char **envp);

#define __exec0(name, path, ...)                \
    ({ const char *argv[] = {path, ##__VA_ARGS__, NULL}; __exec(name, argv, NULL); })

#define exec(path, ...)                         __exec0(NULL, path, ##__VA_ARGS__)
#define nexec(name, path, ...)                  __exec0(name, path, ##__VA_ARGS__)

void halt(void);

#endif /* !__USER_LIBS_ULIB_H__ */
