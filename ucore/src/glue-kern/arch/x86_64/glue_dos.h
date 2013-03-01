#ifndef __GLUE_DOS_H__
#define __GLUE_DOS_H__

#define driver_os_is_enabled      (*driver_os_is_enabled_ptr)
#define driver_os_notify          (*driver_os_notify_ptr)
#define driver_os_buffer_get      (*driver_os_buffer_get_ptr)
#define driver_os_buffer_size_get (*driver_os_buffer_size_get_ptr)

extern int driver_os_is_enabled(void);
extern void driver_os_notify(void);
extern volatile void *driver_os_buffer_get(void);
extern uint64_t driver_os_buffer_size_get(void);

#endif
