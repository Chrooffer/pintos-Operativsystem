/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1


/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

struct lock *lock_thread;
condition * conditionToGo[2];
condition * conditionToGoPrio[2];
int * waiters[2];     // he number of cars waiting to go in each direction
int * waitersPrio[2];
int runningTasks;
int currentdirection;


void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);



void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */



/* initializes semaphores */
void init_bus(void){
//void lock_init (struct lock *lock)
//void cond_init (struct condition *cond)
			lock_init(&lock_thread);
			cond_init(conditToGo[RECEIVER]);
			cond_init(conditToGo[SENDER]);
			cond_init(conditToGoPrio[RECEIVER]);
			cond_init(conditToGoPrio[SENDER]);

    random_init((unsigned int)123456789);

}
/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread.
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}




/* task processes data on the bus send/receive */
void transferData(task_t task)
{
    //msg("NOT IMPLEMENTED");
    /* FIXME implement */
		int timetowait = 10;//math.random(1,10);
		timer_sleep(timetowait);

}

/* task releases the slot */
void leaveSlot(task_t task)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */
}



/* task tries to get slot on the bus subsystem */
void getSlot(task_t task)
{
	lock.acquire(lock_thread);
	while ((runningTasks == 3) || (runningTasks > 0 && currentdirection != task->direction)){

		if(task->priority){ //Detta ger komplieringsfel, kan inte bara göra "->"
				waitersPrio[direction]++;
				cond_wait(conditionToGoPrio[direction],lock_thread);
				waitersPrio[direction]--;
			}else{
				waiters[direction]++;
				cond_wait(conditionToGo[direction],lock_thread);
				waiters[direction]--;
			}

	}
	// get on the busTasks
	runningTasks++;
	currentdirection = direction;
	lock.release();
}

/*
ArriveBridge(int direction) {
lock.acquire();
// while can't get on the bridge, wait
while ((cars == 3) || (cars > 0 && currentdirection != direction)) {
waiters[direction]++;
waitingToGo[direction].wait();
waiters[direction]--;
}
// get on the bridge
cars++;
currentdirection = direction;
lock.release();
}
*/


/*
ExitBridge() {
lock.acquire();
// get off the bridge
cars--;
// if anybody wants to go the same direction, wake them
if (waiters[currentdirection] > 0)
waitingToGo[currentdirection].signal();
// else if empty, try to wake somebody going the other way
else if (cars == 0)
waitingToGo[1-currentdirection].broadcast();
lock.release();
}
*/
