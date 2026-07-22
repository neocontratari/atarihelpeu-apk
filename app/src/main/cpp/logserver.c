// logserver.c - malinky HTTP server, ktery na portu 8765 vypisuje logy.
// Zamerne co nejjednodussi: jedno vlakno, jedno spojeni po druhem,
// odpoved je vzdy cely obsah kruhove pameti jako text/plain.

#define _POSIX_C_SOURCE 200809L  // kvuli localtime_r pri prisnem rezimu prekladace

#include "logserver.h"

#include <android/log.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TAG       "EGLRender"
#define MAX_LINES 400
#define LINE_LEN  240

static char            s_lines[MAX_LINES][LINE_LEN];
static int             s_count   = 0;   // kolik radku uz je ulozeno (max MAX_LINES)
static int             s_next    = 0;   // kam se zapise dalsi radek
static pthread_mutex_t s_mtx     = PTHREAD_MUTEX_INITIALIZER;
static int             s_started = 0;
static int             s_port    = 8765;

void ls_log(const char* fmt, ...) {
    char msg[LINE_LEN - 16];  // rezerva na casove razitko na zacatku radku
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    pthread_mutex_lock(&s_mtx);
    snprintf(s_lines[s_next], LINE_LEN, "%02d:%02d:%02d  %s",
             tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
    s_next = (s_next + 1) % MAX_LINES;
    if (s_count < MAX_LINES) s_count++;
    pthread_mutex_unlock(&s_mtx);
}

static void send_all(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, len, 0);
        if (n <= 0) return;
        buf += n;
        len -= (size_t)n;
    }
}

static void* server_thread(void* arg) {
    (void)arg;

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) return NULL;

    int one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((uint16_t)s_port);

    if (bind(srv, (struct sockaddr*)&addr, sizeof addr) < 0 || listen(srv, 4) < 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG,
            "Log server: port %d nejde otevrit (nedrzi ho emu10?). Render jede dal bez nej.",
            s_port);
        close(srv);
        return NULL;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG,
        "Log server bezi: http://IP-TELEFONU:%d/log", s_port);

    for (;;) {
        int cli = accept(srv, NULL, NULL);
        if (cli < 0) continue;

        char req[512];
        recv(cli, req, sizeof req, 0);  // pozadavek nas nezajima, kazda cesta vraci logy

        // Snapshot logu pod zamkem, odeslani az po odemceni,
        // aby pomaly klient nebrzdil vykreslovani.
        char*  out = malloc((size_t)MAX_LINES * (LINE_LEN + 1) + 64);
        size_t off = 0;
        if (out) {
            pthread_mutex_lock(&s_mtx);
            int start = (s_count == MAX_LINES) ? s_next : 0;
            for (int i = 0; i < s_count; i++) {
                const char* line = s_lines[(start + i) % MAX_LINES];
                size_t l = strlen(line);
                memcpy(out + off, line, l);
                off += l;
                out[off++] = '\n';
            }
            pthread_mutex_unlock(&s_mtx);
        }

        const char* hdr =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n"
            "\r\n";
        send_all(cli, hdr, strlen(hdr));
        if (out) {
            send_all(cli, out, off);
            free(out);
        }
        close(cli);
    }
    return NULL;  // sem se nikdy nedojde
}

void logserver_start(int port) {
    if (s_started) return;
    s_started = 1;
    s_port    = port;

    pthread_t th;
    if (pthread_create(&th, NULL, server_thread, NULL) == 0) {
        pthread_detach(th);
    }
}
