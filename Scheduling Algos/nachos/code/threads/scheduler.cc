// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling SelectNextReadyThread(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// ProcessScheduler::ProcessScheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

ProcessScheduler::ProcessScheduler()
{

    if(s_algo<=6) 
    listOfReadyThreads = new List; 
} 

//----------------------------------------------------------------------
// ProcessScheduler::~ProcessScheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

ProcessScheduler::~ProcessScheduler()
{ 
    if(s_algo<=6)
    delete listOfReadyThreads; 
} 

//----------------------------------------------------------------------
// ProcessScheduler::MoveThreadToReadyQueue
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
ProcessScheduler::MoveThreadToReadyQueue (NachOSThread *thread)
{
    DEBUG('t', "Putting thread %s with PID %d on ready list.\n", thread->getName(), thread->GetPID());
    thread->wait_start = stats->totalTicks;
    thread->setStatus(READY);
    //For SJF
    if (s_algo==2){
        //error += abs(currentThread->estimated_burst-current_burst);
        //printf("Error : pid  %d : %d\n",currentThread->GetPID(),currentThread->estimated_burst-current_burst);
    	//thread->estimated_burst = (thread->thread_burst + thread->estimated_burst)/2;    //value of a chosen as 1/2 
	listOfReadyThreads->SortedInsert((void *)thread,thread->estimated_burst);}
    else if(s_algo<=6)
    	listOfReadyThreads->Append((void *)thread);
}

//----------------------------------------------------------------------
// ProcessScheduler::SelectNextReadyThread
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

NachOSThread *
ProcessScheduler::SelectNextReadyThread ()
{
    if (s_algo>6){    //For UNIX scheduling
	int i,min;
	NachOSThread *minthread=NULL;
	for(i=1;i<thread_index;i++)
	{
	   if(!exitThreadArray[i] && (threadArray[i]->getStatus()==READY || threadArray[i]->getStatus()==RUNNING))
	   {
		min = threadArray[i]->priority;
		minthread=threadArray[i];
		break;
	   }
	}
//	printf("i==%d and pid == %d\n",i,minthread->GetPID());
	for(i=1;i<thread_index;i++)
	{
	    //if(threadArray[i]->getStatus()==READY || threadArray[i]->getStatus()==RUNNING)
	   //	 DEBUG('t',"Thread %d priority is %d \n",threadArray[i]->GetPID(),threadArray[i]->priority);

	    if(!exitThreadArray[i] && (threadArray[i]->getStatus()==READY || threadArray[i]->getStatus()==RUNNING))
	    {
		if( threadArray[i]->priority < min ){
		min=threadArray[i]->priority;
		minthread=threadArray[i];
	    	}
	        else if( threadArray[i]->priority == min)
                {
                    if( threadArray[i]->wait_start < minthread->wait_start )
                    {
			min=threadArray[i]->priority;
                	minthread=threadArray[i];
                    }
		}
	    }
	}
	//if ((minthread!=NULL && (currentThread->priority<minthread->priority)) || minthread == NULL){
	//	return currentThread;
	//}
	if(minthread!=NULL)
	DEBUG('t',"Next thread selected is %d value of i= %d \n", minthread->GetPID(),i);
	return minthread;
     	}
	else
    	    return (NachOSThread *)listOfReadyThreads->Remove();
}

//----------------------------------------------------------------------
// ProcessScheduler::ScheduleThread
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
ProcessScheduler::ScheduleThread (NachOSThread *nextThread)
{
    DEBUG('t',"Now in scheduleThread");
    NachOSThread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveContextOnSwitch();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    if (currentThread->getStatus()==READY){
	total_wait += stats->totalTicks - currentThread->wait_start;
    }
    current_burst = stats->totalTicks;

    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" with pid %d to thread \"%s\" with pid %d\n",
	  oldThread->getName(), oldThread->GetPID(), nextThread->getName(), nextThread->GetPID());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
// DEBUG('t', "Now in thread \"%s\" with pid %d\n", currentThread->getName(), currentThread->GetPID());
     DEBUG('t', "Now in thread *****************\"%s\" *************with pid %d\n", currentThread->getName(), currentThread->GetPID());
//    Print();
    _SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread ++++++++++++++++++\"%s\"+++++++++++++++++ with pid %d\n", currentThread->getName(), currentThread->GetPID());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreContextOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// ProcessScheduler::Tail
//      This is the portion of ProcessScheduler::ScheduleThread after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
ProcessScheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreContextOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// ProcessScheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
ProcessScheduler::Print()
{
    printf("Ready list contents:\n");
    listOfReadyThreads->Mapcar((VoidFunctionPtr) ThreadPrint);
}
