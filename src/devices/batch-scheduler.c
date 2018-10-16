/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "timer.h"
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

struct lock locked_thread;
struct condition conditionToGo[2];
struct condition conditionToGoPrio[2];
int waiters[2];     // the number of cars waiting to go in each direction
int waitersPrio[2];
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


			lock_init(&locked_thread);
			cond_init(&conditionToGo[RECEIVER]);
			cond_init(&conditionToGo[SENDER]);
			cond_init(&conditionToGoPrio[RECEIVER]);
			cond_init(&conditionToGoPrio[SENDER]);

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

    int i;
		for (i=0; i < (int)num_tasks_send;i++){
			//tid_t thread_create (const char *name, int priority, thread func *func, void *aux)
			thread_create ("task_Send_noPrio",NORMAL, *senderTask,0);
		}
		for (i=0; i < (int)num_task_receive;i++){
			thread_create ("task_Receive_noPrio",NORMAL, *receiverTask,0);

		}
		for (i=0; i < (int)num_priority_send;i++){
			thread_create ("task_Send_Prio",HIGH, *senderPriorityTask,0);
		}
		for (i=0; i < (int)num_priority_receive;i++){
			thread_create ("task_Receive_Prio",HIGH, *receiverPriorityTask,0);
		}

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



/* task tries to get slot on the bus subsystem */
void getSlot(task_t task)
{

	int taskDirection;
	int taskPriority;
	taskDirection = task.direction;
	taskPriority = task.priority;
	lock_acquire(&locked_thread);
	while
	(				
					(runningTasks == 3)
					||
					(runningTasks > 0 && currentdirection != taskDirection)
					||
					((taskPriority == 0) && (waitersPrio[SENDER] > 0 || waitersPrio[RECEIVER] > 0) )

	)
			//|| ((waitersPrio[tastDirection] == 0) && (waitersPrio[!tastDirection] > 0))
	{

		if(taskPriority){ //
				waitersPrio[taskDirection]++;
				//void cond_wait (struct condition *cond, struct lock *lock)
				cond_wait(&conditionToGoPrio[taskDirection],&locked_thread);
				waitersPrio[taskDirection]--;
			}else{
				waiters[taskDirection]++;
				//void cond_wait (struct condition *cond, struct lock *lock)
				cond_wait(&conditionToGo[taskDirection],&locked_thread);
				waiters[taskDirection]--;
			}

	}
	// get on the busTasks
	runningTasks++;
	currentdirection = taskDirection;
	lock_release(&locked_thread);
}

/*
ArriveBridge(int direction) {
lock_acquire();
// while can't get on the bridge, wait
while ((cars == 3) || (cars > 0 && currentdirection != direction)) {
waiters[direction]++;
waitingToGo[direction].wait();
waiters[direction]--;
}
// get on the bridge
cars++;
currentdirection = direction;
lock_release();
}
*/


/* task processes data on the bus send/receive */
void transferData(task_t task)
{
    int timetowait = 15;//math.random(1,10);
		timer_sleep(timetowait);
}

/* task releases the slot */
void leaveSlot(task_t task)
{
		int taskDirection;
		int taskPriority;
		taskDirection = task.direction;
		taskPriority = task.priority;
		lock_acquire(&locked_thread);
		// exit bus
		runningTasks--;
		if (waitersPrio[taskDirection] > 0){
			//void cond_signal (struct condition *cond, struct lock *lock)
			cond_signal(&conditionToGoPrio[currentdirection], &locked_thread);
		}
		else if (waitersPrio[!taskDirection] > 0){
			cond_signal(&conditionToGoPrio[!currentdirection], &locked_thread);
		}
		else if (waiters[taskDirection] > 0){
			cond_signal(&conditionToGo[currentdirection], &locked_thread);
		}
		else if (waiters[!taskDirection] > 0){
			cond_signal(&conditionToGo[!currentdirection], &locked_thread);
		}else{
			//void cond_broadcast (struct condition *cond, struct lock *lock)
		}
		lock_release(&locked_thread);
}

/*
ExitBridge() {
lock_acquire();
// get off the bridge
cars--;
// if anybody wants to go the same direction, wake them
if (waiters[currentdirection] > 0)
waitingToGo[currentdirection].signal();
// else if empty, try to wake somebody going the other way
else if (cars == 0)
waitingToGo[1-currentdirection].broadcast();
lock_release();
}
*/
