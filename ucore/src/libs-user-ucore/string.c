#include <string.h>
#include <malloc.h>

char *strdup(const char *src)
{
	char *dst;
	size_t len = strlen(src) + 1;
	if ((dst = malloc(len)) != NULL) {
		memcpy(dst, src, len);
	}
	return dst;
}
