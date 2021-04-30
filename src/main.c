#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <posix/pthread.h>
#include <data/json.h>

#include "civetweb.h"

#define SLEEP_TIME_MS 500

#define LED0_NODE DT_ALIAS(led0)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

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
    mg_printf(conn, "<h1>DUTCHess</h1>");
    mg_printf(conn, "</body></html>\n");

    return 200;
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

    return 0;
}

void main (void)
{
    const struct device *dev;
    pthread_attr_t attr;
    pthread_t thread;

    if ((dev = device_get_binding(LED0)) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0)
    {
        return;
    }

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr,
                          &stack,
                          CONFIG_MAIN_STACK_SIZE);

    pthread_create(&thread, &attr, server, 0);

    while (true)
    {
        gpio_pin_set(dev, PIN, !gpio_pin_get(dev, PIN));
        k_msleep(SLEEP_TIME_MS);
    }
}
