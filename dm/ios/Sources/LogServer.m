// Log server: serves Documents/log.txt over HTTP on port 8765 so the log can be read live from a computer
// (USB: `iproxy 8765 8765`, then `curl localhost:8765/log`; Wi-Fi: the phone's address, found by Bonjour "_nxlog._tcp"). Read-only, log files only.
//   GET /log[?from=N]   the current log from byte N (default 0)     GET /prev   the previous run's log     GET /ping   "ok" and the current log size
#import <Foundation/Foundation.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static char g_log[1024], g_prev[1024];

static void send_all(int fd, const char *p, size_t n) { while (n) { ssize_t w = send(fd, p, n, 0); if (w <= 0) return; p += w; n -= (size_t)w; } }

static void serve_file(int fd, const char *path, long from) {
    FILE *f = fopen(path, "rb");
    if (!f) { const char *m = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n"; send_all(fd, m, strlen(m)); return; }
    fseek(f, 0, SEEK_END); long size = ftell(f);
    if (from < 0 || from > size) from = 0;
    char head[160]; int hn = snprintf(head, sizeof head, "HTTP/1.0 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nX-Log-Size: %ld\r\nContent-Length: %ld\r\n\r\n", size, size - from);
    send_all(fd, head, (size_t)hn);
    fseek(f, from, SEEK_SET);
    char buf[16384]; size_t n; long left = size - from;
    while (left > 0 && (n = fread(buf, 1, left < (long)sizeof buf ? (size_t)left : sizeof buf, f)) > 0) { send_all(fd, buf, n); left -= (long)n; }
    fclose(f);
}

static void *log_server_th(void *arg) {
    (void)arg;
    int s = socket(AF_INET, SOCK_STREAM, 0); if (s < 0) return NULL;
    int one = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in a = {.sin_len = sizeof a, .sin_family = AF_INET, .sin_port = htons(8765), .sin_addr.s_addr = htonl(INADDR_ANY)};
    if (bind(s, (struct sockaddr *)&a, sizeof a) || listen(s, 4)) { fprintf(stderr, "log server: cannot listen on port 8765\n"); close(s); return NULL; }
    fprintf(stderr, "log server: listening on port 8765 (iproxy 8765 8765, then GET /log)\n");
    for (;;) {
        int c = accept(s, NULL, NULL); if (c < 0) continue;
        struct timeval tv = {.tv_sec = 5}; setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        char req[512]; ssize_t n = recv(c, req, sizeof req - 1, 0);
        if (n > 0) {
            req[n] = 0;
            if (!strncmp(req, "GET /log", 8)) { const char *q = strstr(req, "from="); serve_file(c, g_log, q && q < strchr(req, '\n') ? atol(q + 5) : 0); }
            else if (!strncmp(req, "GET /prev", 9)) serve_file(c, g_prev, 0);
            else { char m[96]; int k = snprintf(m, sizeof m, "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n\r\nok"); send_all(c, m, (size_t)k); }
        }
        close(c);
    }
}

// Called by main.m once stderr is the log file. The Bonjour entry is what makes iOS ask for the Local Network permission once, for the Wi-Fi route
void dm_log_server_start(const char *log, const char *prev) {
    strlcpy(g_log, log, sizeof g_log); strlcpy(g_prev, prev, sizeof g_prev);
    pthread_t t; if (!pthread_create(&t, NULL, log_server_th, NULL)) pthread_detach(t);
    dispatch_async(dispatch_get_main_queue(), ^{
        static NSNetService *svc; svc = [[NSNetService alloc] initWithDomain:@"" type:@"_nxlog._tcp." name:@"" port:8765];
        [svc publish];
    });
}
