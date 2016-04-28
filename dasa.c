/* chronos/dasa.c
 *
 * DASA Single-Core Scheduler Module for ChronOS
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

int check_in_the_list(struct rt_info *task , struct rt_info *head ,int SCHED_LIST_NUMBER){

	struct rt_info *curr = head;
	do{
		
		if (task == curr){
			return 1;
		}
		curr = task_list_entry(curr->task_list[SCHED_LIST_NUMBER].next, SCHED_LIST_NUMBER);
	} while(curr != head);

	return 0;

 }

struct rt_info* sched_dasa(struct list_head *head, int flags)
{
	struct rt_info *it,*currTask,*nextTask ,*best_ivd, *best_dead = NULL, *best_dead_old;
	struct timespec currDeadline;
	best_ivd = local_task(head->next);

	/*Iterate through the entire list to initialize lists and dependencies.*/

	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		if(check_task_failure(it, flags))
			return it;

		initialize_lists(it);

		//Initialize dependencies
		initialize_dep(it);
	}	


	/* Calculate the LVDs for all the tasks with dependecies..*/

	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		//livd(it, false, flags);

		//Making "true" for calculating LVDs with dependency consideration.
		livd(it, true, flags);
		list_add_before(best_ivd, it, SCHED_LIST1);

		if(it->local_ivd < best_ivd->local_ivd)
			best_ivd = it;
	}

	// Sort the list according to value densities.
	quicksort(best_ivd, SCHED_LIST1, SORT_KEY_LVD, 0);
	it = best_ivd;
	best_dead = it;

	do {

		// Check if the task is not present in the schedule.
		if(!check_in_the_list(it,best_dead,SCHED_LIST2)){

			//Taking the copy of the main scheduling list into tentative list

			copy_list(best_dead, SCHED_LIST2, SCHED_LIST3);
			/* Remember the original head of the list */
			best_dead_old = best_dead;

			/* insert each task in the EDF ordered list */
			if(best_dead) {
				if(insert_on_list(it, best_dead, SCHED_LIST3, SORT_KEY_DEADLINE, 0) == 1)
					best_dead = it;
			} else
				best_dead = it;

			currDeadline = it->deadline;			
			currTask = it ;
			nextTask = it->dep;

			//Iterate through the dependency list 

			while(nextTask != NULL){

				//Check if the task is already present in the tentative schedule.

				if(check_in_the_list(nextTask,best_dead,SCHED_LIST3)){
					
					// Check for the earlier temporary Deadline and continue if lesser than 
					//the current task deadline.
					
					if(earlier_deadline(&(nextTask->temp_deadline), &(currDeadline))) {
						currTask = nextTask;
						nextTask = currTask->dep;
						continue ;
					}

					else{
						
						//Remove the dependency if not less.
						list_remove(nextTask, SCHED_LIST3);
					}

				}

				//Task insertion according to temporary deadline.

				nextTask->temp_deadline = earlier_deadline(&(it->temp_deadline), &(nextTask->temp_deadline)) ? it->temp_deadline : nextTask->temp_deadline;
				if(insert_on_list(nextTask, best_dead, SCHED_LIST3, SORT_KEY_TDEADLINE, 0) == 1)
					best_dead = nextTask;

				currTask = nextTask;
				nextTask = currTask->dep;

			}

			// If the schedule is feasible, copy the tentative schedule to the current schedule.

			if(list_is_feasible(best_dead, SCHED_LIST3)) {			
				copy_list(best_dead, SCHED_LIST3, SCHED_LIST2);
				//list_remove(it, SCHED_LIST2);
				//best_dead = best_dead_old;
			}
			else{
				//list_remove(nextTask, SCHED_LIST3);
				best_dead = best_dead_old;
			}

		}

		//Iterate over all the tasks.
		
		it = task_list_entry(it->task_list[SCHED_LIST1].next, SCHED_LIST1);

	} while(it != best_ivd);

	return best_dead ? best_dead : best_ivd;
}

struct rt_sched_local dasa = {
	.base.name = "DASA",
	.base.id = SCHED_RT_DASA,	
	.flags = 0,
	.schedule = sched_dasa,
	.base.sort_key = SORT_KEY_TDEADLINE,
	.base.list = LIST_HEAD_INIT(dasa.base.list)
};

static int __init dasa_init(void)
{
	return add_local_scheduler(&dasa);
}
module_init(dasa_init);

static void __exit dasa_exit(void)
{
	remove_local_scheduler(&dasa);
}
module_exit(dasa_exit);

MODULE_DESCRIPTION("DASA single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Nahush Gondhalekar <nahushg@vt.edu>");
MODULE_LICENSE("GPL");
