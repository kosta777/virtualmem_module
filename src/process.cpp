#include "process.h"
#include "kernelprocess.h"
#include "vm_declarations.h"
#include <iostream>
Process::Process(ProcessId pid)
{
	pProcess = new KernelProcess(pid);
}

Process::~Process()
{	
	delete pProcess;
}

ProcessId Process::getProcessId() const
{
	return pProcess->getProcessId();
}

Status Process::createSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags)
{
	return pProcess->createSegment(startAddress, segmentSize, flags);
}

Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags, void* content)
{
	return pProcess->loadSegment(startAddress, segmentSize, flags, content);
}

Status Process::deleteSegment(VirtualAddress startAddress)
{
	return pProcess->deleteSegment(startAddress, false);
}

Status Process::pageFault(VirtualAddress address)
{
	return pProcess->pageFault(address);
}

PhysicalAddress Process::getPhysicalAddress(VirtualAddress address)
{
	return pProcess->getPhysicalAddress(address);
}

Status Process::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags)
{
	return pProcess->createSharedSegment(startAddress, segmentSize, name, flags);
}

Status Process::disconnectSharedSegment(const char* name)
{
	return pProcess->disconnectSharedSegment(name);
}

Status Process::deleteSharedSegment(const char* name)
{
	return pProcess->deleteSharedSegment(name);
}

Process* Process::clone(ProcessId pid)
{
	return pProcess->clone(pid);
}

Process::Process(ProcessId pid, KernelProcess *kProcess)
{
	this->pProcess = kProcess;
}