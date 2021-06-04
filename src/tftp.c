#include <logging/log.h>
LOG_MODULE_REGISTER(tftp_server, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <posix/sys/socket.h>
#include <net/net_core.h>

#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>
#include <net/socket.h>

#include "tftp.h"

// uncomment the next line to have a test/xxx handler
#define TEST_HANDLER 1

#if defined(CONFIG_USERSPACE)
#include <app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || defined(CONFIG_NET_TCP2) || \
    defined(CONFIG_COVERAGE)
#define STACK_SIZE 4096
#else
#define STACK_SIZE 2048
#endif

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define RECV_BUFFER_SIZE 1280
#define STATS_TIMER 60 /* How often to print statistics (in seconds) */
struct data {
    const char *proto;

    struct {
        int sock;
        char recv_buffer[RECV_BUFFER_SIZE];
        uint32_t counter;
        atomic_t bytes_received;
        struct k_work_delayable stats_print;
    } udp;
};

struct configs {
    struct data ipv4;
};

unsigned short int tftpServerPort = 69;

static void process_udp4(void);

APP_DMEM struct configs conf = {
    .ipv4 = {
        .proto = "IPv4",
    },
};

K_THREAD_DEFINE(udp4_thread_id, STACK_SIZE,
        process_udp4, NULL, NULL, NULL,
        THREAD_PRIORITY,
        IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

#define TFTP_DATA_SIZE     512
#define TFTP_OPCODE_SIZE   2
#define TFTP_BLOCKNO_SIZE  2
#define TFTP_MAX_PAYLOAD   512
#define TFTP_ACK_SIZE  TFTP_OPCODE_SIZE + TFTP_BLOCKNO_SIZE 
#define TFTP_PACKET_MAX_SIZE  TFTP_OPCODE_SIZE + TFTP_BLOCKNO_SIZE + TFTP_MAX_PAYLOAD

unsigned int localPort = 69;      // local port to listen on
char packetBuffer[TFTP_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[4];
char ErrorBuffer[37];
char filename[132];
int flagR = 0;
int flagW = 0;
unsigned short int no_block = 0;
int sendit = 0;
int timeout = 0;
unsigned char Block[TFTP_MAX_PAYLOAD];
unsigned char buffer[TFTP_MAX_PAYLOAD]; // line of srec bindary data
char mode[32];
tftpHandler *curHandler=NULL;
unsigned int readAddr;
unsigned int writeAddr;

#define MAX_HANDLERS 5
tftpHandler *handlers[MAX_HANDLERS];
unsigned int noHandlers=0;

void ack(unsigned char block0, unsigned char block1) 
{
    ReplyBuffer[0] = 0;
    ReplyBuffer[1] = 4;
    ReplyBuffer[2] = block0;
    ReplyBuffer[3] = block1;
}

void error(int e) 
{
    memset(ErrorBuffer,0,sizeof(ErrorBuffer));
    ErrorBuffer[0] = 0;
    ErrorBuffer[1] = 5;
    ErrorBuffer[2] = 0;
    ErrorBuffer[3] = e;
    char *message = "";
    switch (e) {
        case 0:
            message = "Not defined error";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 1:
            message = "Only Octet mode supported";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 2:
            message = "Access violation";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 3:
            message = "Disk full or allocation exceeded";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 4:
            message = "Failed to verify write (power?)";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 5:
            message = "No I2C device";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 6:
            message = "Could not open file";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
        case 7:
            message = "No such handler";
            for (int i = 0; i < strlen(message); i++){
                ErrorBuffer[i+4] = message[i];
            }
            break;
    }
    ErrorBuffer[36] = 0;
}

bool validblock(int b, int b1, int b2) 
{
    if ( b == ((b1<<8)+b2))
        return true;
    return false;
}
void sendBlock(unsigned short int no_block, unsigned char *packetBuffer, int sock, struct sockaddr *client_addr, socklen_t client_addr_len)
{
    int len = 4 ;
    timeout = 0;
    packetBuffer[0] = 0;
    packetBuffer[1] = 3;
    packetBuffer[2] = no_block >> 8;
    packetBuffer[3] = no_block & 0xff;
    len+=curHandler->read(&readAddr, &packetBuffer[4], TFTP_PACKET_MAX_SIZE-len);
    NET_INFO("sending packet %d", len);
    sendto(sock,packetBuffer,len,0,client_addr, client_addr_len); 

}

void registerHandler(tftpHandler *client)
{
    if ( noHandlers < MAX_HANDLERS)
    {
        NET_INFO("Registered %s as %d", log_strdup(client->id), noHandlers);
        handlers[noHandlers]=client;
        noHandlers++;
    }
    else
    {
        NET_ERR("Cannot register %s\n", client->id);
    }
}
tftpHandler *getHandler(char *filename)
{
    // NB we alter filename in this function to split it up...
    char *ptr=filename;
    int i;
    while ( *ptr != 0 )
    {
        if ( *ptr == '/')
            break; //got it
        ptr++;
    }
    if ( *ptr ==0 )
    {
        NET_INFO("Cannot find handler for %s\n", log_strdup(filename));
        return NULL; // no need to look as there cant be a handler for this
    }

    *ptr=0;
    ptr++; // this is the rest of the filename which we send to the handler...if we find one


    for (i=0;i< noHandlers;i++)
    {
        if (strcmp(handlers[i]->id, filename) == 0 )
        {
            return handlers[i];
        }
    }
    NET_INFO("Cannot find handler for %s\n", log_strdup(filename));
    return NULL ; // we cant do this
}

void tftp_packet(int sock,unsigned char *packetBuffer,int packetSize, struct sockaddr *client_addr, socklen_t client_addr_len) 
{
    if(packetSize)
    {
        // was it a start read?
        if (packetBuffer[1] == 1 ){
            if (flagR == 0) {
                *filename = 0;
                timeout = 0;
            }
            NET_INFO("RRQ Start a read request");
            strcpy(filename,  &packetBuffer[2]);
            NET_INFO("Filename : %s",log_strdup( filename));
            curHandler=getHandler(filename);;
            if ( curHandler == NULL )
            {
                error(7);
                NET_INFO("No Handler detected");

                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len);
                return;
            }

            // ok we have a handler for this type of filename
            no_block = 1;
            packetBuffer[0] = 0;
            packetBuffer[1] = 3;
            packetBuffer[2] = 0;
            packetBuffer[3] = no_block;
            readAddr=0;
            int len=4+curHandler->read(&readAddr, &packetBuffer[4], TFTP_PACKET_MAX_SIZE-4);
            sendto(sock,packetBuffer,len,0,client_addr, client_addr_len); 


            // was it a start Write?
        } else if (packetBuffer[1] == 2 ) {
            NET_INFO("WRQ start a write request");
            if (flagW == 0) {
                timeout = 0;
            }
            strcpy(filename,  &packetBuffer[2]);
            strcpy(mode,  &packetBuffer[2+strlen(filename)+1]);
            NET_INFO("Filename : %s Mode %s", log_strdup(filename),log_strdup(mode));
            curHandler=getHandler(filename);;
            if ( curHandler == NULL )
            {
                error(7);
                NET_INFO("No Handler detected");

                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len);
                return;
            }
            if ( curHandler->open(filename) < 0)
            {
                error(6);
                NET_INFO("Could not open file");
                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len);
                return;
            }


            ack(0,0);
            writeAddr=0;

            if ( strcmp(mode,"octet")==0)
            {
                sendto(sock,ReplyBuffer,TFTP_ACK_SIZE,0,client_addr, client_addr_len); 
                flagW = 1;
                no_block = 1;
            }
            else
            {
                error(1);
                NET_INFO("Only support octet mode");

                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 
            }

            // was it a write data packet
        } else if (packetBuffer[1] ==3 ) {
            timeout = 0;
            if (validblock(no_block, (int)packetBuffer[2], (int)packetBuffer[3])) {
                ack(packetBuffer[2],packetBuffer[3]);
                readAddr=writeAddr;
                curHandler->write(&writeAddr,&packetBuffer[4],packetSize-4);
                // now verify what we wrote (if indeed we did write it)

                curHandler->read(&readAddr,&Block[4],packetSize-4);
                int i;
                for ( i=4;i < packetSize;i++)
                {
                    if ( packetBuffer[i] != Block[i] )
                    {
                        NET_INFO("Failure in verification.Different %d",i);
                        error(4);
                        sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 
                        return;
                    }

                }

                if (packetSize < 516){
                    flagW = 0;
                }
                sendto(sock,ReplyBuffer,TFTP_ACK_SIZE,0,client_addr, client_addr_len); 
                no_block++;
            } else {
                NET_INFO("Wrong block  %d %d %d",no_block,(int)packetBuffer[2],(int)packetBuffer[3]);
                error(0);

                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 

            }


            // was it a read data packet?
        } else if (packetBuffer[1] == 4 ) {
            if (validblock(no_block, (int)packetBuffer[2], (int)packetBuffer[3])) {
                no_block++;
                sendBlock(no_block,packetBuffer,sock,client_addr, client_addr_len);
            } else if ( !validblock(no_block, (int)packetBuffer[2], (int)packetBuffer[3]) ) {
                NET_INFO("resending packet");
                sendBlock(no_block,packetBuffer,sock,client_addr, client_addr_len);
            } else if (timeout == 8000) {
                error(0);
                NET_INFO("timeout packet");

                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 
            } else {
                NET_INFO("error packet");
                error(0);
                sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 
            }


        } else {
            error(2);
            sendto(sock,ErrorBuffer,TFTP_PACKET_MAX_SIZE,0,client_addr, client_addr_len); 
        }
        timeout++;
    }
}



static int start_udp_proto(struct data *data, struct sockaddr *bind_addr,
        socklen_t bind_addrlen)
{
    int ret;

    data->udp.sock = socket(bind_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (data->udp.sock < 0) {
        NET_ERR("Failed to create UDP socket (%s): %d", data->proto,
                errno);
        return -errno;
    }

    ret = bind(data->udp.sock, bind_addr, bind_addrlen);
    if (ret < 0) {
        NET_ERR("Failed to bind UDP socket (%s): %d", data->proto,
                errno);
        ret = -errno;
    }

    return ret;
}

static int process_udp(struct data *data)
{
    int ret = 0;
    int received;
    struct sockaddr client_addr;
    socklen_t client_addr_len;

    NET_INFO("Waiting for UDP packets on port %d (%s)...",
            tftpServerPort, data->proto);

    do {
        client_addr_len = sizeof(client_addr);
        received = recvfrom(data->udp.sock, data->udp.recv_buffer,
                sizeof(data->udp.recv_buffer), 0,
                &client_addr, &client_addr_len);

        NET_INFO("Received UDP packet %d bytes",received);
        if (received < 0) {
            /* Socket error */
            NET_ERR("UDP (%s): Connection error %d", data->proto,
                    errno);
            ret = -errno;
            break;
        } else if (received) {
            atomic_add(&data->udp.bytes_received, received);
            tftp_packet(data->udp.sock, data->udp.recv_buffer,received,&client_addr, client_addr_len);
        }

    } while (true);

    return ret;
}

static void process_udp4(void)
{
    int ret;
    struct sockaddr_in addr4;

    (void)memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(tftpServerPort);

    ret = start_udp_proto(&conf.ipv4, (struct sockaddr *)&addr4,
            sizeof(addr4));
    if (ret < 0) {
        return;
    }

    k_work_reschedule(&conf.ipv4.udp.stats_print, K_SECONDS(STATS_TIMER));

    while (ret == 0) {
        ret = process_udp(&conf.ipv4);
#if 0
        // its unlikely that we got an error, but we must continue
        // so dont check the error code
        if (ret < 0) {
            return;
        }
#endif
    }
}


static void print_stats(struct k_work *work)
{
    struct data *data = CONTAINER_OF(work, struct data, udp.stats_print);
    int total_received = atomic_get(&data->udp.bytes_received);

    if (total_received) {
        if ((total_received / STATS_TIMER) < 1024) {
            LOG_INF("%s UDP: Received %d B/sec", data->proto,
                    total_received / STATS_TIMER);
        } else {
            LOG_INF("%s UDP: Received %d KiB/sec", data->proto,
                    total_received / 1024 / STATS_TIMER);
        }

        atomic_set(&data->udp.bytes_received, 0);
    }

    k_work_reschedule(&data->udp.stats_print, K_SECONDS(STATS_TIMER));
}

#ifdef TEST_HANDLER
unsigned char mybuf[ TFTP_MAX_PAYLOAD];
int testOpen(char *filename)
{
    int i;
    NET_INFO("Test Handler asked for %s\n", log_strdup(filename));
    for (i=0;i<TFTP_MAX_PAYLOAD;i++)
        mybuf[i]=i;
    return 0;
}
int testRead(unsigned int *addr, unsigned char *buf, unsigned int len)
{
    NET_INFO("read %x", *addr);
    if ( *addr >= 0x8000)
        return 0;
    memcpy(buf,mybuf,len);
    *addr+=len;
    return len;
}
int testWrite(unsigned int *addr, unsigned char *buf, unsigned int len)
{
    NET_INFO("write %x", *addr);
    memcpy(mybuf,buf,len);
    *addr+=len;
    return len;
}
tftpHandler testHandler=
{
    .id="test",
    .open=testOpen,
    .read=testRead,
    .write=testWrite,
};
#endif
void dut_start_tftpServer(void)
{
    if (IS_ENABLED(CONFIG_NET_IPV4)) {
#if defined(CONFIG_USERSPACE)
        k_mem_domain_add_thread(&app_domain, udp4_thread_id);
#endif

        k_work_init_delayable(&conf.ipv4.udp.stats_print, print_stats);
        k_thread_name_set(udp4_thread_id, "udp4");
        k_thread_start(udp4_thread_id);
    }
#ifdef TEST_HANDLER
    registerHandler(&testHandler);
#endif
}

void dut_stop_tftpServer(void)
{
    /* Not very graceful way to close a thread, but as we may be blocked
     * in recvfrom call it seems to be necessary
     */
    if (IS_ENABLED(CONFIG_NET_IPV4)) {
        k_thread_abort(udp4_thread_id);
        if (conf.ipv4.udp.sock >= 0) {
            zsock_close(conf.ipv4.udp.sock);
        }
    }
}
