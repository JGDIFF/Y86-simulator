#include <string>
#include <cstdint>
#include <iostream>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "E.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"


/*
 * doClockLow
 * When the clock tick is low, this is what is performed, 
 * values are grabbed from the FetchStage and decoded, registers are read and our Control Signals are calculated.
 *
 */
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    	//Initialize the values from FetchStage
    	D * dreg = (D *) pregs[DREG];
    	E * ereg = (E *) pregs[EREG];
       
    	ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];

    	uint64_t stat  = dreg->getstat()->getOutput();
    	uint64_t icode = dreg->geticode()->getOutput();
    	uint64_t ifun  = dreg->getifun()->getOutput();
    	uint64_t valC  = dreg->getvalC()->getOutput();
    	uint64_t rA    = dreg->getrA()->getOutput();
    	uint64_t rB    = dreg->getrB()->getOutput();
    	uint64_t valP  = dreg->getvalP()->getOutput();

    	//Placeholder 0/RNONE before values are calculated.
    	uint64_t valA = 0;
    	uint64_t valB = 0;
    	uint64_t rvalA = 0;
    	uint64_t rvalB = 0;

    	uint64_t dstE = RNONE; 
    	uint64_t dstM = RNONE;

    	d_srcA = RNONE;
    	d_srcB = RNONE;

    	//Use components to get values 
    	d_srcA = d_srcAComponent(icode, rA);
    	d_srcB = d_srcBComponent(icode, rB);

    	dstE = dstEComponent(icode, rB);
    	dstM = dstMComponent(icode, rA);

    	uint64_t E_icode = ereg->geticode()->getOutput();
    	uint64_t E_dstM = ereg->getdstM()->getOutput();
    	uint64_t e_Cnd = es->gete_Cnd();	
	E_bubble = calculateControlSignals(E_icode, E_dstM, d_srcA, d_srcB, e_Cnd);

	RegisterFile * rf = RegisterFile::getInstance();
	bool error = false;
	rvalA = rf->readRegister(d_srcA, error);
	rvalB = rf->readRegister(d_srcB, error);

	valA = selPlusFwdA(rvalA, d_srcA, icode, valP, stages, pregs);
	valB = FwdB(rvalB, d_srcB, stages, pregs);


	//Pass input to next stage.
    	setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM, d_srcA, d_srcB);
    	return false;
}

/*
 * doClockHigh
 * When the clock ticks high, we inspect the results of our CalculateControlSignals(),
 * to determine if we need to Bubble or Normal the E register and its values.
 *
 */
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
	if (E_bubble)
	{
		bubbleE(pregs);
	}
	else
	{
		normalE(pregs);
	}
	
}


/*
 * setEInput
 * Method to streamline passing values to the E register at the end of doClockLow()
 * 
 *
 */
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t valC, uint64_t valA,
                           uint64_t valB, uint64_t dstE, uint64_t dstM,
						   uint64_t srcA, uint64_t srcB)
{
	ereg->getstat()->setInput(stat);
	ereg->geticode()->setInput(icode);
	ereg->getifun()->setInput(ifun);
	ereg->getvalC()->setInput(valC);
	ereg->getvalA()->setInput(valA);
	ereg->getvalB()->setInput(valB);
	ereg->getdstE()->setInput(dstE);
	ereg->getdstM()->setInput(dstM);
	ereg->getsrcA()->setInput(srcA);
	ereg->getsrcB()->setInput(srcB);
}


/*
 * d_srcAComponent
 * Calcuates our needed value for rA based on the passed icode
 * 
 *
 */
uint64_t DecodeStage::d_srcAComponent(uint64_t icode, uint64_t rA)
{
	if (icode == IRRMOVQ || icode == IRMMOVQ || icode == IOPQ || icode == IPUSHQ) 
	{
		return rA;
	}

	if (icode == IPOPQ || icode == IRET) 
	{
		return RSP;
	}

	return RNONE;
}

/*
 * d_srcBComponent
 * Calcuates our needed value for rB based on the passed icode
 * 
 *
 */
uint64_t DecodeStage::d_srcBComponent(uint64_t icode, uint64_t rB)
{
	if (icode == IOPQ || icode == IRMMOVQ || icode == IMRMOVQ)
	{ 
		return rB;
	}

	if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET) 
	{
		return RSP;
	}

	return RNONE;
}

/*
 * dstEComponent
 * Calculates our needed value for dstE based on the passed icode. 
 * 
 *
 */
uint64_t DecodeStage::dstEComponent(uint64_t icode, uint64_t rB)
{
	if (icode == IRRMOVQ || icode == IIRMOVQ || icode == IOPQ)
	{
		return rB;
	}
	if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET)
	{
		return RSP;
	}
	return RNONE;
}

/*
 * dstMComponent
 * Calculates our needed value for dstM based on the passed icode. 
 * 
 *
 */
uint64_t DecodeStage::dstMComponent(uint64_t icode, uint64_t rA)
{
	if (icode == IMRMOVQ || icode == IPOPQ)
	{
		return rA;
	}

	return RNONE;
}


/*
 * selPlusFwdA
 * Takes in a ton of parameters in order to implement the forwarding of srcA
 * 
 *
 */
uint64_t DecodeStage::selPlusFwdA(uint64_t rvalA, uint64_t srcA, uint64_t icode, uint64_t valP, Stage ** stages, PipeReg ** pregs)
{
	M * mreg = (M *) pregs[MREG];
	W * wreg = (W *) pregs[WREG];
	MemoryStage * ms = (MemoryStage *) stages[MSTAGE];
	ExecuteStage * es = (ExecuteStage *) stages[ESTAGE];
	
	if (icode == ICALL || icode == IJXX)
	{
		return valP;
	}
	
	if (srcA == RNONE)
	{
		return 0;
	}

	
	if (srcA == es->get_dstE())
	{
		return es->get_valE();
	}
	
	if (srcA == mreg->getdstM()->getOutput())
	{
		return ms->get_valM();
	}
	
	
	if (srcA == mreg->getdstE()->getOutput())
	{
		return mreg->getvalE()->getOutput();
	}
	
	if (srcA == wreg->getdstM()->getOutput())
	{
		return wreg->getvalM()->getOutput();
	}

	if (srcA == wreg->getdstE()->getOutput())
	{

		return wreg->getvalE()->getOutput();
	}
	return rvalA;

}

/*
 * FwdB 
 * Forwards the value of srcB 
 * 
 *
 */
uint64_t DecodeStage::FwdB(uint64_t rvalB, uint64_t srcB, Stage ** stages, PipeReg ** pregs)
{
	M * mreg = (M *) pregs[MREG];
	W * wreg = (W *) pregs[WREG];
	MemoryStage * ms = (MemoryStage *) stages[MSTAGE];
	ExecuteStage * es = (ExecuteStage *) stages[ESTAGE];


	if (srcB == RNONE)
	{
		return 0;
	}
	if (srcB == es->get_dstE())
	{
		return es->get_valE();
	}
	
	if (srcB == mreg->getdstM()->getOutput())
	{
		return ms->get_valM();
	}

	if (srcB == mreg->getdstE()->getOutput())
	{
		return mreg->getvalE()->getOutput();
	}
	
	if (srcB == wreg->getdstM()->getOutput())
	{
		return wreg->getvalM()->getOutput();
	}

	if (srcB == wreg->getdstE()->getOutput())
	{
		return wreg->getvalE()->getOutput();
	}
	return rvalB;
}


/*
 * CalculateControlSignals
 * Implements the logic of determining if we need to Bubble or Stall
 *  
 *
 */
bool DecodeStage::calculateControlSignals(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB, uint64_t e_Cnd)
{
        return (E_icode == IJXX && !e_Cnd) || ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB));  
}

/*
 * bubbleE
 * Bubbles the ExecuteStage reg values
 * 
 *
 */
void DecodeStage::bubbleE(PipeReg ** pregs)
{
	E * ereg = (E *) pregs[EREG];
    	ereg->getstat()->bubble(SAOK);
    	ereg->geticode()->bubble(INOP);
    	ereg->getifun()->bubble();
    	ereg->getvalC()->bubble();
    	ereg->getvalA()->bubble();
    	ereg->getvalB()->bubble();
    	ereg->getdstE()->bubble(RNONE);
    	ereg->getdstM()->bubble(RNONE);
    	ereg->getsrcA()->bubble(RNONE);
    	ereg->getsrcB()->bubble(RNONE);
}

/*
 * normalE
 * Normals the ExecuteStage reg values
 * 
 *
 */
void DecodeStage::normalE(PipeReg ** pregs)
{
    	E * ereg = (E *) pregs[EREG];
    	ereg->getstat()->normal();
    	ereg->geticode()->normal();
    	ereg->getifun()->normal();
    	ereg->getvalC()->normal();
    	ereg->getvalA()->normal();
    	ereg->getvalB()->normal();
    	ereg->getdstE()->normal();
    	ereg->getdstM()->normal();
    	ereg->getsrcA()->normal();
    	ereg->getsrcB()->normal();

}

/*
 * getd_srcA
 * Returns d_srcA
 * 
 *
 */
uint64_t DecodeStage::getd_srcA()
{
    	return d_srcA;
}

/*
 * getd_srcB
 * Returns d_srcB
 * 
 *
 */
uint64_t DecodeStage::getd_srcB()
{
    	return d_srcB;
}
