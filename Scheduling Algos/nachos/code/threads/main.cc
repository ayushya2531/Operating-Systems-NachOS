// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN
#include "syscall.h"
#include "utility.h"
#include "system.h"
//#include "scheduler.h"
// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void LaunchUserProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void syscall_wrapper_Exit(int status);
extern void ForkStartFunction(int dummy);                                                                                                                                           
//extern void MoveThreadToReadyQueue(NachOSThread *thread);
//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int argCount;			// the number of arguments 
					// for a particular command

    DEBUG('t', "Entering main");
    (void) Initialize(argc, argv);
    
#ifdef THREADS
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // print copyright
            printf (copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) {        	// run a user program
	    ASSERT(argc > 1);
            LaunchUserProcess(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-c")) {      // test the console
	    if (argc == 1)
	        ConsoleTest(NULL, NULL);
	    else {
		ASSERT(argc > 2);
	        ConsoleTest(*(argv + 1), *(argv + 2));
	        argCount = 3;
	    }
	    interrupt->Halt();		// once we start the console, then 
					// Nachos will loop forever waiting 
					// for console input
	}else if (!strcmp(*argv, "-F")) {
            ASSERT(argc > 1);
            OpenFile *txtfile = fileSystem->Open(*(argv+1));
            if (txtfile == NULL) {
                printf("Unable to open file %s\n", *(argv+1));
                argCount = 2;
            	continue;
	    }
	    int len = txtfile->Length(),start = 0;
	    char scheduling_algo[3],a[1];
	    txtfile->ReadAt(a,1,start);
	    while (a[0]!='\n'){
		scheduling_algo[start] = a[0];		
		txtfile->ReadAt(a,1,++start);
	    }
	    scheduling_algo[start]='\0';
	    s_algo = atoi(scheduling_algo);
	    start++;
	    char buff[len];
	    txtfile->ReadAt(buff,len-start,start);
	    //printf("start point is%d\nnd the first 2 characters are %c %c\n",start,buff[0],buff[1]);
	    int i=0,str=0,j=0,num=0,pri=0,init_priority[100];
            char name[100],prty[100];
            while (i<=len-start-1){
		if (buff[i]=='\n' || i==len-start-1){
                    name[str]='\0';	
		   // printf("%s\n",name);
                    if (num==0){ init_priority[j]=100;}
                    else{ 
			prty[num]='\0';
			init_priority[j] = atoi(prty); }
		    OpenFile *executable = fileSystem->Open(name);
		    ProcessAddressSpace *space;
		    if (executable == NULL) {
		        printf("Unable to open file %s\n", name);
		        return  0;
		    }
    		    space = new ProcessAddressSpace(executable);
                    delete executable;         // close file

		    NachOSThread *newthread = new NachOSThread("seq_thread");
                    space->InitUserModeCPURegisters();             // set the initial register values
		    newthread->base_priority = 50 + init_priority[j];
//		    printf("%d : priority %d\n",j,init_priority[j]);
                    newthread->cpu_count = 0;
		    newthread->space = space;
		    newthread->CreateThreadStack(ForkStartFunction,0);
		    // set the initial register values
		    //newthread->space->RestoreContextOnSwitch();            // load page table register
		    newthread->SaveUserState();
                    num = 0;str=0;pri=0;j++;
                    ASSERT(newthread!=NULL);
                    //newthread->priority = 50 + init_priority[j];
                    //newthread->cpu_count = 0;

		    scheduler->MoveThreadToReadyQueue(newthread);
		   
		}else if (buff[i]==' '){
                    pri=1;
                }else if(pri==0){
                    name[str] = buff[i];
                    str++;
                }else{
                    prty[num] = buff[i];
                    num++;
                }
       		i++;
	    }
//	    printf("i is %d last name %s and j is %d",i,name,j);
	    DEBUG('t', "Out of here");

	    argCount = 2;
		
		exitThreadArray[currentThread->GetPID()] = true;
	//	for (i=0; i<thread_index; i++) {                                
        //	  if (!exitThreadArray[i]) break;                              
       	//	}
		mte=stats->totalTicks;
       		currentThread->Exit(0, 0);
        }

#endif // USER_PROGRAM
#ifdef FILESYS
	if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
	    ASSERT(argc > 2);
	    Copy(*(argv + 1), *(argv + 2));
	    argCount = 3;
	} else if (!strcmp(*argv, "-p")) {	// print a Nachos file
	    ASSERT(argc > 1);
	    Print(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-r")) {	// remove Nachos file
		ASSERT(argc > 1);
		fileSystem->Remove(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-l")) {	// list Nachos directory
            fileSystem->List();
	} else if (!strcmp(*argv, "-D")) {	// print entire filesystem
            fileSystem->Print();
	} else if (!strcmp(*argv, "-t")) {	// performance test
            PerformanceTest();
	} 
#endif // FILESYS
#ifdef NETWORK
        if (!strcmp(*argv, "-o")) {
	    ASSERT(argc > 1);
            Delay(2); 				// delay for 2 seconds
						// to give the user time to 
						// start up another nachos
            MailTest(atoi(*(argv + 1)));
            argCount = 2;
        }
#endif // NETWORK
    }
    currentThread->FinishThread();	// NOTE: if the procedure "main" 
				// returns, then the program "nachos"
				// will exit (as any other normal program
				// would).  But there may be other
				// threads on the ready list.  We switch
				// to those threads by saying that the
				// "main" thread is finished, preventing
				// it from returning.
    return(0);			// Not reached...
}
