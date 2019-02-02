//
//  semaphore_group.h
//  libdispatch
//
//  Created by 王举范 on 2018/11/27.
//

#ifndef semaphore_group_h
#define semaphore_group_h

#include <stdio.h>

/*
 * If we are not in the kernel, then these will all be represented by
 * ports at user-space.
mach_port_t  unsigned int
 */
typedef mach_port_t		task_t;
typedef mach_port_t		task_name_t;
typedef mach_port_t		task_inspect_t;
typedef mach_port_t		task_suspension_token_t;
typedef mach_port_t		thread_t;
typedef	mach_port_t		thread_act_t;
typedef mach_port_t		thread_inspect_t;
typedef mach_port_t		ipc_space_t;
typedef mach_port_t		ipc_space_inspect_t;
typedef mach_port_t		coalition_t;
typedef mach_port_t		host_t;
typedef mach_port_t		host_priv_t;
typedef mach_port_t		host_security_t;
typedef mach_port_t		processor_t;
typedef mach_port_t		processor_set_t;
typedef mach_port_t		processor_set_control_t;
typedef mach_port_t		semaphore_t;
typedef mach_port_t		lock_set_t;
typedef mach_port_t		ledger_t;
typedef mach_port_t		alarm_t;
typedef mach_port_t		clock_serv_t;
typedef mach_port_t		clock_ctrl_t;

typedef struct semaphore {
	queue_chain_t	  task_link;  /* chain of semaphores owned by a task */
	struct waitq	  waitq;      /* queue of blocked threads & lock     */
	task_t		  owner;      /* task that owns semaphore            */
	ipc_port_t	  port;	      /* semaphore port	 		     */
	uint32_t	  ref_count;  /* reference count		     */
	int		  count;      /* current count value	             */
	boolean_t	  active;     /* active status			     */
} Semaphore;
semaphore_t the address of Semaphore

#endif /* semaphore_group_h */
