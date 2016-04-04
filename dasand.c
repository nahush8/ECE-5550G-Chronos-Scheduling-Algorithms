/* chronos/dasand.c
 *
 * DASA_ND Single-Core Scheduler Module for ChronOS
 *
 * Author(s)
 *	- Nahush Gondhalekar nahushg@vt.edu
 *
 * Copyright (C) 2009-2012 Virginia Tech Real Time Systems Lab
 */

#include <linux/module.h>
#include <linux/chronos_types.h>
#include <linux/chronos_sched.h>
#include <linux/chronos_util.h>
#include <linux/list.h>

struct rt_info* sched_dasand(struct list_head *head, int flags)
{
	struct rt_info *task, *temp = NULL ,*save_temp, *best = local_task(head->next);

	/*Check for each entry in the list*/
	list_for_each_entry(task, head, task_list[LOCAL_LIST]){

		/* Check if task is aborted, if yes, the return it. Otherwise
		   check if the deadline is blown, if yes, abort the task using 
		   abort_thread(). {Done in handle_task_failure() function.}
		 */
		if(check_task_failure(task, flags))
				return task;

		/* Initialize the lists*/
		initialize_lists(task);

		/* Calculate inverse value density for the current task.*/
		livd(task, false, flags);
		/* Add tasks to SCHED_LIST1 according to value densities*/

		list_add_before(best, task, SCHED_LIST1);

		/* Compare the inverse value densities and assign the higher value
		   density task to best.
		 */
		if(task->local_ivd < best->local_ivd)
				best = task ;

	}

	quicksort(best, SCHED_LIST1, SORT_KEY_LVD, 0);
	task = best ;

	while(1) {

		save_temp = temp;

		if(temp == NULL)
			temp = task;

		else if(insert_on_list(task, temp, SCHED_LIST2, SORT_KEY_DEADLINE, 0))
			temp = task;
		

		if(!list_is_feasible(temp, SCHED_LIST2)) {
			list_remove(task, SCHED_LIST2);
			temp = save_temp;
				
		}

		task = task_list_entry(task->task_list[SCHED_LIST1].next, SCHED_LIST1);
		if (task == best)
			break;

	}

	if(temp)
		return temp;
	else
		return best;
}

struct rt_sched_local dasand = {
	.base.name = "DASAND",
	.base.id = SCHED_RT_DASAND,	
	.flags = 0,
	.schedule = sched_dasand,
	.base.sort_key = SORT_KEY_DEADLINE,
	.base.list = LIST_HEAD_INIT(dasand.base.list)
};

static int __init dasand_init(void)
{
	return add_local_scheduler(&dasand);
}
module_init(dasand_init);

static void __exit dasand_exit(void)
{
	remove_local_scheduler(&dasand);
}
module_exit(dasand_exit);

MODULE_DESCRIPTION("DASAND single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Nahush Gondhalekar <nahushg@vt.edu>");
MODULE_LICENSE("GPL");