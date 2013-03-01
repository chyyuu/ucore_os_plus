#include <rf212.h>
#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void usage()
{
	cprintf("usage : wireless chat\n");
	cprintf("                 reg <reg_num> <reg_value>\n");
	cprintf("                 reset\n");
	cprintf("                 power <tx_power_dbm>\n");
	cprintf("                 mode <kbit_per_second>\n");
}

#define BUFSIZE 127
char *readline(const char *prompt)
{
	static char buffer[BUFSIZE] = { 0xFF };

	int ret, i = 1;
	while (1) {
		char c;
		if ((ret = read(0, &c, sizeof(char))) < 0) {
			return NULL;
		} else if (ret == 0) {
			if (i > 0) {
				buffer[i] = '\0';
				break;
			}
			return NULL;
		}

		if (c == 3) {
			return NULL;
		} else if (c >= ' ' && i < BUFSIZE - 1) {
			//putc(c);
			buffer[i++] = c;
		} else if (c == '\b' && i > 0) {
			//putc(c);
			i--;
		} else if (c == '\n' || c == '\r') {
			//putc(c);
			buffer[i] = '\0';
			break;
		}
	}
	return buffer;
}

void chat()
{
	cprintf("chat!  type \"quit\" to quit. \n");

	while (1) {
		char *data = readline(NULL);
		if (strcmp(data + 1, "quit") == 0)
			break;
		int len = strlen(data + 1);
		rf212_send(len + 1, data);
	}
}

void reg_write(uint8_t reg, uint8_t value)
{
	rf212_reg(reg, value);
}

void reset()
{
	rf212_reset();
}

uint8_t power_value[] =
    { 0xe1, 0xa1, 0x81, 0x82, 0x83, 0x84, 0x85, 0x42, 0x22, 0x23, 0x24, 0x25,
	0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d
};

void set_power(int8_t power)
{
	if (power > 10 || power < -11) {
		cprintf("error : tx_power_dbm must in [-11,10].\n");
		return;
	}
	uint8_t value = power_value[10 - power];
	reg_write(0x05, value);
}

void set_mode(int mode)
{
	uint8_t value;
	switch (mode) {
	case 20:
		value = 0x00;
		break;
	case 40:
		value = 0x04;
		break;
	case 100:
		value = 0x08;
		break;
	case 200:
		value = 0x09;
		break;
	case 250:
		value = 0x0C;
		break;
	case 500:
		value = 0x0D;
		break;
	case 1000:
		value = 0x2E;
		break;
	default:
		cprintf
		    ("error : supported mode : 20, 40, 100, 200, 250, 500, 1000 kbps\n");
		return;
		break;
	}
	reg_write(0x0C, value);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return 0;
	}
	if (strcmp(argv[1], "chat") == 0) {
		chat();
	} else if (strcmp(argv[1], "reg") == 0) {
		if (argc < 4) {
			usage();
			return 0;
		}
		uint8_t reg, value;
		reg = strtol(argv[2], NULL, 10);
		value = strtol(argv[3], NULL, 10);
		reg_write(reg, value);
	} else if (strcmp(argv[1], "reset") == 0) {
		reset();
	} else if (strcmp(argv[1], "power") == 0) {
		if (argc < 3) {
			usage();
			return 0;
		}
		int8_t power = strtol(argv[2], NULL, 10);
		set_power(power);
	} else if (strcmp(argv[1], "mode") == 0) {
		if (argc < 3) {
			usage();
			return 0;
		}
		int mode = strtol(argv[2], NULL, 10);
		set_mode(mode);
	}

	return 0;
}
