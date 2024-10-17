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
const size_t max_msg = k_max_msg * 10;
const size_t msg_array_size = 10;
char messages[msg_array_size][k_max_msg];

// Prototypes
static int32_t multiple_request(int connfd);
static int32_t one_request(int connfd);
static void do_something(int connfd);
static int32_t read_full(int fd, char *buf, size_t n);
static int32_t write_all(int fd, char *buf, size_t n);


int main(int argc, char** argv)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind IPv4 address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) perror("bind()");

    rv = listen(fd, SOMAXCONN);
    if (rv) perror("listen()");

    while (true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0)
        {
            continue; // error
        }
        while (true) {
            int32_t err = multiple_request(connfd);
            if (err) perror("one_request()");
            if (err == 0) break;
        }
        close(connfd);
    }

    return 0;
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

static int32_t multiple_request(int connfd){

    // Read as many bytes as possible
    char rbuf[max_msg];
    int32_t err = read_full(connfd, rbuf, max_msg);
    if (err == 0){
        printf("EOF\n");
    }

    // Parse the message
    size_t index = 0;
    uint32_t len = 0;
    uint32_t total_byte_stream = 0;

    while (index < msg_array_size){
        memcpy(&len, &rbuf[total_byte_stream + 4*index], 4);
        if (len > max_msg) {
            break;
        }
        memcpy(messages[index], &rbuf[4+total_byte_stream+4*(index)], len);
        index++;
        total_byte_stream += len;
    }

    // Print out messages
    for(index = 0; index < msg_array_size; index++){
        printf("Msg num: %lu, Client says: %s\n", index+1, messages[index]);
    }

    return 0;

}
static int32_t one_request(int connfd)
{
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    int32_t err = read_full(connfd, rbuf, 4);
    if (err == 0) {
        printf("EOF\n");
    } else {
        perror("read() error");
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_msg){
        printf("Message too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err){
        perror("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "Reply to client";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);

    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}
