/* chronos/hvdf.c
 *
 * HVDF Single-Core Scheduler Module for ChronOS
 *
 * Author(s)
 *	- Nahush Gondhalekar nahushg@vt.edu
 *
 * Copyright (C) 2009-2012 Virginia Tech Real Time Systems Lab
 */

#include <linux/module.h>
#include <linux/chronos_types.h>
#include <linux/chronos_sched.h>
#include <linux/list.h>

struct rt_info* sched_hvdf(struct list_head *head, int flags)
{
	struct rt_info *task, *best = local_task(head->next);

	/*Check for each entry in the list*/
	list_for_each_entry(task, head, task_list[LOCAL_LIST]){

	/* Check if task is aborted, if yes, the return it. Otherwise
	   check if the deadline is blown, if yes, abort the task using 
	   abort_thread(). {Done in handle_task_failure() function.}
	 */
		if(check_task_failure(task, flags))
			return task;

	/* Calculate inverse value density for the current task.*/
		livd(task, false, flags);

	/* Compare the inverse value densities and assign the higher value
	   density task to best.
	 */
		if(task->local_ivd < best->local_ivd)
			best = task ;

	}


	if(flags & SCHED_FLAG_PI)
		best = get_pi_task(best, head, flags);

	return best;
}

struct rt_sched_local hvdf = {
	.base.name = "HVDF",
	.base.id = SCHED_RT_HVDF,
	.flags = 0,
	.schedule = sched_hvdf,
	.base.sort_key = SORT_KEY_DEADLINE,
	.base.list = LIST_HEAD_INIT(hvdf.base.list)
};

static int __init hvdf_init(void)
{
	return add_local_scheduler(&hvdf);
}
module_init(hvdf_init);

static void __exit hvdf_exit(void)
{
	remove_local_scheduler(&hvdf);
}
module_exit(hvdf_exit);

MODULE_DESCRIPTION("HVDF single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Nahush Gondhalekar <nahushg@vt.edu>");
MODULE_LICENSE("GPL");

