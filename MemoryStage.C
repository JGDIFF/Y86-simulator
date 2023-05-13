#include <string>
#include <cstdint>
#include <iostream>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"



/*
 * doClockLow
 * When the clock ticks low, we grab values, determine if we need to read our write to memory, and do such.  
 *
 *
 */
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
	M * mreg = (M *) pregs[MREG];
	W * wreg = (W *) pregs[WREG];
	Memory * mem = Memory::getInstance();
	bool error = false;
	
	valM = 0;
	
	stat = mreg->getstat()->getOutput();
	uint64_t icode = mreg->geticode()->getOutput();
	uint64_t valE = mreg->getvalE()->getOutput();
	uint64_t valA = mreg->getvalA()->getOutput();
	uint64_t dstE = mreg->getdstE()->getOutput();
	uint64_t dstM = mreg->getdstM()->getOutput();
	
	uint64_t address = addr(icode, valE, valA); // MAYBE SHOULD BE A uint32_t !!! (If buggy later).


	if (memRead(icode))
	{
		valM = mem->getLong(address, error);
	}
	
	if (memWrite(icode))
	{
		mem->putLong(valA, address, error);
	}
	
	if (error)
	{
		stat = SADR;
	}
	setWInput(wreg, stat, icode, valE, valM, dstE, dstM);
	
	return true;
}

/*
 * doClockHigh
 * When clock ticks high, normal the Writeback piperegister.
 * The MemoryStage does not bubble or stall
 *
 */
void MemoryStage::doClockHigh(PipeReg ** pregs)
{
	W * wreg = (W *) pregs[WREG];
	
	wreg->getstat()->normal();
	wreg->geticode()->normal();
	wreg->getvalE()->normal();
	wreg->getvalM()->normal();
	wreg->getdstE()->normal();
	wreg->getdstM()->normal();

}

/*
 * setWInput
 * Method to streamline the process of feeding values into the Writeback stage preg. 
 *
 */
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM)
{
	wreg->getstat()->setInput(stat);
	wreg->geticode()->setInput(icode);
	wreg->getvalE()->setInput(valE);
	wreg->getvalM()->setInput(valM);
	wreg->getdstE()->setInput(dstE);
	wreg->getdstM()->setInput(dstM);
}


/*
 * addr()
 * Determine which value to use as the address for our next read or write from/to Memory.  
 *
 */
uint64_t MemoryStage::addr(uint64_t icode, uint64_t M_valE, uint64_t M_valA)
{
	if (icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL || icode == IMRMOVQ)
	{
		return M_valE;
	}
	if (icode == IPOPQ || icode == IRET)
	{
		return M_valA;
	}
	return 0;
}

/*
 * memRead()
 * This method determines if we need to read from Memory, uses icode. 
 *
 */
bool MemoryStage::memRead(uint64_t icode)
{
	return (icode == IMRMOVQ || icode == IPOPQ || icode == IRET);
}

/*
 * memWrite()
 * This method determines if we need to write to Memory, uses icode.
 *
 */
bool MemoryStage::memWrite(uint64_t icode)
{
	return (icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL);
}

/*
 * get_valM()
 * This method returns valM. 
 *
 */
uint64_t MemoryStage::get_valM()
{
	//return 0;
	return valM;
}

/*
 * get_mstat()
 * This method returns mstat.
 *
 */
uint64_t MemoryStage::get_mstat()
{
	return stat;
}
