#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../kafl_user.h"
#include <asm/types.h>
#include "libnetlink.h"
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#define NETLINK_USER 17
 
#define MAX_PAYLOAD PAYLOAD_SIZE /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;
 
int main(int argc, char **argv) {

    kAFL_payload* payload_buffer = mmap((void*)NULL, PAYLOAD_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(payload_buffer, 0xff, PAYLOAD_SIZE);
    kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uint64_t)payload_buffer);
    kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);

    while(1){

        kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);
        kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0);
        sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_XFRM);
        if(sock_fd<0)
            return -1;
        memset(&src_addr, 0, sizeof(src_addr));
        src_addr.nl_family = AF_NETLINK;
        src_addr.nl_pid = getpid(); /* self pid */
        bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
 
        memset(&dest_addr, 0, sizeof(dest_addr));
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.nl_family = AF_NETLINK;
        dest_addr.nl_pid = 0; /* For Linux Kernel */
        dest_addr.nl_groups = 0; /* unicast */
        nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
        memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
        nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;
        iov.iov_base = payload_buffer->data;
        iov.iov_len = payload_buffer->size;
        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        
        sendmsg(sock_fd,&msg,0);
        close(sock_fd);
        kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
    }
    return 0;
}
