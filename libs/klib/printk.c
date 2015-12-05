
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <klib/printk.h>

enum {
	EMERG = 0,
	ALERT,
	CRIT,
	ERR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG_LOG,
	STATUS
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
	static char last_status[2048] = "";
	static int status_nr_lines = 0;
	int cnt = 0;
	char buf[2048] = "";
	int fmt_off = 0;
	int buf_pos = 0;
	int i;

	if (format[0] == '<' && format[2] == '>') {
		fmt_off = 3;
		switch (format[1]) {
		case '0' ... '8':
			log_level = format[1] - '0';
			break;
		case 'c':
			cnt = 1;
			log_level = last_log_level;
			break;
		default:
			return 0;
		}
	}
	last_log_level = log_level;
	if (log_level != STATUS && log_level - printk_log_level > 0)
		return 0;
	for (i = 0; i != status_nr_lines; ++i) {
		strcat(buf, "\033[A\033[2K");
	}
	buf_pos = status_nr_lines * 7;
	if (!cnt) {
		switch(log_level) {
		case EMERG:
			strcpy(buf + buf_pos, "EMERGENCY: ");
			break;
		case ALERT:
			strcpy(buf + buf_pos, "ALERT    : ");
			break;
		case CRIT:
			strcpy(buf + buf_pos, "CRITICAL : ");
			break;
		case ERR:
			strcpy(buf + buf_pos, "ERROR    : ");
			break;
		case WARNING:
			strcpy(buf + buf_pos, "WARNING  : ");
			break;
		case NOTICE:
			strcpy(buf + buf_pos, "NOTICE   : ");
			break;
		case INFO:
			strcpy(buf + buf_pos, "INFO     : ");
			break;
		case DEBUG_LOG:
			strcpy(buf + buf_pos, "DEBUG    : ");
			break;
		case STATUS:
			va_start(args, format);
			vsnprintf(last_status, 2048, format + fmt_off, args);
			va_end(args);
			strcpy(buf + buf_pos, last_status);
			fputs(buf, stdout);
			status_nr_lines = 0;
			for (i = 0; last_status[i] != '\0' && i < 2048; ++i)
				if (last_status[i] == '\n')
					++status_nr_lines;
			fflush(stdout);
			return 0;
		default:
			break;
		}
		buf_pos = strlen(buf);
	} else {
		buf[0] = '\0';
		buf_pos = 0;
	}
	va_start(args, format);
	vsnprintf(buf + buf_pos, 2048 - buf_pos, format + fmt_off, args);
	va_end(args);
	if (log_level <= ERR) {
		fputs(buf, stderr);
		fflush(stderr);
	} else {
		fputs(buf, stdout);
		fflush(stdout);
	}
	if (last_status[0]) {
		fputs(last_status, stdout);
		fflush(stdout);
	}
	return 0;
}

