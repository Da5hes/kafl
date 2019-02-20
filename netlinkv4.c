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

int main(int argc, char **argv) {

    int fd;
    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    //sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_XFRM);
    unsigned int rcvbuf,sndbuf;
    socklen_t optlen = sizeof(rcvbuf);
    rcvbuf = UINT_MAX;
    sndbuf = UINT_MAX;
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf));
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&sndbuf,sizeof(sndbuf));
    bind(fd, (struct sockaddr *) &sa, sizeof(sa));

    struct nlmsghdr *nh;    /* The nlmsghdr with payload to send */
    struct sockaddr_nl sa2;

    kAFL_payload* payload_buffer = mmap((void*)NULL, PAYLOAD_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(payload_buffer, 0xff, PAYLOAD_SIZE);
    int payloadfd[2];
    payloadfd[0] = open(TEMPPAYLOAD, O_RDWR | O_CREAT | O_SYNC, 0777);
    payloadfd[1] = open(TEMPPAYLOAD, O_RDWR | O_CREAT | O_SYNC, 0777);
    kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);
    kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uint64_t)payload_buffer);
    char buffer[8192];
    int len;
    int sequence_number;
    while(1){
        lseek(payloadfd[0], 0, SEEK_SET);
        lseek(payloadfd[1], 0, SEEK_SET);
        kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);
        write(payloadfd[0], payload_buffer->data, payload_buffer->size-4);
      //  read(payloadfd[1],&p, sizeof(p));
        len = read(payloadfd[1],buffer,sizeof(buffer));
        struct iovec iov = { &buffer[0], len };
        struct msghdr msg;
        msg.msg_name = &sa2;
        msg.msg_namelen = sizeof(sa2);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
        memset(&sa2, 0, sizeof(sa2));
        sa2.nl_family = AF_NETLINK;
        nh->nlmsg_pid = 0;
        nh->nlmsg_seq = ++sequence_number;
        /* Request an ack from kernel by setting NLM_F_ACK */
        nh->nlmsg_flags |= NLM_F_ACK;
        kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0);
        sendmsg(fd, &msg, 0);
        int len;
        char buf[8192];     /* 8192 to avoid message truncation on
                                      platforms with page size > 4096 */
        struct iovec iov2 = { buf, sizeof(buf) };
        struct sockaddr_nl sa3;
        struct msghdr msg2;
        struct nlmsghdr *nh;

        msg2.msg_name = &sa3;
        msg2.msg_namelen = sizeof(sa3);
        msg2.msg_iov = &iov2;
        msg2.msg_iovlen = 1;
        msg2.msg_control = NULL;
        msg2.msg_controllen = 0;
        msg2.msg_flags = 0;
        len = recvmsg(fd, &msg2, 0);

        for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
            nh = NLMSG_NEXT (nh, len)) {
                   /* The end of multipart message */
        }
        kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
    };
    close(payloadfd[0]);
    close(payloadfd[1]);
}
