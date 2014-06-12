// Demonstrate lack of fairness in IPC.
// Start three instances of this program as envs 1, 2, and 3.
// (user/idle is env 0).

#include "lib.h"

void
umain(void)
{
	u_int who, id;

	id = sys_getenvid();

	if (env == &envs[1]) {
		for (;;) {
			ipc_recv(&who);
			printf("%x recv from %x\n", id, who);
		}
	} else {
		printf("%x loop sending to %x\n", id, envs[1].env_id);
		for (;;)
			ipc_send(envs[1].env_id, 0);
	}
}

