#define _GNU_SOURCE
#include <stdlib.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
//	return syslog(8, NULL, atoi(argv[1]));
	//return klogctl(8, NULL, atoi(argv[1]));
	return syscall(SYS_syslog, 8, NULL, atoi(argv[1]));
}
