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
#include "Instructions.h"
#include "Stage.h"
#include "WritebackStage.h"
#include "Status.h"
#include "Debug.h"



/*
 * doClockLow
 * When the clock ticks low, we check our stat to see if we need to stop. 
 *
 *
 */
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
	W* wreg = (W*) pregs[WREG];
	uint64_t stat = wreg->getstat()->getOutput();
	if (stat == SAOK)
	{
		return false;
	}
	
	return true;
}

/*
 * doClockHigh
 * When the clock ticks low, we write to our Registers from DecodeStage
 *
 *
 */
void WritebackStage::doClockHigh(PipeReg ** pregs)
{
	bool error = 0;
	W* wreg = (W*) pregs[WREG];
	RegisterFile * rf = RegisterFile::getInstance();
	uint64_t valE = wreg->getvalE()->getOutput();
	uint64_t valM = wreg->getvalM()->getOutput();
	uint64_t dstM = wreg->getdstM()->getOutput();
	uint64_t dstE = wreg->getdstE()->getOutput();
	rf->writeRegister(valE, dstE, error);
   	rf->writeRegister(valM, dstM, error);
	

}
