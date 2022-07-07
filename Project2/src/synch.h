//
//	synch.h
//
//	Include file for synchronization stuff.  The synchronization
//	primitives include:
//	Semaphore
//	Lock
//	Condition
//
//	Semaphores are the only "native" synchronization primitive.
//	Condition variables and locks are implemented using semaphores.
//

#ifndef	_synch_h_
#define	_synch_h_

#include "queue.h"

#define MAX_SEMS	32	//Maximum 32 semaphores allowed in the system
#define MAX_LOCKS	64	//Maximum 64 locks allowed in the system
				//This is because condition vars also use
				//locks from the same pool
#define MAX_CONDS	32	//Maximum 32 conds allowed in the system

typedef int sem_t;
typedef int lock_t;
typedef int cond_t;

#define INVALID_SEM -1
#define INVALID_LOCK -1
#define INVALID_PROC -1
#define INVALID_COND -1
typedef struct Sem {
    Queue	waiting;
    int		count;
    uint32	inuse; 		//indicates whether the semaphore is being
    				//used by any process
} Sem;

extern void	SemInit (Sem *, int);
extern void	SemWait (Sem *);
extern void	SemSignal (Sem *);

typedef struct Lock {
	int pid;			// PID of whatever process has the lock
	Queue	waiting;	// list of processes waiting for the lock
	uint32	inuse;		// indicates whether the lock is being used by any other process
} Lock;

int LockInit(Lock *);
int LockAcquire(Lock *);
int LockRelease(Lock *);

typedef struct Cond {

	//ignore this
} Cond;

#endif	//_synch_h_
