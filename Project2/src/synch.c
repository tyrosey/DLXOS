//
//	synch.c
//
//	Routines for synchronization
//
//

#include "dlxos.h"
#include "process.h"
#include "synch.h"
#include "lab2-api.h"

static Sem sems[MAX_SEMS]; 	//All semaphores in the system
static Lock locks[MAX_LOCKS];   // All locks in the system

extern struct PCB *currentPCB; 
//----------------------------------------------------------------------
//	SynchModuleInit
//
//	Initializes the synchronization primitives: the semaphores
//----------------------------------------------------------------------
void 
SynchModuleInit()
{
    int           i;
    dbprintf ('p', "Entering SynchModuleInit\n");
    for(i=0; i<MAX_SEMS; i++)
    {
      sems[i].inuse = 0;
    }
    for(i=0; i<MAX_LOCKS; i++)
    {
	// Your stuff for initializing locks goes here
    }
    for(i=0; i<MAX_CONDS; i++)
    {
	// ignore this
    }
    dbprintf ('p', "Leaving SynchModuleInit\n");
}
//---------------------------------------------------------------------
//
//	SemInit
//
//	Initialize a semaphore to a particular value.  This just means
//	initting the process queue and setting the counter.
//
//----------------------------------------------------------------------
void
SemInit (Sem *sem, int count)
{
    QueueInit (&sem->waiting);
    sem->count = count;
}

//----------------------------------------------------------------------
// 	SemCreate
//
//	Grabs a Semaphore, initializes it and returns a handle to this
//	semaphore. All subsequent accesses to this semaphore should be made
//	through this handle
//----------------------------------------------------------------------
sem_t 
SemCreate(int count)
{
    sem_t sem;
    uint32 intrval;

    // grabbing a semaphore should be an atomic operation
    intrval = DisableIntrs();
    for(sem=0; sem<MAX_SEMS; sem++)
    {
      if(sems[sem].inuse==0)
      {
        sems[sem].inuse = 1;
	break;
      }
    }
    RestoreIntrs(intrval);
    
    if(sem==MAX_SEMS)
      return INVALID_SEM;

    SemInit(&sems[sem], count);

    return sem;
}


//----------------------------------------------------------------------
//
//	SemWait
//
//	Wait on a semaphore.  As described in Section 6.4 of _OSC_,
//      OR section 2.3 of Modern Operating Systems,
//	we decrement the counter and suspend the process if the
//	semaphore's value is less than 0.  To ensure atomicity,
//	interrupts are disabled for the entire operation.  Note that,
//	if the process is put to sleep, interrupts will be OFF when
//	it returns from sleep.  Thus, we enable interrupts at the end of
//	the routine.
//
//----------------------------------------------------------------------
void
SemWait (Sem *sem)
{
    Link	*l;
    int		intrval;

    intrval = DisableIntrs ();
    dbprintf ('I', "Old interrupt value was 0x%x.\n", intrval);
    dbprintf ('s', "Proc 0x%x waiting on sem 0x%x, count=%d.\n", currentPCB,
	      sem, sem->count);
    sem->count -= 1;
    if (sem->count < 0) {
	l = QueueAllocLink ();
	QueueLinkInit (l, (void *)currentPCB);
	dbprintf ('s', "Suspending current proc (0x%x).\n", currentPCB);
	QueueInsertLast (&sem->waiting, l);
	ProcessSleep ();
    }
    RestoreIntrs (intrval);
}
int SemHandleWait(sem_t sem)
{
  if(sem>=0&&sem<MAX_SEMS)
  {
    if(sems[sem].inuse)
    {
      SemWait(&sems[sem]);
      return 0;
    }
    return 1;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------
//
//	SemSignal
//
//	Signal on a semaphore.  Again, details are in Section 6.4 of
//	_OSC_ OR section 2.3 of Modern Operating Systems.
//
//----------------------------------------------------------------------
void
SemSignal (Sem *sem)
{
    Link	*l;
    int		intrs;

    intrs = DisableIntrs ();
    dbprintf ('s', "Signalling on sem 0x%x, count=%d.\n", sem, sem->count);
    sem->count += 1;
    if (sem->count <= 0) {
	l = QueueFirst (&sem->waiting);
	QueueRemove (l);
	dbprintf ('s', "Waking up PCB 0x%x.\n", l->object);
	ProcessWakeup ((PCB *)(l->object));
	QueueFreeLink (l);
    }
    RestoreIntrs (intrs);
}
int SemHandleSignal(sem_t sem)
{
  if(sem>=0&&sem<MAX_SEMS)
  {
    if(sems[sem].inuse)
    {
      SemSignal(&sems[sem]);
      return 0;
    }
    return 1;
  }
  else
  {
    return 1;
  }
}


//----------------------------------------------------------------------
//
//	Your assignment is to implement locks and condition variables
//	using Mesa-style monitor synchronization. Fill in the details in
//	synch.c before making any changes here.
//
//----------------------------------------------------------------------

//-----------------------------------------------------------------------
//	LockCreate
//
//	LockCreate grabs a lock from the systeme-wide pool of locks and 
//	initializes it.
//	It also sets the inuse flag of the lock to indicate that the lock is
//	being used by a process. It returns a unique id for the lock. All the
//	references to the lock should be made through the returned id. The
//	process of grabbing the lock should be atomic.
//
//	If a new lock cannot be created, your implementation should return
//	INVALID_LOCK (see synch.h).
//-----------------------------------------------------------------------
lock_t LockCreate()
{
	lock_t l;								// declaring lock 'l'
	uint32 intrval;							// declaring interrupt

	intrval = DisableIntrs();				// turning off interrupts to ensure atomicity
	
	for (l = 0; l < MAX_LOCKS; l++) {		// for loop creating new locks until the max number (64) is reached 
		if (locks[l].inuse == 0) {			// checking if specific lock is being used already
			locks[l].inuse = 1;				// setting lock to 'being used' essentially
			break;
		}
	}
	
	RestoreIntrs(intrval);					// turning interrupts back on
	
	if (l == MAX_LOCKS) {					// if the max number of locks has been reached
		return INVALID_LOCK;				// letting the system know it has failed to create another lock
	}

	if (LockInit(&locks[l]) != 1) {			// if lock initializion fails
		return INVALID_LOCK;				// let the system know it failed to create a lock
	}
	
	return l;								// return the lock
}

int LockInit(Lock *l) {						// function for initializing the locks
	if (!l) {								// if there is no lock
		return INVALID_LOCK;				// letting the system know it failed to create a lock
	}	
	
	if (AQueueInit (&l->waiting) != 1) {	// if lock waiting queue fails to initialize
		printf("Lock waiting queue initialization has failed. Exiting...\n");
		exitsim();							// exits the simulator 
	}
	
	l->pid = -1;							// set pid to -1, meaning the pid is available to use
	
	return 1;								// return successful
}

//---------------------------------------------------------------------------
//	LockHandleAcquire
//
//	This routine acquires a lock given its handle. The handle must be a 
//	valid handle for this routine to succeed. In that case this routine 
//	returns a zero. Otherwise the routine returns a 1.
//
//	Your implementation should be such that if a process that already owns
//	the lock calls LockHandleAcquire for that lock, it should not block.
//---------------------------------------------------------------------------
int
LockAcquire(lock_t *k)
{
    Link	*l;								// declaring linked list l 
    int		intrval;						// declaring interrupt
    
	if (!k) {								// if there is no lock
		return INVALID_LOCK;				// let the system know it failed
	}


	intrval = DisableIntrs ();				// disable interrupts
	
	if (k->pid == GetCurrentPid()) {		// checks to see if lock is owned by the current process
    
		dbprintf('s', "LockAcquire: Process %d already owns lock %d\n", GetCurrentPid(), (int)(k-locks));
		RestoreIntrs(intrval);				// enable interrupts again
		
		return 1;							// success
	}


	dbprintf ('s', "LockAcquire: Process %d asking for lock %d.\n", GetCurrentPid(), (int)(k-locks));
	  
	if (k->pid >= 0) { 						// if lock is owned by a different process
		
		dbprintf('s', "LockAcquire: Process %d is going to sleep\n", GetCurrentPid());
		
		if ((l = AQueueAllocLink ((void *)currentPCB)) == NULL) {	// if a link for the lock could not be allocated
			
			printf("LockAcquire: Failed to allocate link for lock. Exiting...\n");
			exitsim();						// exiting the simulator
		}
		
		if (AQueueInsertLast (&k->waiting, l) != 1) {	// if inserting a new link into lock queue fails
			
			printf("LockAcquire: Failed to insert new link into lock waiting queue. Exiting...\n");
			exitsim();						// exiting the simulator
		}
		
		ProcessSleep();						// calls sleep function to put the process to sleep
	}
	
	
	else {									// else if no other process owns the lock
		
		dbprintf('s', "LockAcquire: Lock is not owned, assigning to process %d\n", GetCurrentPid());
		k->pid = GetCurrentPid();			// set pid to current PID
	  }
	  
	  RestoreIntrs(intrval);				// enable interrupts
	  return 1;								// return successful
	}


int LockHandleAcquire(lock_t lock) {		// function to ensure lock is successful
	if (lock < 0) {							// if there is a valid number of locks
		return INVALID_LOCK;				// fail
	}
	
	if (lock >= MAX_LOCKS) {				// if max number of locks has been reached
		return INVALID_LOCK;				// fail
	}
	
	if (!locks[lock].inuse) {				// if current lock is already owned
		return INVALID_LOCK;				// fail
	}
	
	return LockAcquire(&locks[lock]);		// success --> return output of LockAcquire function 
	}

//---------------------------------------------------------------------------
//	LockHandleRelease
//
//	This procedure releases the unique lock described by the handle. It
//	first checks whether the lock is a valid lock. If not, it returns 1.
//	If the lock is a valid lock, it should check whether the calling
//	process actually holds the lock. If not it returns 1. Otherwise it
//	releases the lock, and returns 0.
//---------------------------------------------------------------------------
int
LockRelease(lock_t lock)
{
	Link *l;								// declaring linked list l 
	int	intrs;								// declaring interrupts
	PCB *pcb;								// declare pcb

	if (!k) {								// if there is no lock
		return INVALID_LOCK;				// fail
	}

	intrs = DisableIntrs ();				// disable interrupts
	dbprintf ('s', "LockRelease: Lock %d is being released by Process %d.\n", (int)(k-locks), GetCurrentPid());

	if (k->pid != GetCurrentPid()) {		// if lock is not owned by current process
		
		dbprintf('s', "LockRelease: Lock %d is not owned by Process %d.\n", (int)(k-locks), GetCurrentPid());
		return INVALID_LOCK;				// fail
	}
	
	k->pid = -1;							// set pid to -1 to show the lock is now available.
	  
	if (!AQueueEmpty(&k->waiting)) { 		// if there are processes in the queue waiting to be woken up
		
		l = AQueueFirst(&k->waiting);		// set link to the process at the top of the waiting queue
		pcb = (PCB *)AQueueObject(l);		// set pcb to the process
		
		if (AQueueRemove(&l) != 1) { 		// if link could not be removed from the lock queue
			printf("LockRelease: Failed to remove link from lock queue. Exiting...\n");
			exitsim();						// exits the simulator
		}
		
		dbprintf ('s', "LockRelease: PID %d has been awoken and given a new lock.\n", (int)(GetPidFromAddress(pcb)));
		k->pid = GetPidFromAddress(pcb);	// function call that sets pid to the pid from the address
		
		ProcessWakeup (pcb);				// wakes up the process
	  }
	  
	RestoreIntrs (intrs);					// enables interrupts
	return 1;								// return successfull
}


int LockHandleRelease(lock_t lock) {		// function to ensure lock is successful
	if (lock < 0) {							// if there is a valid number of locks
		return INVALID_LOCK;				// fail
	}
	
	if (lock >= MAX_LOCKS) {				// if max number of locks has been reached
		return INVALID_LOCK;				// fail
	}
	
	if (!locks[lock].inuse) {				// if current lock is already owned
		return INVALID_LOCK;				// fail
	}
	
	return LockRelease(&locks[lock]);		// success --> return output of LockAcquire function 
	}

//--------------------------------------------------------------------------
//	CondCreate
//
//	This function grabs a condition variable from the system-wide pool of
//	condition variables and associates the specified lock with
//	it. It should also initialize all the fields that need to initialized.
//	The lock being associated should be a valid lock, which means that
//	it should have been obtained via previous call to LockCreate(). 
//	
//	If for some reason a condition variable cannot be created (no more
//	condition variables left, or the specified lock is not a valid lock),
//	this function should return INVALID_COND (see synch.h). Otherwise it
//	should return handle of the condition variable.
//--------------------------------------------------------------------------
cond_t
CondCreate(lock_t lock)
{
  // ignore this
}

//---------------------------------------------------------------------------
//	CondHandleWait
//
//	This function makes the calling process block on the condition variable
//	till either ConditionHandleSignal or ConditionHandleBroadcast is
//	received. The process calling CondHandleWait must have acquired the
//	lock associated with the condition variable (the lock that was passed
//	to CondCreate. This implies the lock handle needs to be stored
//	somewhere. hint! hint!) for this function to
//	succeed. If the calling process has not acquired the lock, it does not
//	block on the condition variable, but a value of 1 is returned
//	indicating that the call was not successful. Return value of 0 implies
//	that the call was successful.
//
//	This function should be written in such a way that the calling process
//	should release the lock associated with this condition variable before
//	going to sleep, so that the process that intends to signal this
//	process could acquire the lock for that purpose. After waking up, the
//	blocked process should acquire (i.e. wait on) the lock associated with
//	the condition variable. In other words, this process does not
//	"actually" wake up until the process calling CondHandleSignal or
//	CondHandleBroadcast releases the lock explicitly.
//---------------------------------------------------------------------------
int
CondHandleWait(cond_t cond)
{
  // ignore this
}

//---------------------------------------------------------------------------
//	CondHandleSignal
//
//	This call wakes up exactly one process waiting on the condition
//	variable, if at least one is waiting. If there are no processes
//	waiting on the condition variable, it does nothing. In either case,
//	the calling process must have acquired the lock associated with
//	condition variable for this call to succeed, in which case it returns
//	0. If the calling process does not own the lock, it returns 1,
//	indicating that the call was not successful. This function should be
//	written in such a way that the calling process should retain the
//	acquired lock even after the call completion (in other words, it
//	should not release the lock it has already acquired before the call).
//
//	Note that the process woken up by this call tries to acquire the lock
//	associated with the condition variable as soon as it wakes up. Thus,
//	for such a process to run, the process invoking CondHandleSignal
//	must explicitly release the lock after the call is complete.
//---------------------------------------------------------------------------
int
CondHandleSignal(cond_t cond)
{
  // ignore this
}

//---------------------------------------------------------------------------
//	CondHandleBroadcast
//
//	This function is very similar to CondHandleSignal. But instead of
//	waking only one process, it wakes up all the processes waiting on the
//	condition variable. For this call to succeed, the calling process must
//	have acquired the lock associated with the condition variable. This
//	function should be written in such a way that the calling process
//	should retain the lock even after call completion.
//
//	Note that the process woken up by this call tries to acquire the lock
//	associated with the condition variable as soon as it wakes up. Thus,
//	for such a process to run, the process invoking CondHandleBroadcast
//	must explicitly release the lock after the call completion.
//---------------------------------------------------------------------------
int
CondHandleBroadcast(cond_t cond)
{
  // ignore this
}
