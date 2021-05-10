#include <posix/pthread.h>
#include <data/json.h>

#include "civetweb.h"
#include "DUTCHess.h"

#define HTTP_PORT 80

#define MAX_REQUEST_SIZE_BYTES 1024
#define HTTP_OK "HTTP/1.1 200 OK\r\n" \
                "Content-Type: text/html\r\n" \
                "Connection: close\r\n\r\n"

K_THREAD_STACK_DEFINE(stack, CONFIG_MAIN_STACK_SIZE);

int home (struct mg_connection *conn, void *cbdata)
{
    mg_printf(conn, HTTP_OK);
    mg_printf(conn, "<html><body>");
    mg_printf(conn, "<h1>DUTCHess " VERSION " Relay %s </h1>", relaystate()?"ON":"OFF");

    mg_printf(conn, "</body></html>\n");

    return 200;
}

int turnRelayOn (struct mg_connection *conn, void *cbdata)
{
	relayon();
	return	home(conn,cbdata);
}

int turnRelayOff (struct mg_connection *conn, void *cbdata)
{
	relayoff();
	return	home(conn,cbdata);
}
int reconfigure (struct mg_connection *conn, void *cbdata)
{
	storeData_t *data=getStoreData();
	// generate the form for reconfiguring the settings
	mg_printf(conn, HTTP_OK);
	mg_printf(conn, "<html><body>");
	mg_printf(conn, "<h1>DUTCHess " VERSION " Relay %s </h1>", relaystate()?"ON":"OFF");

	mg_printf(conn, "<form action=\"config\" method=\"GET\">"
                        "  ip:<br>"
                        "  <input type=\"text\" name=\"ip\" value=\"%ld.%ld.%ld.%ld\"> 0.0.0.0 = DHCP<br><br>"
                        "  gateway:<br>"
                        "  <input type=\"text\" name=\"gateway\" value=\"%ld.%ld.%ld.%ld\"><br><br>"
                        "  mask:<br>"
                        "  <input type=\"text\" name=\"mask\" value=\"%ld.%ld.%ld.%ld\"><br><br>"
                        "  Relay Default state (0 or 1):<br>"
                        "  <input type=\"text\" name=\"relaydefault\" value=\"%d\"><br><br>"                      
                        "  Controller IP<br>"
                        "  <input type=\"text\" name=\"controller\" value=\"%ld.%ld.%ld.%ld\"><br><br>"                       
                        "  Controller port:<br>"
                        "  <input type=\"text\" name=\"port\" value=\"%d\"><br><br>"                       
                        "  <input type=\"submit\" value=\"Submit\">"
                        "</form> ", printIP(data->ip), printIP(data->gateway), printIP(data->netmask),
					data->defaultRelayState?1:0,  printIP(data->controllerIP),
					data->controllerPort);

    	mg_printf(conn, "</body></html>\n");

	return	200;
}
int configure (struct mg_connection *conn, void *cbdata)
{
	// this is called when the configuration form is submitted
	const struct mg_request_info *req_info = mg_get_request_info(conn);
	char data[32];
	int queryLen=strlen(req_info->query_string+1) ; // includes the NULL

	store("ip",data,mg_get_var(req_info->query_string,queryLen,"ip",data,32)); // get the ip as a string
	store("gateway",data,mg_get_var(req_info->query_string,queryLen,"gateway",data,32)); 
	store("mask",data,mg_get_var(req_info->query_string,queryLen,"mask",data,32)); 
	store("relaydefault",data,mg_get_var(req_info->query_string,queryLen,"relaydefault",data,32)); 
	store("controller",data,mg_get_var(req_info->query_string,queryLen,"controller",data,32)); 
	store("port",data,mg_get_var(req_info->query_string,queryLen,"port",data,32)); 

	return	home(conn,cbdata);
}


void *server (void *arg)
{
    static const char * const options[] = {
        "listening_ports",
        STRINGIFY(HTTP_PORT),
        "num_threads",
        "1",
        "max_request_size",
        STRINGIFY(MAX_REQUEST_SIZE_BYTES),
        NULL
    };
    struct mg_callbacks callbacks;
    struct mg_context *ctx;

    memset(&callbacks, 0, sizeof(callbacks));

    if ((ctx = mg_start(&callbacks, 0, (const char **)options)) == NULL)
    {
        printf("Unable to start the server.");
        return 0;
    }

    mg_set_request_handler(ctx, "/$", home, 0);
    mg_set_request_handler(ctx, "/on$", turnRelayOn, 0);
    mg_set_request_handler(ctx, "/off$", turnRelayOff, 0);
    mg_set_request_handler(ctx, "/reconfigure$", reconfigure, 0);
    mg_set_request_handler(ctx, "/config$", configure, 0);

    return 0;
}

void webStart ()
{
    pthread_attr_t attr;
    pthread_t thread;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr,
                          &stack,
                          CONFIG_MAIN_STACK_SIZE);

    pthread_create(&thread, &attr, server, 0);
}
