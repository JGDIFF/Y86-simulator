//class to Memory instructions
//Memory Stage
class MemoryStage: public Stage
{
	private:
		uint64_t valM;
		uint64_t stat;
	public:
		bool doClockLow(PipeReg ** pregs, Stage ** stages);
		void doClockHigh(PipeReg ** pregs);
		//void setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM);
		void setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM);
		bool memRead(uint64_t icode);
	    bool memWrite(uint64_t icode);
	    uint64_t addr(uint64_t icode, uint64_t M_valE, uint64_t M_valA);
		uint64_t get_valM();
		uint64_t get_mstat();
};
