// Ping-pong a counter between two processes.
// Start two instances of this program as envs 1 and 2
// (user/idle is env 0).

#include "lib.h"

void
umain(void)
{
	u_int who, i;

	if (env == &envs[1]) {
		// get the ball rolling
		ipc_send(envs[2].env_id, 0);
	}

	for (;;) {
		i = ipc_recv(&who);
		printf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;
		i++;
		ipc_send(who, i);
		if (i == 10)
			return;
	}
}
