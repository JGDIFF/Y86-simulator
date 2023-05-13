//class to execute instructions
//Execute Stage
class ExecuteStage: public Stage
{

	private:
		uint64_t dstE;
		uint64_t valE;
		uint64_t e_Cnd;
		bool M_bubble;
	public:
		bool doClockLow(PipeReg ** pregs, Stage ** stages);
		void doClockHigh(PipeReg ** pregs);
		//void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode,
		//					uint64_t ifun, uint64_t valC, uint64_t valA,
		//					uint64_t valB, uint64_t dstE, uint64_t dstM,
		//					uint64_t srcA, uint64_t srcB, uint64_t Cnd,
		//					uint64_t valE);
		void setMInput(M * mreg, uint64_t stat, uint64_t icode,
							uint64_t valA, uint64_t dstE, uint64_t dstM,
							uint64_t Cnd, uint64_t valE);
		uint64_t aluA(uint64_t icode, uint64_t valA, uint64_t valC);
		uint64_t aluB(uint64_t icode, uint64_t valB);
		uint64_t aluFun(uint64_t icode, uint64_t ifun);
		bool set_cc(uint64_t icode, uint64_t m_stat, uint64_t W_stat);
		uint64_t dstEComponent(uint64_t icode, uint64_t Cnd, uint64_t dstE);
		//uint64_t ccLogic(bool &of, bool &sf, bool &zf);
		uint64_t ALUCompute(uint64_t aluFun, uint64_t aluA, uint64_t aluB, bool &of);
		uint64_t get_dstE();
		uint64_t get_valE();
		uint64_t gete_Cnd();
		uint64_t cond(uint64_t icode, uint64_t ifun);
		bool calculateControlSignals(uint64_t m_stat, uint64_t W_stat);
		void normalM(PipeReg ** pregs);
		void bubbleM(PipeReg ** pregs);
};

