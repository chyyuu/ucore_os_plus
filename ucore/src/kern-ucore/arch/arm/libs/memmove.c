#include <arm.h>

void *__memmove(void *dst, const void *src, size_t n)
{
	if (dst < src) {
		return __memcpy(dst, src, n);
	}
	const char *s = src;
	char *d = dst;
	if (s < d && s + n > d) {
		s += n, d += n;
		while (n-- > 0) {
			*--d = *--s;
		}
	} else {
		while (n-- > 0) {
			*d++ = *s++;
		}
	}
	return dst;
}
