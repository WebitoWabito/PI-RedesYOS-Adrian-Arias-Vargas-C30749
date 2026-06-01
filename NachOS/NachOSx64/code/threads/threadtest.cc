// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//

#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "diningph.h"
#include "h2o.h"

DiningPh * dp;
H2O * h2o;

void Philo( void * p ) {

    int eats, thinks;
    long who = (long) p;

    currentThread->Yield();

    for ( int i = 0; i < 10; i++ ) {

        printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

        dp->pickup( who );
        dp->print();
        eats = Random() % 6;

        currentThread->Yield();
        sleep( eats );

        dp->putdown( who );

        thinks = Random() % 6;
        currentThread->Yield();
        sleep( thinks );
    }

}

void HydrogenThread( void * p ) {
    long who = (long) p;
    
    for ( int i = 0; i < 5; i++ ) {
        printf("Hydrogen %ld: looking for partner\n", who);
        currentThread->Yield();
        h2o->hydrogen();
        printf("Hydrogen %ld: formed H2O molecule %d\n", who, i + 1);
        currentThread->Yield();
    }
}


void OxygenThread( void * p ) {
    long who = (long) p;
    
    for ( int i = 0; i < 5; i++ ) {
        printf("Oxygen %ld: looking for partners\n", who);
        currentThread->Yield();
        h2o->oxygen();
        printf("Oxygen %ld: formed H2O molecule %d\n", who, i + 1);
        currentThread->Yield();
    }
}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    char* threadName = (char*)name;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    for (int num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf("*** thread %s looped %d times\n", threadName, num);
	    //interrupt->SetLevel(oldLevel);
        //currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", threadName);
    //interrupt->SetLevel(oldLevel);
}


void
TestH2O()
{
    Thread * t;

    DEBUG('t', "Entering H2O Test");

    h2o = new H2O();

    // Create hydrogen threads (2 minimum needed for each water molecule)
    for ( long k = 0; k < 4; k++ ) {
        t = new Thread( "H" );
        t->Fork( HydrogenThread, (void *) k );
    }

    // Create oxygen threads (1 needed for each water molecule)
    for ( long k = 0; k < 2; k++ ) {
        t = new Thread( "O" );
        t->Fork( OxygenThread, (void *) k );
    }
    // To test dp, comment this test and uncomment the ThreadTest
}


//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

/*void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    // Run Dining Philosophers Problem (default)
    dp = new DiningPh();
    Thread * Ph;
    for ( long k = 0; k < 5; k++ ) {
        Ph = new Thread( "dp" );
        Ph->Fork( Philo, (void *) k );
    }

    //To test h2o comment this test and uncomment the H2O test
}
*/
