#include <posix/pthread.h>
#include <data/json.h>

#include "civetweb.h"

#include "relay.h"
#include "web.h"

#define HTTP_PORT 80

#define MAX_REQUEST_SIZE_BYTES 1024
#define HTTP_OK "HTTP/1.1 200 OK\r\n" \
                "Content-Type: text/html\r\n" \
                "Connection: close\r\n\r\n"

K_THREAD_STACK_DEFINE(stack, CONFIG_MAIN_STACK_SIZE);

static int home (struct mg_connection *conn, void *cbdata)
{
    mg_printf(conn, HTTP_OK);
    mg_printf(conn, "<html><body>\n");
    mg_printf(conn, "<h1>DUTCHess</h1>\n");
    mg_printf(conn, "Relay: %s\n", dutchess_relay_state() ? "ON":"OFF");
    mg_printf(conn, "</body></html>\n");

    return 200;
}

static int turn_relay_on (struct mg_connection *conn, void *cbdata)
{
    dutchess_relay_set(1);

    return home(conn,cbdata);
}

static int turn_relay_off (struct mg_connection *conn, void *cbdata)
{
    dutchess_relay_set(0);

    return home(conn,cbdata);
}

static void *server (void *arg)
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
    mg_set_request_handler(ctx, "/on$", turn_relay_on, 0);
    mg_set_request_handler(ctx, "/off$", turn_relay_off, 0);

    return 0;
}

void dutchess_web_server_init ()
{
    pthread_attr_t attr;
    pthread_t thread;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr,
                          &stack,
                          CONFIG_MAIN_STACK_SIZE);

    pthread_create(&thread, &attr, server, 0);
}
