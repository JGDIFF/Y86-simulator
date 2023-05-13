#include <string>
#include <cstdint>
#include <iostream>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"
#include "WritebackStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ConditionCodes.h"
#include "Tools.h"



/*
 * doClockLow
 * When the clock tick is low, this is where most of the logic is performed.
 * Values are grabbed, calculated, updated and then passed to the MemoryStage.
 *
 */
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
	E * ereg = (E *) pregs[EREG];
	M * mreg = (M *) pregs[MREG];
	
	
	uint64_t stat = ereg->getstat()->getOutput();
	uint64_t icode = ereg->geticode()->getOutput();
	uint64_t ifun = ereg->getifun()->getOutput();
	uint64_t valC = ereg->getvalC()->getOutput();
	e_Cnd = 0;
	valE = 0;
	uint64_t valA = ereg->getvalA()->getOutput();
	uint64_t valB = ereg->getvalB()->getOutput();
	dstE = ereg->getdstE()->getOutput();
	uint64_t dstM = ereg->getdstM()->getOutput();
	
	

	uint64_t aluAvar = 0;
	uint64_t alufunvar = aluFun(icode, ifun);
	aluAvar = aluA(icode, valA, valC);
	valB = aluB(icode, valB);

	bool  of;
	bool  sf;
	bool  zf;
	valE = ALUCompute(alufunvar, aluAvar, valB, of);
	if (valE == 0)
	{
		zf = 1;
	}
	else {
		zf = 0;
	}
	if (Tools::sign(valE) == 1)
	{		sf = 1;
	}
	else {
		sf = 0;
	}
	MemoryStage * ms = (MemoryStage*) stages[MSTAGE];
   	 W * wreg = (W *) pregs[WREG];

   	uint64_t E_icode = ereg->geticode()->getOutput();
    	uint64_t m_stat = ms->get_mstat();
    	uint64_t W_stat = wreg->getstat()->getOutput();
    	bool ccBool = set_cc(E_icode, m_stat, W_stat);

	if (ccBool)
	{
		ConditionCodes * cc = ConditionCodes::getInstance();
		bool ccerror = false;
		cc->setConditionCode(of, OF, ccerror);
		cc->setConditionCode(sf, SF, ccerror);
		cc->setConditionCode(zf, ZF, ccerror);
	}

	M_bubble = calculateControlSignals(m_stat, W_stat);

	e_Cnd = cond(icode, ifun);
	dstE = dstEComponent(icode, e_Cnd, dstE);


	setMInput(mreg, stat, icode, valA, dstE, dstM, e_Cnd, valE);
	return false;
	
}

/*
 * doClockHigh
 * When the clock ticks high, we check our ControlSignals to determine
 * .. if we need to normalM or bubbleM the M register.
 *
 */
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
    	if (!M_bubble)
    	{
        	normalM(pregs);
    	}
    	else
    	{
        	bubbleM(pregs);
    	}

}


/*
 * setMInput
 * Method to streamline passing values to the M register at the end of doClockLow().
 * 
 *
 */
void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode,
							uint64_t valA, uint64_t dstE, uint64_t dstM,
							uint64_t Cnd, uint64_t valE)
{
	mreg->getstat()->setInput(stat);
	mreg->geticode()->setInput(icode);
	mreg->getvalA()->setInput(valA);
	mreg->getdstE()->setInput(dstE);
	mreg->getdstM()->setInput(dstM);
	mreg->getCnd()->setInput(Cnd);
	mreg->getvalE()->setInput(valE);
	
}

/*
 * get_dstE()
 * returns the dstE value.
 * 
 *
 */
uint64_t ExecuteStage::get_dstE()
{
    	return dstE;
}

/*
 * get_valE()
 * returns the valE value.
 * 
 *
 */
uint64_t ExecuteStage::get_valE()
{
    	return valE;
}

/*
 * gete_Cnd()
 * returns the e_Cnd value.
 * 
 *
 */
uint64_t ExecuteStage::gete_Cnd()
{
    	return e_Cnd;
}

/*
 * aluA
 * Computes the value needed for ALUA to go into the ALU, works off of valA and valC.
 * 
 *
 */
uint64_t ExecuteStage::aluA(uint64_t icode, uint64_t valA, uint64_t valC)
{
	if (icode == IRRMOVQ || icode == IOPQ)
	{
		return valA;
	}
	if (icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ)
	{
		return valC;
	}
	if (icode == ICALL || icode == IPUSHQ)
	{
		return -8;
	}
	if (icode == IRET || icode == IPOPQ)
	{
		return 8;
	}
	return 0;
}

/*
 * aluB
 * Computes the value needed for ALUB to go into the ALU, works off of valB only. 
 * 
 *
 */
uint64_t ExecuteStage::aluB(uint64_t icode, uint64_t valB)
{
	if (icode == IRMMOVQ || icode == IMRMOVQ || icode == IOPQ || icode == ICALL || icode == IPUSHQ 
		|| icode == IRET || icode == IPOPQ)
	{
			return valB;
	}

	if (icode == IRRMOVQ || icode == IIRMOVQ)
	{
		return 0;
	}
	return 0;
}

/*
 * aluFun
 * Determines if we are doing an insturction that needs the ifun value, like an OPQ.
 * 
 *
 */
uint64_t ExecuteStage::aluFun(uint64_t icode, uint64_t ifun)
{
	if (icode == IOPQ)
	{
		return ifun;
	}
	else {
		return ADDQ;
	}
}


/*
 * set_cc
 * This method checks our icode to determine if we need condition codes, it also checks m_stat and W_stat.
 * 
 *
 */
bool ExecuteStage::set_cc(uint64_t icode, uint64_t m_stat, uint64_t W_stat)
{
	return (icode == IOPQ) && (m_stat != SADR && m_stat != SINS && m_stat != SHLT)
    	&& (W_stat != SADR && W_stat != SINS && W_stat != SHLT);
}

/*
 * dstEComponent
 * This method determinnes our value for dstE, it looks at our Cnd value. 
 * 
 *
 */
uint64_t ExecuteStage::dstEComponent(uint64_t icode, uint64_t Cnd, uint64_t dstE)
{
	if (icode == IRRMOVQ && (!Cnd))
		return RNONE;
	else
		return dstE;
}

/*
 * cond
 * This method is where we determine how our conditions codes will be used for our jump/jxx conditions, based off an HCL
 * 
 *
 */
uint64_t ExecuteStage::cond(uint64_t icode, uint64_t ifun)
{
	bool error = false;
	ConditionCodes * cc = ConditionCodes::getInstance();
	bool of = cc->getConditionCode(OF, error);
	bool sf = cc->getConditionCode(SF, error);
	bool zf = cc->getConditionCode(ZF, error);
	if (icode == IJXX || icode == ICMOVXX)
	{
		if (ifun == UNCOND)
		{
			return 1;
		}
		else if (ifun == LESSEQ)
		{
			return ((sf ^ of) | zf);
		}
		else if (ifun == LESS)
		{
			return ((sf ^ of));
		}
		else if (ifun == EQUAL)
		{
			return ((zf));
		}
		else if (ifun == NOTEQUAL)
		{
			return ((!zf));
		}
		else if (ifun == GREATER)
		{
			return (!(sf ^ of) & !zf);
		}
		else if (ifun == GREATEREQ)
		{
			return (!(sf ^ of));
		}
	}
	return 0;
}

/*
 * ALUCompute
 * This method is the actual brains of the ALU, which boils down to four basic operations,
 * we also pass in a reference to our of bool to set our overflow condition code. 
 *
 */
uint64_t ExecuteStage::ALUCompute(uint64_t aluFun, uint64_t aluA, uint64_t aluB, bool &of)
{
	if (aluFun == ADDQ)
	{
		of = Tools::addOverflow(aluA, aluB);			
		return aluA + aluB;
	}

	else if (aluFun == SUBQ)
	{	of = Tools::subOverflow(aluA, aluB);
		return aluB - aluA;
	}

	else if (aluFun == XORQ)
	{
		return aluA ^ aluB;
	}

	else if (aluFun == ANDQ)
	{
		return aluA & aluB;
	}
	else return 0;
}

/*
 * normalM
 * Applies the normal values to the M reg.
 * 
 *
 */
void ExecuteStage::normalM(PipeReg ** pregs)
{
    	M * mreg = (M *) pregs[MREG];
    	mreg->getstat()->normal();
    	mreg->geticode()->normal();
    	mreg->getCnd()->normal();
    	mreg->getvalE()->normal();
    	mreg->getvalA()->normal();
    	mreg->getdstE()->normal();
    	mreg->getdstM()->normal();
}

/*
 * bubbleM
 * Bubbles the M register values. 
 * 
 *
 */
void ExecuteStage::bubbleM(PipeReg ** pregs)
{
    	M * mreg = (M *) pregs[MREG];
    	mreg->getstat()->bubble(SAOK);
    	mreg->geticode()->bubble(INOP);
    	mreg->getCnd()->bubble();
    	mreg->getvalE()->bubble();
    	mreg->getvalA()->bubble();
    	mreg->getdstE()->bubble(RNONE);
    	mreg->getdstM()->bubble(RNONE);
}

/*
 * CalculateControlSignals
 * Determines if we need bubble or normal the M reg, used in doClockHigh().
 * 
 *
 */
bool ExecuteStage::calculateControlSignals(uint64_t m_stat, uint64_t W_stat)
{
    	return (m_stat == SADR || m_stat == SINS || m_stat == SHLT) ||
           (W_stat == SADR || W_stat == SINS || W_stat == SHLT);
}





















