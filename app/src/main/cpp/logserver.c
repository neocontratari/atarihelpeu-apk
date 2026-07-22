// logserver.c - maly HTTP server na portu 8765:
//   GET  /        ... stranka "Nahrat hru" (pres wifi, bez opravneni)
//   POST /put     ... prijem souboru hry do slozky aplikace
//   POST /putbios ... prijem BIOS souboru
//   GET  /list    ... seznam nahranych souboru
//   GET  /log     ... vypis logu (jako dosud; i jakakoli jina adresa)
// Jedno vlakno, jedno spojeni po druhem. Behem nahravani velkeho
// souboru je /log chvili nedostupny - to je v poradku.

#define _POSIX_C_SOURCE 200809L

#include "logserver.h"

#include <android/log.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TAG       "EGLRender"
#define MAX_LINES 400
#define LINE_LEN  240
#define HDR_CAP   8192

static char            s_lines[MAX_LINES][LINE_LEN];
static int             s_count   = 0;
static int             s_next    = 0;
static pthread_mutex_t s_mtx     = PTHREAD_MUTEX_INITIALIZER;
static int             s_started = 0;
static int             s_port    = 8765;
static char            s_dir[320]      = "";   // slozka pro hry (<interni>/ps1)
static char            s_dir_bios[360] = "";   // slozka pro BIOS

void ls_log(const char* fmt, ...) {
    char msg[LINE_LEN - 16];
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

void logserver_set_upload_dir(const char* internal_dir) {
    if (!internal_dir || !*internal_dir) return;
    snprintf(s_dir, sizeof s_dir, "%s/ps1", internal_dir);
    mkdir(s_dir, 0700);
    snprintf(s_dir_bios, sizeof s_dir_bios, "%s/bios", s_dir);
    mkdir(s_dir_bios, 0700);
}

static void send_all(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, len, 0);
        if (n <= 0) return;
        buf += n;
        len -= (size_t)n;
    }
}

static void send_str(int fd, const char* s) { send_all(fd, s, strlen(s)); }

static void send_head(int fd, const char* status, const char* ctype) {
    char h[200];
    snprintf(h, sizeof h,
             "HTTP/1.1 %s\r\nContent-Type: %s; charset=utf-8\r\n"
             "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
             status, ctype);
    send_str(fd, h);
}

// ------------------------------------------------------------------
// /log - vypis kruhove pameti
// ------------------------------------------------------------------
static void send_logs(int fd) {
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
    send_head(fd, "200 OK", "text/plain");
    if (out) {
        send_all(fd, out, off);
        free(out);
    }
}

// ------------------------------------------------------------------
// / - stranka pro nahrani hry
// ------------------------------------------------------------------
static const char PAGE[] =
"<!doctype html><html><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>AH EGL Render</title></head>"
"<body style='font-family:sans-serif;max-width:640px;margin:24px auto;padding:0 12px'>"
"<h2>Nahr&aacute;t hru do AH EGL Render</h2>"
"<p>Vyber soubory hry (<b>.cue + .bin</b> spolu, nebo jeden <b>.chd</b>) "
"a nahraj je p&#345;es wifi p&#345;&iacute;mo do telefonu. Nen&iacute; pot&#345;eba &#382;&aacute;dn&eacute; opr&aacute;vn&#283;n&iacute;.</p>"
"<p><input id='g' type='file' multiple> "
"<button onclick=\"up('g','/put')\">Nahr&aacute;t hru</button></p>"
"<h3>BIOS (nepovinn&eacute;)</h3>"
"<p><input id='b' type='file' multiple> "
"<button onclick=\"up('b','/putbios')\">Nahr&aacute;t BIOS</button></p>"
"<pre id='o' style='background:#f3f3f3;padding:10px;min-height:60px'></pre>"
"<p><a href='/log'>Zobrazit log</a> | <a href='/list'>Soubory v telefonu</a></p>"
"<script>\n"
"async function up(id,ep){var o=document.getElementById('o');"
"var fs=document.getElementById(id).files;"
"if(!fs.length){o.textContent+='Nejdriv vyber soubor(y).\\n';return;}"
"for(var i=0;i<fs.length;i++){var f=fs[i];"
"o.textContent+='Nahravam '+f.name+' ('+f.size+' bajtu)...\\n';"
"try{var r=await fetch(ep+'?name='+encodeURIComponent(f.name),{method:'POST',body:f});"
"o.textContent+=(await r.text())+'\\n';}"
"catch(e){o.textContent+='CHYBA: '+e+'\\n';}}"
"o.textContent+='Hotovo. Uplne zavri apku AH EGL Render a spust ji znovu.\\n';}\n"
"</script></body></html>";

// ------------------------------------------------------------------
// /list - co uz je v telefonu
// ------------------------------------------------------------------
static void list_one_dir(int fd, const char* dir, const char* label) {
    char line[420];
    snprintf(line, sizeof line, "%s (%s):\n", label, dir[0] ? dir : "-");
    send_str(fd, line);
    if (!dir[0]) return;
    DIR* d = opendir(dir);
    if (!d) { send_str(fd, "  (slozka nejde otevrit)\n"); return; }
    struct dirent* e;
    int n = 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[480];
        snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        struct stat st;
        long long sz = (stat(path, &st) == 0) ? (long long)st.st_size : -1;
        if (sz >= 0 && !S_ISREG(st.st_mode)) continue;
        snprintf(line, sizeof line, "  %s  (%lld bajtu)\n", e->d_name, sz);
        send_str(fd, line);
        n++;
    }
    closedir(d);
    if (!n) send_str(fd, "  (prazdno)\n");
}

// ------------------------------------------------------------------
// POST /put a /putbios - prijem souboru
// ------------------------------------------------------------------
static long long find_content_length(const char* hdr) {
    const char* p = hdr;
    while (p && *p) {
        if (strncasecmp(p, "Content-Length:", 15) == 0) return atoll(p + 15);
        p = strstr(p, "\r\n");
        if (p) p += 2;
    }
    return -1;
}

static void decode_and_sanitize(const char* in, char* out, size_t outsz) {
    char tmp[256];
    size_t j = 0;
    for (size_t i = 0; in[i] && in[i] != ' ' && in[i] != '&' && j < sizeof tmp - 1; i++) {
        char c = in[i];
        if (c == '%' && in[i + 1] && in[i + 2]) {
            char hx[3] = { in[i + 1], in[i + 2], 0 };
            c = (char)strtol(hx, NULL, 16);
            i += 2;
        } else if (c == '+') {
            c = ' ';
        }
        tmp[j++] = c;
    }
    tmp[j] = 0;
    const char* base = strrchr(tmp, '/');
    base = base ? base + 1 : tmp;
    size_t k = 0;
    for (size_t i = 0; base[i] && k < outsz - 1; i++) {
        char c = base[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '.' || c == '_' ||
                 c == '-' || c == ' ' || c == '(' || c == ')';
        out[k++] = ok ? c : '_';
    }
    out[k] = 0;
    if (k == 0) snprintf(out, outsz, "soubor.bin");
}

static void handle_upload(int fd, const char* dir, const char* name_query,
                          const char* buf, int got, int header_end) {
    if (!dir[0]) {
        send_head(fd, "500 Internal Server Error", "text/plain");
        send_str(fd, "CHYBA: uloziste aplikace jeste neni pripravene");
        return;
    }
    long long cl = find_content_length(buf);
    if (cl <= 0) {
        send_head(fd, "400 Bad Request", "text/plain");
        send_str(fd, "CHYBA: chybi Content-Length");
        return;
    }
    char name[128];
    decode_and_sanitize(name_query, name, sizeof name);

    char path[480];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE* f = fopen(path, "wb");
    if (!f) {
        send_head(fd, "500 Internal Server Error", "text/plain");
        send_str(fd, "CHYBA: soubor nejde vytvorit");
        return;
    }

    long long written = 0;
    int wr_err = 0;
    int body0 = got - header_end;
    if (body0 > 0) {
        if (fwrite(buf + header_end, 1, (size_t)body0, f) != (size_t)body0) wr_err = 1;
        written += body0;
    }
    static char rb[65536];
    while (!wr_err && written < cl) {
        ssize_t n = recv(fd, rb, sizeof rb, 0);
        if (n <= 0) break;
        if (fwrite(rb, 1, (size_t)n, f) != (size_t)n) { wr_err = 1; break; }
        written += n;
    }
    fclose(f);

    char msg[260];
    if (!wr_err && written == cl) {
        snprintf(msg, sizeof msg, "OK: %s ulozeno (%lld bajtu)", name, written);
        ls_log("Server: prijat soubor %s (%lld bajtu)", name, written);
        __android_log_print(ANDROID_LOG_INFO, TAG, "%s", msg);
        send_head(fd, "200 OK", "text/plain");
    } else {
        snprintf(msg, sizeof msg, "CHYBA: %s - prijato %lld z %lld bajtu%s",
                 name, written, cl, wr_err ? " (zapis selhal - plna pamet?)" : "");
        ls_log("Server: %s", msg);
        send_head(fd, "500 Internal Server Error", "text/plain");
    }
    send_str(fd, msg);
}

// ------------------------------------------------------------------
// Hlavni smycka serveru
// ------------------------------------------------------------------
static int recv_headers(int fd, char* buf, int cap, int* header_end) {
    int total = 0;
    *header_end = -1;
    while (total < cap - 1) {
        ssize_t n = recv(fd, buf + total, (size_t)(cap - 1 - total), 0);
        if (n <= 0) break;
        total += (int)n;
        buf[total] = 0;
        char* p = strstr(buf, "\r\n\r\n");
        if (p) {
            *header_end = (int)(p - buf) + 4;
            break;
        }
    }
    return total;
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
            "Server: port %d nejde otevrit (nedrzi ho emu10?). Bezi se dal bez nej.",
            s_port);
        close(srv);
        return NULL;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG,
        "Server bezi: http://IP-TELEFONU:%d/ (nahrani hry) a /log (vypis logu)", s_port);

    for (;;) {
        int cli = accept(srv, NULL, NULL);
        if (cli < 0) continue;

        static char buf[HDR_CAP];
        int header_end = -1;
        int got = recv_headers(cli, buf, sizeof buf, &header_end);
        if (got <= 0 || header_end < 0) { close(cli); continue; }

        char method[8] = {0}, path[300] = {0};
        sscanf(buf, "%7s %299s", method, path);

        if (strcmp(method, "POST") == 0 && strncmp(path, "/put", 4) == 0) {
            int bios = (strncmp(path, "/putbios", 8) == 0);
            const char* nm = "";
            const char* q = strchr(path, '?');
            if (q) {
                const char* n = strstr(q, "name=");
                if (n) nm = n + 5;
            }
            handle_upload(cli, bios ? s_dir_bios : s_dir, nm, buf, got, header_end);
        } else if (strcmp(path, "/") == 0 || strncmp(path, "/index", 6) == 0) {
            send_head(cli, "200 OK", "text/html");
            send_str(cli, PAGE);
        } else if (strncmp(path, "/list", 5) == 0) {
            send_head(cli, "200 OK", "text/plain");
            list_one_dir(cli, s_dir, "Hry");
            list_one_dir(cli, s_dir_bios, "BIOS");
        } else {
            send_logs(cli);   // /log i cokoliv jineho -> vypis logu (jako dosud)
        }
        close(cli);
    }
    return NULL;
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
