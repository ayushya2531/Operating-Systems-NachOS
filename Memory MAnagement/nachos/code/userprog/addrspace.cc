// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
//#include <stdlib.h>
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// ProcessAddressSpace::ProcessAddressSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

ProcessAddressSpace::ProcessAddressSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;
    openexecutable = executable;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numVirtualPages = divRoundUp(size, PageSize);
    size = numVirtualPages * PageSize;
    backup = new char[size];
    /*if (rep_algo == 0){
	ASSERT(numVirtualPages+numPagesAllocated <= NumPhysPages);		// check we're not trying
										// to run anything too big --
										// at least until we have
										// virtual memory
	DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numVirtualPages, size);
	// first, set up the translation 
    	KernelPageTable = new TranslationEntry[numVirtualPages];
    	for (i = 0; i < numVirtualPages; i++) {
		KernelPageTable[i].virtualPage = i;
		//KernelPageTable[i].physicalPage = i+numPagesAllocated;
		if(numPagesAllocated < NumPhysPages){
                   if (nextunallocatedpage < NumPhysPages){                /// do remember to reverse this if block later
                      KernelPageTable[i].physicalPage = nextunallocatedpage++;
                   }else{
                         //pick page from free pool
                    }
		}
		KernelPageTable[i].valid = TRUE;
		KernelPageTable[i].use = FALSE;
		KernelPageTable[i].dirty = FALSE;
		KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
		KernelPageTable[i].shared = FALSE; // a separate page, we could set its 
					// pages to be read-only
	}
	// zero out the entire address space, to zero the unitialized data segment 
	// and the stack segment
	bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
 	//nextunallocatedpage += numVirtualPages; 
    	numPagesAllocated += numVirtualPages;
	stats->numPageFaults += numVirtualPages;
// then, copy in the code and data segments into memory
    	if (noffH.code.size > 0) {
        	DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        	vpn = noffH.code.virtualAddr/PageSize;
        	offset = noffH.code.virtualAddr%PageSize;
        	entry = &KernelPageTable[vpn];
        	pageFrame = entry->physicalPage;
        	executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.code.size, noffH.code.inFileAddr);
    	}
    	if (noffH.initData.size > 0) {
    	    DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        	vpn = noffH.initData.virtualAddr/PageSize;
        	offset = noffH.initData.virtualAddr%PageSize;
        	entry = &KernelPageTable[vpn];
        	pageFrame = entry->physicalPage;
        	executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
	}
    }else{*/
    KernelPageTable = new TranslationEntry[numVirtualPages];
    for (i = 0; i < numVirtualPages; i++) {
        KernelPageTable[i].virtualPage = i;
        KernelPageTable[i].physicalPage = -1;
        KernelPageTable[i].valid = FALSE;
        KernelPageTable[i].use = FALSE;
        KernelPageTable[i].dirty = FALSE;
        KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
        KernelPageTable[i].shared = FALSE; // a separate page, we could set its 
                                // pages to be read-only
        KernelPageTable[i].backup = FALSE;
    }
//	machine->KernelPageTable = KernelPageTable;
  //      machine->KernelPageTableSize = size;
//        bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);

    //}
}

//----------------------------------------------------------------------
// ProcessAddressSpace::ProcessAddressSpace (ProcessAddressSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddressSpace::ProcessAddressSpace(ProcessAddressSpace *parentSpace, int child_pid)
{
    numVirtualPages = parentSpace->GetNumPages();
    unsigned i,j, size = numVirtualPages * PageSize;
    openexecutable = parentSpace->openexecutable;
    //ASSERT(numVirtualPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
    backup = new char[size];                                                    // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Fork Initializing address space, num pages %d, size %d\n",
                                        numVirtualPages, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
 IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts
    KernelPageTable = new TranslationEntry[numVirtualPages];
    for (i = 0; i < numVirtualPages; i++) {
        KernelPageTable[i].virtualPage = i;
        if (parentPageTable[i].shared ==TRUE){
            KernelPageTable[i].physicalPage = parentPageTable[i].physicalPage;
        }
        else if (parentPageTable[i].valid == TRUE){
            stats->numPageFaults++;
            if(numPagesAllocated < NumPhysPages){
                if (nextunallocatedpage < NumPhysPages)                /// do remember to reverse this if block later
                    KernelPageTable[i].physicalPage = nextunallocatedpage++;
                else{
                    int*  fp = (int *)freePages->Remove();
                    KernelPageTable[i].physicalPage =  *fp;
                }
                numPagesAllocated++;
            }else if (rep_algo!=0){
		DEBUG('t', "Page replace called in child address spce");	
                KernelPageTable[i].physicalPage = PageReplace(parentPageTable[i].physicalPage);
            }else ASSERT(numPagesAllocated < NumPhysPages);
	    
	    bzero(&machine->mainMemory[(KernelPageTable[i].physicalPage)*PageSize], PageSize);
            for (j=0;j<PageSize;j++){
                machine->mainMemory[KernelPageTable[i].physicalPage*PageSize+j] = machine->mainMemory[parentPageTable[i].physicalPage*PageSize+j];
            }
            page_pid[KernelPageTable[i].physicalPage] = child_pid; 
            physical_to_virtual[KernelPageTable[i].physicalPage]=&KernelPageTable[i];
            DEBUG('t',"In fork,PHYSICAL %d TO VIRTUAL %d pid = %d \n",KernelPageTable[i].physicalPage,physical_to_virtual[KernelPageTable[i].physicalPage]->virtualPage,page_pid[KernelPageTable[i].physicalPage]);
            Count_ArrLRU[KernelPageTable[i].physicalPage] = stats->totalTicks;
            Count_ArrFIFO[KernelPageTable[i].physicalPage] = stats->totalTicks;
            Count_ArrLRU[parentPageTable[i].physicalPage] = stats->totalTicks + 1;
            ArrCLRU_s[KernelPageTable[i].physicalPage] = 1;
            ArrCLRU_s[parentPageTable[i].physicalPage] = 1;
	    currentThread->SortedInsertInWaitQueue(stats->totalTicks+1000); //put it in sleep
        }else{
            KernelPageTable[i].physicalPage = -1;
        }
            KernelPageTable[i].valid = parentPageTable[i].valid;
            KernelPageTable[i].use = parentPageTable[i].use;
            KernelPageTable[i].dirty = parentPageTable[i].dirty;
            KernelPageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
            KernelPageTable[i].shared = parentPageTable[i].shared;                // a separate page, we could set its
            KernelPageTable[i].backup = parentPageTable[i].backup;
            //DEBUG('t',"Backup = %d vpn = %d \n",KernelPageTable[i].backup, i ); 		//be read-only
                        
        }
        //copy backup
    for (i=0;i<size;i++)
	    backup[i] = parentSpace->backup[i];
 (void) interrupt->SetLevel(oldLevel); // re-enable interrupt
    // Copy the contents
    //unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    //unsigned startAddrChild = numPagesAllocated*PageSize;
    /*r (i=0; i<numVirtualPages; i++) {
	unsigned startAddrParent = parentPageTable[i].physicalPage*PageSize;
    	unsigned startAddrChild = KernelPageTable[i].physicalPage*PageSize;
	if (!parentPageTable[i].shared && parentPageTable[i].valid){
	    for (j=0;j<PageSize;j++){
        	machine->mainMemory[startAddrChild+j] = machine->mainMemory[startAddrParent+j];
	    }
	}
    }*/
    //numPagesAllocated += numVirtualPages;
    //    machine->KernelPageTable = KernelPageTable;
      //  machine->KernelPageTableSize = size;

}
//---------------------------------------------------------------------
// int ProcessAddressSpace::ProcessAddressSpace (int shared_size) is called by a syscall Shm_Allocate.
//      We need to duplicate the address space of the currentThread + add new shared space.
//----------------------------------------------------------------------
void ProcessAddressSpace::ShmAllocate(int shared_size)
{
    //numVirtualPages = currentThread->space->GetNumPages();
    unsigned i, size = numVirtualPages * PageSize;
    int shared_pages = divRoundUp(shared_size, PageSize);
    stats->numPageFaults += shared_pages;
    ASSERT(shared_pages+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Initializing address space in shmallocate, num pages %d, size %d\n",
                                        numVirtualPages+shared_pages, size+shared_pages*PageSize);
    // first, set up the translation
    TranslationEntry* OldTable = KernelPageTable;
    KernelPageTable = new TranslationEntry[numVirtualPages+shared_pages];
    for (i = 0; i < numVirtualPages; i++) {

        KernelPageTable[i].virtualPage = OldTable[i].virtualPage;
        KernelPageTable[i].physicalPage = OldTable[i].physicalPage;
        KernelPageTable[i].valid = OldTable[i].valid;
        KernelPageTable[i].use = OldTable[i].use;
        KernelPageTable[i].dirty = OldTable[i].dirty;
        KernelPageTable[i].readOnly = OldTable[i].readOnly;      // if the code segment was entirely on
        KernelPageTable[i].shared = OldTable[i].shared;         // a separate page, we could set its
        KernelPageTable[i].backup = OldTable[i].backup;         // pages to be read-only
	//DEBUG('t',"VPN = %d PhysPage = %d \n",OldTable[i].virtualPage,OldTable[i].physicalPage);
        if(KernelPageTable[i].valid==TRUE)
            physical_to_virtual[KernelPageTable[i].physicalPage]=&KernelPageTable[i];

    }
    //DEBUG('a', "Hi 1\n");
    for (i = numVirtualPages; i < numVirtualPages+shared_pages; i++) {
        KernelPageTable[i].virtualPage = i;
        //KernelPageTable[i].physicalPage = nextunallocatedpage++;
	if(numPagesAllocated < NumPhysPages){
        if (nextunallocatedpage < NumPhysPages)                /// do remember to reverse this if block later
                KernelPageTable[i].physicalPage = nextunallocatedpage++;
		else{
			int*  fp = (int *)freePages->Remove();
                        KernelPageTable[i].physicalPage =  *fp;}
                numPagesAllocated++;
        }else if (rep_algo!=0){
		DEBUG('t',"Called page replace in shmallocate\n");
                KernelPageTable[i].physicalPage = PageReplace(-1);//replace a page
        }else ASSERT(numPagesAllocated < NumPhysPages);
	DEBUG('t',"assigned vpn %d to ppn %d in shm\n",i,KernelPageTable[i].physicalPage );
        //DEBUG('t',"Shared Page vpn = %d Physpn = %d pid = [%d]",i,KernelPageTable[i].physicalPage,currentThread->GetPID());
	bzero(&machine->mainMemory[(KernelPageTable[i].physicalPage)*PageSize], PageSize);
        physical_to_virtual[KernelPageTable[i].physicalPage]=&KernelPageTable[i];
        KernelPageTable[i].valid = TRUE;
        KernelPageTable[i].use = FALSE;
        KernelPageTable[i].dirty = FALSE;
        KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
        KernelPageTable[i].shared = TRUE; // a separate page, we could set its 
        KernelPageTable[i].backup = FALSE;             // pages to be read-only
        //DEBUG('t',"VPN = %d PhysPage = %d \n",OldTable[i].virtualPage,OldTable[i].physicalPage);

    }
   //DEBUG('a', "Hi 2\n");
   //numPagesAllocated += shared_pages;
   numVirtualPages += shared_pages;
    // Copy the contents
    //unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    //unsigned startAddrChild = numPagesAllocated*PageSize;
    //for (i=0; i<size; i++) {
    //   machine->mainMemory[startAddrChild+i] = machine->mainMemory[startAddrParent+i];
    //}
   machine->KernelPageTable = KernelPageTable;
   //DEBUG('a', "Hi 3\n"); 
   delete OldTable; 
    machine->KernelPageTableSize+=shared_pages;
    //currentThread->space->numVirtualPages += shared_pages;
//    return size; 
  // DEBUG('a', "Hi 4 %d\n",numVirtualPages);
}

//----------------------------------------------------------------------
// ProcessAddressSpace::~ProcessAddressSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

ProcessAddressSpace::~ProcessAddressSpace()
{
   delete KernelPageTable;
}


//AllocateNextPage - To allocate the next page depending on the page replacement algorithm

bool ProcessAddressSpace::AllocateNextPage(int vaddr)
{
    int vpn = vaddr/PageSize; 
    //DEBUG('t',"VPN = %d Physical Page assigned = %d\n",vpn,numPagesAllocated);
    if(numPagesAllocated < NumPhysPages){
	if (nextunallocatedpage < NumPhysPages)                /// do remember to reverse this if block later
	        KernelPageTable[vpn].physicalPage = nextunallocatedpage++;
        else{
            	int*  fp = (int *)freePages->Remove();
                KernelPageTable[vpn].physicalPage =  *fp;
	}
        numPagesAllocated++;
    }else if(rep_algo != 0){
	DEBUG('t',"Called pace replace in Allocatenext page for vpn=%d, pid = %d\n",vpn, currentThread->GetPID());
	KernelPageTable[vpn].physicalPage = PageReplace(-1); 	//select page for replacement depending on rep_algo
    } 
    else ASSERT(numPagesAllocated<NumPhysPages);
    physical_to_virtual[KernelPageTable[vpn].physicalPage]=&KernelPageTable[vpn];
    page_pid[KernelPageTable[vpn].physicalPage] = currentThread->GetPID();
    DEBUG('t',"IN allocatenextpage PHYSICAL %d TO VIRTUAL %d pid = %d \n",KernelPageTable[vpn].physicalPage,physical_to_virtual[KernelPageTable[vpn].physicalPage]->virtualPage,page_pid[KernelPageTable[vpn].physicalPage]);

    //DEBUG('t',"assigned physical page %d to vpn %d\n",KernelPageTable[vpn].physicalPage,vpn);
    KernelPageTable[vpn].valid = TRUE;
    /*if( vpn == (KernelPageTableSize - 1) ) {                 will look into it later
        readSize = size - vpn * PageSize;                                        
    }else readSize = PageSize;*/                  
    //machine->KernelPageTable = KernelPageTable;
//    DEBUG('a',"physical page = %d \n", KernelPageTable[vpn].physicalPage);
    //machine->
    bzero(&machine->mainMemory[(KernelPageTable[vpn].physicalPage)*PageSize], PageSize);
    if(KernelPageTable[vpn].backup == FALSE){
        NoffHeader noffH;
        openexecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        //DEBUG('t',"physical page = %d \n", KernelPageTable[vpn].physicalPage);
        if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
        ASSERT(noffH.noffMagic == NOFFMAGIC); 
        openexecutable->ReadAt(&(machine->mainMemory[KernelPageTable[vpn].physicalPage * PageSize]), PageSize, noffH.code.inFileAddr + vpn*PageSize);
        //DEBUG('t',"VPN = %d Physical Page assigned = %d\n",vpn,numPagesAllocated);
    }
    else{
	
	DEBUG('t',"Loading data from backup for pid = %d\n",currentThread->GetPID());
	//LOAD from backup
	for(int i=0;i<PageSize;i++){
    	machine->mainMemory[KernelPageTable[vpn].physicalPage * PageSize+i] = backup[vpn*PageSize+i];
//		DEBUG('t',"%c\n", backup[vpn*PageSize+i]);
	}
    }
    Count_ArrFIFO[KernelPageTable[vpn].physicalPage] = stats->totalTicks;
    machine->KernelPageTable = KernelPageTable;
    return TRUE;
}
//----------------------------------------------------------------------
// ProcessAddressSpace::InitUserModeCPURegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------
void
ProcessAddressSpace::InitUserModeCPURegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numVirtualPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numVirtualPages * PageSize - 16);
}
//---------------------------------------------------------------------
//ProcessAddressSpace::Free_Exiting_Pages()
//Free all pages belonging to the thread exiting currently
//
//--------------------------------------------------------------------
void ProcessAddressSpace::Free_Exiting_Pages()
{
    int i=0;
    int * temp;
    for(i=0;i<numVirtualPages;i++)
    {
	temp = new int(KernelPageTable[i].physicalPage);
	if(KernelPageTable[i].valid && !KernelPageTable[i].shared){
	    numPagesAllocated--;
	    freePages->Append((void*)temp);
            physical_to_virtual[KernelPageTable[i].physicalPage]=NULL;

	    // Remove the entry from the hashmap
	    
	}
    }
}
//---------------------------------------------------------------------
//int ProcessAddressSpace::PageReplace()
//Returns the physical page number which is switched out of main memory
//---------------------------------------------------------------------
int ProcessAddressSpace::PageReplace(int parent){
    //DEBUG('t',"In PageReplace ParentPage = %d\n",parent);
    //DEBUG('t',"*******************   page linked to 3 is %d ******************* ", physical_to_virtual[3]->virtualPage);
    int x;
    if (rep_algo == 1)
    {
	   x=Random()%NumPhysPages;
	   while (physical_to_virtual[x]->shared == TRUE || parent==x)
       {
		  x=Random()%NumPhysPages;
	   }

    }
    else if (rep_algo == 2)
    {
          
        x = -1;
        int min = stats->totalTicks;
        for(int index = 0;index<NumPhysPages;index++)
        {
            if(physical_to_virtual[index]->shared == TRUE || parent==index)
                continue;
            else
            {
                if(Count_ArrFIFO[index] < min)
                {
                    min = Count_ArrFIFO[index];
                    x = index;
                }
            }
        }
        ASSERT(x >= 0);

    }
    else if (rep_algo == 3)
    {
        x = -1;
        //DEBUG('a',"Hello");
        int min = stats->totalTicks;
        for(int index = 0;index<NumPhysPages;index++)
        {
            DEBUG('a'," Time is %d for index %d with mint %d \n",Count_ArrLRU[index],index,min);
            if(physical_to_virtual[index]->shared == TRUE || parent==index)
                continue;
            else
            {
                if(Count_ArrLRU[index] <= min)
                {
                    min = Count_ArrLRU[index];
                    x = index;
                }
            }
        }
        ASSERT(x >= 0);
    }
    else if (rep_algo == 4)
    {
        x =-1;
	int index;
        int initindex = clockindex;
        clockindex = clockindex + 1;
        int finindex = clockindex + NumPhysPages;
        for(int ind = clockindex; ind<=finindex; ind++)
        {
            index = ind%NumPhysPages;
            if(physical_to_virtual[index]->shared == TRUE || parent==index)
                continue;
            else
            {
                if(ArrCLRU_s[index] != 1) 
                {
                     x = index;
                     break;
                }
                else if(ArrCLRU_s[index] == 1)
                {
                    ArrCLRU_s[index] = 0;
                }
            }
        }
        ArrCLRU_s[x] = 1;
        clockindex = x;
    }

    physical_to_virtual[x]->valid = FALSE;
    int pid = page_pid[x];
   if (physical_to_virtual[x]->dirty==TRUE)
   {
        physical_to_virtual[x]->backup = TRUE;
        DEBUG('t',"Setting backup to true for pid = %d, vpn = %d\n",pid,physical_to_virtual[x]->virtualPage);
       for(int j = 0 ; j<PageSize;j++ )
       {
        threadArray[pid]->space->backup[physical_to_virtual[x]->virtualPage*PageSize+j] = machine->mainMemory[x*PageSize+j];
        //DEBUG('t',"Writing Backup memeory %c\n",machine->mainMemory[x*PageSize+j]);
       }
       physical_to_virtual[x]->dirty = FALSE;
    }
        //DEBUG('t'," In REPLACE PHYSICAL %d TO VIRTUAL %d pid = %d \n",x,physical_to_virtual[x]->virtualPage,page_pid[x]);

   DEBUG('t',"Replaced page from pid = %d vpn = %d PhysPageNum = %d\n",pid,physical_to_virtual[x]->virtualPage,x);
   return x;

}
//----------------------------------------------------------------------
// ProcessAddressSpace::SaveContextOnSwitch
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void ProcessAddressSpace::SaveContextOnSwitch() 
{}

//----------------------------------------------------------------------
// ProcessAddressSpace::RestoreContextOnSwitch
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void ProcessAddressSpace::RestoreContextOnSwitch() 
{
    machine->KernelPageTable = KernelPageTable;
    machine->KernelPageTableSize = numVirtualPages;
}

unsigned
ProcessAddressSpace::GetNumPages()
{
   return numVirtualPages;
}

TranslationEntry*
ProcessAddressSpace::GetPageTable()
{
   return KernelPageTable;
}
