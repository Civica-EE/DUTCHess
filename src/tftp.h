#ifndef TFTP_H
#define TFTP_H

void dut_start_tftpServer(void);

typedef int (*openDataType)(char *filename);
typedef int (*readDataType)(unsigned int *addr, unsigned char *buf, unsigned int len);
typedef int (*writeDataType)(unsigned int *addr, unsigned char *buf, unsigned int len);
typedef struct
{
	char *id; // first part of the filename up to a "/"
	openDataType open;
	readDataType read;
	writeDataType write;
} tftpHandler;
#endif
