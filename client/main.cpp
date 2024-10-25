#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <cstring>
#include <assert.h>
#include <string.h>

// Global Variables
const size_t k_max_msg = 4096;

// Prototype
static int32_t query(int fd, const char **msg);
static int32_t read_full(int fd, char *buf, size_t n);
static int32_t write_all(int fd, char *buf, size_t n);

int main(int argc, char **argv){

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        perror("connect");
    }

    const char *msg[] = {"Help", "Me", NULL};
    int32_t err = query(fd, msg);
    if (err) perror("query()");

    close(fd);

}

static int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return rv; // error -1 and EOF is 0

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, char* buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) return -1; // error

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t query(int fd, const char **msg){
    uint32_t total_msg_len = 0;
    uint32_t total_byte_len = 0;
    size_t msg_len = 0;
    uint32_t err = 0;
    size_t index = 0;

    // Allocate write buffer space -- Potentially one extra space in the mem alloc??
    char *wbuf = (char *)malloc(sizeof(char));
    if (wbuf == NULL) perror("malloc failed");

    while (true){
        // Check for null termination in array
        if (msg[index] == NULL) break;
        msg_len = strlen(msg[index]);
        if (msg_len > k_max_msg) perror("Msg too long");

        // Reallocate space to the wbuf
        char *new_wbuf = (char *)realloc(wbuf, (msg_len + 4) * sizeof(char));
        if (new_wbuf == NULL) perror("realloc failed");
        wbuf = new_wbuf;

        // Copy message length to the wbuf
        memcpy(&wbuf[total_msg_len+4*index], &msg_len, 4 + total_msg_len + 4*index);

        // Copy message to the wbuf
        memcpy(&wbuf[4+total_msg_len+4*index], msg[index], msg_len);

        // Add up the total_byte_len and total_msg_len
        total_msg_len += (msg_len);
        total_byte_len += (msg_len + 4);
        index++;
    }

    // Write all messages to the fd
    err = write_all(fd, wbuf, total_byte_len);

    free(wbuf);
    return 0;
}
