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
    kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uint64_t)payload_buffer);
    kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);


    int payloadfd = open(TEMPPAYLOAD, O_RDWR | O_CREAT | O_SYNC, 0777);
    while(1){
        kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);

        lseek(payloadfd, 0, SEEK_SET);
        write(payloadfd, payload_buffer->data, payload_buffer->size-4);
        int payl = open(TEMPPAYLOAD, O_RDONLY);
        if (read(payl,&p, sizeof(p)) != sizeof(p)){
            close(payl);
            return -1;
        }
        len = read(payl,buffer,sizeof(buffer));
        close(payl);
        if(len <0)
            return -1;

        int sock = socket(AF_NETLINK, p.type, p.protocol);
        if (sock == -1)
            return -1;
        p.msg.msg_name = &p.name;

        struct iovec iov = {
            .iov_base = &buffer[0],
            .iov_len = (size_t) len,
        };
        p.msg.msg_iov = &iov;
        p.msg.msg_iovlen = 1;
        p.msg.msg_control = 0;
        kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0); 
        sendmsg(sock, &p.msg, p.flags);
        close(sock);
        kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
    }


}
