#include <chrono>
#include <cstdint>
#include <memory>
#include <iostream>
#include <thread>
#include "System.h"
#include "part.h"
#include "vm_declarations.h"
#include "ProcessTest.h"
#include "SystemTest.h"
#include <unordered_map>
#include <string.h>

#define VM_SPACE_SIZE (500)
#define PMT_SPACE_SIZE (5000)
#define N_PROCESS (5)
#define PERIODIC_JOB_COST (1)

PhysicalAddress alignPointer(PhysicalAddress address) {
    uint64_t addr = reinterpret_cast<uint64_t> (address);

    addr += PAGE_SIZE;
    addr = addr / PAGE_SIZE * PAGE_SIZE;

    return reinterpret_cast<PhysicalAddress> (addr);
}

int main()
{
	/*
	Partition part("p1.ini");
	uint64_t size = (VM_SPACE_SIZE + 2) * PAGE_SIZE;
	PhysicalAddress vmSpace = (PhysicalAddress) new char[size];
	PhysicalAddress alignedVmSpace = alignPointer(vmSpace);

	size = (PMT_SPACE_SIZE + 2) * PAGE_SIZE;
	PhysicalAddress pmtSpace = (PhysicalAddress) new char[size];
	PhysicalAddress alignedPmtSpace = alignPointer(pmtSpace);

	System system(alignedVmSpace, VM_SPACE_SIZE, alignedPmtSpace, PMT_SPACE_SIZE, &part);


	Process *p1 = system.createProcess();
	p1->createSharedSegment(0x020000, 200, "pero", AccessType::READ_WRITE);

	Process *p2 = system.createProcess();
	p2->createSharedSegment(0x080000, 200, "pero", AccessType::READ_WRITE);
	p2->createSegment(0x020000, 10, AccessType::READ);
	system.access(1, 0x080000, AccessType::READ_WRITE);
	p2->pageFault(0x020000);
	system.access(0, 0x020000, AccessType::READ_WRITE);

	Process *p3 = p2->clone(2);
	system.access(2, 0x080000, AccessType::READ_WRITE);
	system.access(2, 0x020000, AccessType::READ);
	
	//p1->createSegment(0x020000, 200, 1);

	//Process *clone = p1->clone(3);
	
	//p1->disconnectSharedSegment("pero");
	//p2->disconnectSharedSegment("pero");
	//p2->createSharedSegment(0x080000, 200, "pero", AccessType::READ_WRITE);
	//p1->deleteSharedSegment("pero");
	//p2->createSharedSegment(0x080000, 200, "pero", AccessType::READ_WRITE);
	
	*/
	
    Partition part("p1.ini");

    uint64_t size = (VM_SPACE_SIZE + 2) * PAGE_SIZE;
    PhysicalAddress vmSpace = (PhysicalAddress ) new char[size];
    PhysicalAddress alignedVmSpace = alignPointer(vmSpace);

    size = (PMT_SPACE_SIZE + 2) * PAGE_SIZE;
    PhysicalAddress pmtSpace = (PhysicalAddress ) new char[size];
    PhysicalAddress alignedPmtSpace = alignPointer(pmtSpace);

    System system(alignedVmSpace, VM_SPACE_SIZE, alignedPmtSpace, PMT_SPACE_SIZE, &part);
    SystemTest systemTest(system, alignedVmSpace, VM_SPACE_SIZE);
    ProcessTest* process[N_PROCESS];
    std::thread *threads[N_PROCESS];
	
    std::mutex globalMutex;

    for (int i = 0; i < N_PROCESS; i++) {
		process[i] = new ProcessTest(system, systemTest);
    }
	
	for (int i = 0; i < N_PROCESS; i++) {
		std::cout << "Create process " << i << std::endl;
        threads[i] = new std::thread(&ProcessTest::run, process[i]);
    }
	
    Time time;
    while ((time = system.periodicJob())) {
        std::this_thread::sleep_for(std::chrono::microseconds(time));

        std::lock_guard<std::mutex> guard(systemTest.getGlobalMutex());

        std::cout << "Doing periodic job\n";

        std::this_thread::sleep_for(std::chrono::microseconds(PERIODIC_JOB_COST));

        bool finished = true;
        for (int i = 0; i < N_PROCESS; i++) {
            finished = finished && process[i]->isFinished();
        }

        if (finished) {
            break;
        }
    }
	
    for (int i = 0; i < N_PROCESS; i++) {
        threads[i]->join();
        delete threads[i];
        delete process[i];
    }

    delete [] vmSpace;
    delete [] pmtSpace;

	
    std::cout << "Test finished\n";
	
}