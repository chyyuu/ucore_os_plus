#include <ulib.h>
#include <stdio.h>
#include <file.h>

int main(void)
{
	static char buffer[1024];
	while (1) {
		int ret;
		if ((ret = read(0, buffer, sizeof(buffer))) <= 0) {
			return ret;
		}
		write(1, buffer, ret);
	}
}
