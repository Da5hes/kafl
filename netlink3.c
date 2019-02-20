#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../../kafl_user.h"
#include <asm/types.h>
#include "libnetlink.h"
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/xfrm.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEMPPAYLOAD   "/tmp/payload"
struct params {
    /* For socket() */
    int type;
    int protocol;

    /* For sendmsg() */
    struct sockaddr_nl name;
    struct msghdr msg;
    int flags;
} __attribute__((packed));

int main(int argc, char **argv) {

    struct params p;
    char buffer[2048];
    ssize_t len;

    kAFL_payload* payload_buffer = mmap((void*)NULL, PAYLOAD_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(payload_buffer, 0xff, PAYLOAD_SIZE);
    int payloadfd[2];
    payloadfd[0] = open(TEMPPAYLOAD, O_RDWR | O_CREAT | O_SYNC, 0777);
    payloadfd[1] = open(TEMPPAYLOAD, O_RDWR | O_CREAT | O_SYNC, 0777);
    kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);
    kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uint64_t)payload_buffer);
    while(1){
        lseek(payloadfd[0], 0, SEEK_SET);
        lseek(payloadfd[1], 0, SEEK_SET);
        kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);
        write(payloadfd[0], payload_buffer->data, payload_buffer->size-4);

        if (read(payloadfd[1],&p, sizeof(p)) != sizeof(p)){
            close(payloadfd[1]);
            return -1;
        }
        len = read(payloadfd[1],buffer,sizeof(buffer));
        if(len <0){
            close(payloadfd[1]);
            return -1;
        }

        int sock = socket(16, 3, 6);
        if (sock == -1)
            return -1;
        unsigned int rcvbuf,sndbuf;
        socklen_t optlen = sizeof(rcvbuf);
        rcvbuf = UINT_MAX;
        sndbuf = UINT_MAX;
        if (setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf)) < 0)
            return -1;
        if (setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&sndbuf,sizeof(sndbuf)) < 0)
            return -1;


        p.msg.msg_name = &p.name;

        struct iovec iov = {
            .iov_base = &buffer[0],
            .iov_len = (size_t)len,
        };

        p.msg.msg_iov = &iov;
        p.msg.msg_iovlen = 1;
        p.msg.msg_control = 0;
        p.msg.msg_controllen = 0;

        kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0); 
        int g = sendmsg(sock, &p.msg, p.flags);
        if (g<0){
            printf("failed sendmsg");
        }
        close(sock);
        kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
        

    }
    close(payloadfd[0]);
    close(payloadfd[1]);

}
