/* chronos/lbesa.c
 *
 * LBESA Single-Core Scheduler Module for ChronOS
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

struct rt_info* sched_lbesa(struct list_head *head, int flags){

	struct rt_info *task, *best = local_task(head->next), *least, *temp;
	/* Just initializing temp to the first node to add in the list later.*/
	temp = best;
	/* Initialize the lists*/
	initialize_lists(best);

	list_for_each_entry(task, head, task_list[LOCAL_LIST]) {
		/* Check if task is aborted, if yes, the return it. Otherwise
		   check if the deadline is blown, if yes, abort the task using 
		   abort_thread(). {Done in handle_task_failure() function.}
		 */
		if(check_task_failure(task, flags))
			return task;

		livd(task, false, flags);
		/* Creating list 1 with the head node as the best value density.*/
		list_add_before(best, task, SCHED_LIST1);

		/* Creating list 2 with all the tasks */
		list_add_before(temp, task, SCHED_LIST2);

		/* Updating best value density by linear search. */
		if(task->local_ivd < best->local_ivd)
			best= task;
	}

	/* Sorting List1 according to value densities.*/
	quicksort(best, SCHED_LIST1, SORT_KEY_LVD, 0);

	/* Sorting List2 according to deadlines.*/
	quicksort(temp, SCHED_LIST2, SORT_KEY_DEADLINE, 0);
	
	/* Since the best value density is at the head, least value density can be 
	   found out by following.
	 */
	least = task_list_entry(best->task_list[SCHED_LIST1].prev, SCHED_LIST1);

	/* Remove the lowest Value Density task while the schedule is infeasible */
	while (1) {

		/* If list is feasible break ut of the loop and just return the  */
		if(list_is_feasible(temp, SCHED_LIST2))
			break;
		/* If the least value density guy is the head, we move the head
			to the next location.
		 */
		if(least == temp)
			temp = task_list_entry(temp->task_list[SCHED_LIST2].next, SCHED_LIST2);
		
		/* Remove the least value density node from list2.*/
		list_remove(least, SCHED_LIST2);
		
		/* The new least value density node is the previous one. */
		least = task_list_entry(least->task_list[SCHED_LIST1].prev, SCHED_LIST1);

		if(least == best)
			break;
	} 

	return temp;
}

struct rt_sched_local lbesa = {
	.base.name = "LBESA",
	.base.id = SCHED_RT_LBESA,
	.flags = 0,
	.schedule = sched_lbesa,
	.base.sort_key = SORT_KEY_DEADLINE,
	.base.list = LIST_HEAD_INIT(lbesa.base.list)
};

static int __init lbesa_init(void)
{
	return add_local_scheduler(&lbesa);
}
module_init(lbesa_init);

static void __exit lbesa_exit(void)
{
	remove_local_scheduler(&lbesa);
}
module_exit(lbesa_exit);

MODULE_DESCRIPTION("LBESA Scheduling Module for ChronOS");
MODULE_AUTHOR("Nahush Gondhalekar <nahushg@vt.edu>");
MODULE_LICENSE("GPL");
