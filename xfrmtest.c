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


int xfrm_policy_add_simple() {
struct {
   struct nlmsghdr                 n; //netlink message header
   struct xfrm_userpolicy_info     xpinfo; // xfrm message
   char                            buf[ 2048 ]; // field of attributes
}req;

/*int main(int argc, char** argv)
{
	struct rtnl_handle rth;
	kAFL_payload* payload_buffer = mmap((void*)NULL, PAYLOAD_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	memset(payload_buffer, 0xff, PAYLOAD_SIZE);
	kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uint64_t)payload_buffer);
	kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);
	while(1){
			kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);
			kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0); 
			rtnl_open_byproto(&rth, 0, NETLINK_XFRM);
			rtnl_send(&rth,payload_buffer->data, payload_buffer->size);
			rtnl_close(&rth);
			kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
	}
	return 0;
}*/

struct rtnl_handle rth;
memset(&req, 0, sizeof(req));

// nlmsghdr initialization 
req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.xpinfo));
req.n.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL;
req.n.nlmsg_type = XFRM_MSG_NEWPOLICY;

// xfrm_userpolicy_info initialization
req.xpinfo.sel.family = AF_INET;
req.xpinfo.lft.soft_byte_limit = XFRM_INF;
req.xpinfo.lft.hard_byte_limit = XFRM_INF;
req.xpinfo.lft.soft_packet_limit = XFRM_INF;
req.xpinfo.lft.hard_packet_limit = XFRM_INF;
req.xpinfo.dir = XFRM_POLICY_IN; 

if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
   exit(1);

if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
   exit(2);

rtnl_close(&rth);

return 0;
}
int main(int argc, char** argv){

	xfrm_policy_add_simple();
}

