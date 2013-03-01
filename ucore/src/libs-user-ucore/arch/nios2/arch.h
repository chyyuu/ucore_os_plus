#ifndef __USER_LIBS_NIOS2_H__
#define __USER_LIBS_NIOS2_H__

#define do_div(n, base) ({                                          \
            unsigned long __mod;                                    \
            __mod = (n) % (base);                                   \
            (n) /= (base);                                          \
            __mod;                                                  \
        })

#endif /* !__USER_LIBS_NIOS2_H__ */
