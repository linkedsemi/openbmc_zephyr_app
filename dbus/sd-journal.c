#include "systemd/sd-journal.h"
int sd_journal_print(int priority, const char *format, ...){}
int sd_journal_printv(int priority, const char *format, va_list ap){}
int sd_journal_send(const char *format, ...){}
int sd_journal_sendv(const struct iovec *iov, int n){}
int sd_journal_perror(const char *message){}