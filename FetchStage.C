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
#include "E.h"
#include "Stage.h"
#include "FetchStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"
#include "ExecuteStage.h"
#include "DecodeStage.h"


/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   	F * freg = (F *) pregs[FREG];
   	D * dreg = (D *) pregs[DREG];
   	M * mreg = (M *) pregs[MREG];
   	W * wreg = (W *) pregs[WREG];
   	uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
   	uint64_t rA = RNONE, rB = RNONE, stat = SAOK;

   	f_pc = selectPC(freg, mreg, wreg);
   	Memory * mem = Memory::getInstance();
   	bool error = false;
   	uint64_t byte = mem->getByte(f_pc, error);
   	ifun  = Tools::getBits(byte, 0, 3);
   	icode = Tools::getBits(byte, 4, 7);
   	if (error)
   	{    
        	icode = INOP;
        	ifun = FNONE;
   	}
   	bool regIdBool = need_regids(icode);
   	if (regIdBool)
   	{
	   	getRegIds(f_pc, icode, rA, rB);
   	}
   	bool valCBool = need_valC(icode); 	
   	if (valCBool)
   	{
			valC = buildValC(f_pc, regIdBool);
   	}
   	bool instr_validBool = instr_valid(icode);
   	stat = f_stat(error, instr_validBool, icode);
   	valP = PCIncrement(f_pc, regIdBool, valCBool);
   	uint64_t pcResult = predictPC(icode, valC, valP);

   	freg->getpredPC()->setInput(pcResult);

   	calculateControlSignals(pregs, stages);
   
   	//Input passed to next stage.
   	setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
   	return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
   	F * freg = (F *) pregs[FREG];
   	if (!F_stall)
   	{
        	freg->getpredPC()->normal();
   	}
   	if (D_bubble)
   	{
       		bubbleD(pregs);
   	}
   	else if (!D_stall)
   	{
        	normalD(pregs);
   	}	
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
*/
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t rA, uint64_t rB,
                           uint64_t valC, uint64_t valP)
{
   	dreg->getstat()->setInput(stat);
   	dreg->geticode()->setInput(icode);
   	dreg->getifun()->setInput(ifun);
   	dreg->getrA()->setInput(rA);
   	dreg->getrB()->setInput(rB);
   	dreg->getvalC()->setInput(valC);
   	dreg->getvalP()->setInput(valP);
}

/* selectPC
 * Determines/returns the value for f_pc in doClockLow based on the icodes
 * stored in the memory and writeback registers.
 *
 *
*/
uint64_t FetchStage::selectPC(F* freg, M* mreg, W* wreg) 
{
	if (mreg->geticode()->getOutput() == IJXX && (!mreg->getCnd()->getOutput()))
	{
		return mreg->getvalA()->getOutput();
	}

	if (wreg ->geticode()->getOutput() == IRET)
	{
		return wreg->getvalM()->getOutput(); 
	}   	 
	else
        	return freg->getpredPC()->getOutput(); 
}

/* need_regids
 * Determines whether or not a specific instruction requires register values to be
 * pulled from said instruction based on the icodes retrieved earlier in doClockLow.
 *
 *
*/
bool FetchStage::need_regids(uint64_t f_icode) 
{
	if (f_icode == IRRMOVQ || f_icode == IOPQ || f_icode == IPUSHQ || f_icode == IPOPQ || 
        f_icode == IIRMOVQ || f_icode == IRMMOVQ || f_icode == IMRMOVQ)
	{
		return true;
	}
	return false;
}

/* need_valC 
 * Determines whether or not a specific instruction requires a ValC to be
 * pulled from said instruction based on the icodes retrieved earlier in
 * doClockLow.
 *
 *
*/
bool FetchStage::need_valC(uint64_t f_icode) 
{
	if (f_icode == IIRMOVQ || f_icode == IRMMOVQ || f_icode == IMRMOVQ || 
        f_icode == IJXX || f_icode == ICALL)
	{
		return true;
	}
	return false;
}

uint64_t FetchStage::PCIncrement(uint64_t f_pc, bool needRegIds, bool needValC)
{
	if (needRegIds && needValC)
	{
		return f_pc + 10;
	}

	else if (!needRegIds && needValC)
	{
		return f_pc + 9;	
	}

	else if (needRegIds && !needValC)
	{
		return f_pc + 2;
	}

	else {
		return f_pc + 1;
	}
}

uint64_t FetchStage::predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP) 
{
	if (f_icode == IJXX || f_icode == ICALL)
	{
		return f_valC;
	}
	return f_valP;

}

void FetchStage::getRegIds(uint64_t f_pc, uint64_t f_icode, uint64_t &rA, uint64_t &rB)
{
	if (need_regids(f_icode))
	{
		bool error = false;
		Memory* mem = Memory::getInstance();
		uint8_t byte = mem->getByte((f_pc + 1), error);
		rA = Tools::getBits(byte, 4, 7);
		rB = Tools::getBits(byte, 0, 3);
	}
}



uint64_t FetchStage::buildValC(uint64_t f_pc, bool regIdBool)
{
	bool error = false; 
	Memory* mem = Memory::getInstance();
	int pcCounter = 0;
	uint8_t bytes[LONGSIZE];
	if (regIdBool)
	{
			pcCounter = 2;
			for (int i = 0; i < 8; i++)
			{
				bytes[i] = mem->getByte((f_pc + pcCounter), error);
				pcCounter += 1;
			}
	}
	else {
			pcCounter = 1; 
			for (int i = 0; i < 8; i++)
			{
				bytes[i] = mem->getByte(f_pc + pcCounter, error);
				pcCounter += 1;	
			}
	}
	
	return Tools::buildLong(bytes);
}



bool FetchStage::instr_valid(uint64_t icode)
{
    	return (icode == INOP || icode == IHALT || icode == IRRMOVQ || icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ 
    	 || icode == IOPQ || icode == IJXX || icode == ICALL || icode == IRET || icode == IPUSHQ || icode == IPOPQ);
}

uint64_t FetchStage::f_stat(bool mem_error, bool instr_valid, uint64_t icode)
{
    	if (mem_error)
    	{
    		return SADR;
    	} 
    	else if (!instr_valid)
    	{
    		return SINS;
    	} 
    	else if (icode == IHALT) 
    	{
    		return SHLT;
    	}
    	else {
		return SAOK;
	}	
}

bool FetchStage::setDBubble(uint64_t E_icode, uint64_t e_Cnd, uint64_t d_srcA, uint64_t d_srcB, uint64_t E_dstM, uint64_t D_icode, uint64_t M_icode)
{
    	return ((E_icode == IJXX && !e_Cnd)) || (!((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB)) &&
        	(D_icode == IRET || E_icode == IRET || M_icode == IRET));
}

bool FetchStage::setFStall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB, uint64_t D_icode, uint64_t M_icode)
{
    	return ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB)) || 
        	(D_icode == IRET || E_icode == IRET || M_icode == IRET);
}

bool FetchStage::setDStall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB)
{
	return ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB));
}

void FetchStage::calculateControlSignals(PipeReg ** pregs, Stage ** stages)
{

	ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];
	DecodeStage * ds = (DecodeStage*) stages[DSTAGE];
   	E * ereg = (E *) pregs[EREG];
   	M * mreg = (M *) pregs[MREG];
   	D * dreg = (D *) pregs[DREG];

   	uint64_t E_icode = ereg->geticode()->getOutput();
   	uint64_t E_dstM = ereg->getdstM()->getOutput();
   	uint64_t d_srcA = ds->getd_srcA();
   	uint64_t d_srcB = ds->getd_srcB();
   	uint64_t e_Cnd = es->gete_Cnd();
   	uint64_t D_icode = dreg->geticode()->getOutput();
   	uint64_t M_icode = mreg->geticode()->getOutput();

  	D_bubble = setDBubble(E_icode, e_Cnd, d_srcA, d_srcB, E_dstM, D_icode, M_icode);
   	F_stall = setFStall(E_icode, E_dstM, d_srcA, d_srcB, D_icode, M_icode);
   	D_stall = setDStall(E_icode, E_dstM, d_srcA, d_srcB);
}


void FetchStage::normalD(PipeReg ** pregs)
{
	D * dreg = (D *) pregs[DREG]; 
	dreg->getstat()->normal();
   	dreg->geticode()->normal();
   	dreg->getifun()->normal();
   	dreg->getrA()->normal();
   	dreg->getrB()->normal();
   	dreg->getvalC()->normal();
   	dreg->getvalP()->normal();
}

void FetchStage::bubbleD(PipeReg ** pregs)
{
	D * dreg = (D *) pregs[DREG];
	dreg->getstat()->bubble(SAOK);
   	dreg->geticode()->bubble(INOP);
   	dreg->getifun()->bubble();
   	dreg->getrA()->bubble(RNONE);
   	dreg->getrB()->bubble(RNONE);
   	dreg->getvalC()->bubble();
   	dreg->getvalP()->bubble();
}