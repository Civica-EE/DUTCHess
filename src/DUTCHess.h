#ifndef DUTCHESS_H
#define DUTCHESS_H

#include <logging/log.h>

#define DUT_VERSION "1.0"

#define printIP(a) a&0xff, (a>>8)&0xff, (a>>16)&0xff, (a>>24)

#define IP(a,b,c,d) ( (d<<24) | (c<<16) | (b<<8) | a )

bool relaystate(void);
void relayon(void);
void relayoff(void);
double getTemperature(void);

typedef struct
{
	unsigned long ip;
	unsigned long gateway;
	unsigned long netmask;
	bool defaultRelayState;
	unsigned long controllerIP;
	unsigned int controllerPort;
} storeData_t;

storeData_t *getStoreData(void);
void updateStore(char *var, char *value, int valueLen);
void saveStore(void);

#define SAY(fmt, args...) printk("%s:%d: " fmt "\n", __FILE__, __LINE__, ##args)
#define _SAY(fmt, args...) 
#endif

