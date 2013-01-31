
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <klib/compiler.h>
#include <klib/printk.h>

enum {
	EMERG = 0,
	ALERT,
	CRIT,
	ERR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG_LOG
};

static int printk_log_level = 3;

void printk_set_log_level(int log_level)
{
	printk_log_level = log_level;
}

int printk(const char *format, ...)
{
	va_list args;
	int log_level = INFO; /* info output */
	static int last_log_level = DEBUG_LOG;
	int cnt = 0;
	char buf[2048];
	if (likely(format[0] == '<' && format[2] == '>')) {
		int buf_pos = 0;
		switch (format[1]) {
		case '0' ... '7':
			log_level = format[1] - '0';
			break;
		case 'c':
			cnt = 1;
			log_level = last_log_level;
			break;
		default:
			return 0;
		}
		last_log_level = log_level;
		if (likely(log_level - printk_log_level > 0))
			return 0;
		if (!cnt) {
			switch(log_level) {
			case EMERG:
				strcpy(buf, "EMERGENCY: ");
				break;
			case ALERT:
				strcpy(buf, "ALERT    : ");
				break;
			case CRIT:
				strcpy(buf, "CRITICAL : ");
				break;
			case ERR:
				strcpy(buf, "ERROR    : ");
				break;
			case WARNING:
				strcpy(buf, "WARNING  : ");
				break;
			case NOTICE:
				strcpy(buf, "NOTICE   : ");
				break;
			case INFO:
				strcpy(buf, "INFO     : ");
				break;
			case DEBUG_LOG:
				strcpy(buf, "DEBUG    : ");
				break;
			default:
				break;
			}
			buf_pos = strlen(buf);
		} else {
			buf[0] = '\0';
			buf_pos = 0;
		}
		va_start(args, format);
		vsnprintf(buf + buf_pos, 2048 - buf_pos, format + 3, args);
		va_end(args);
		if (log_level <= ERR) {
			fputs(buf, stderr);
			fflush(stderr);
		} else {
			fputs(buf, stdout);
			fflush(stdout);
		}
	}
	return 0;
}

