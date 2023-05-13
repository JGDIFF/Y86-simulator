//class to perform the combinational logic of
//the Fetch stage
class FetchStage: public Stage
{
   private:

   	bool D_stall;
   	bool F_stall;
   	bool D_bubble;
	
     	void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t rA, uint64_t rB,
                     uint64_t valC, uint64_t valP);
		uint64_t selectPC(F* freg, M* mreg, W* wreg);
		bool need_regids(uint64_t f_icode);
		bool need_valC(uint64_t f_icode);
		uint64_t predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP);
		void getRegIds(uint64_t f_pc, uint64_t f_icode, uint64_t &rA, uint64_t &rB);
		uint64_t buildValC(uint64_t f_pc, bool regIdBool);
		uint64_t PCIncrement(uint64_t f_pc, bool needRegIds, bool needValC);
		uint64_t f_stat(bool mem_error, bool instr_valid, uint64_t icode);
		bool instr_valid(uint64_t icode);
		bool setFStall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB, uint64_t D_icode, uint64_t M_icode);
		bool setDStall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
		bool setDBubble(uint64_t E_icode, uint64_t e_Cnd, uint64_t d_srcA, uint64_t d_srcB, uint64_t E_dstM, uint64_t D_icode, uint64_t M_icode);
		void calculateControlSignals(PipeReg ** pregs, Stage ** stages);
		void bubbleD(PipeReg ** pregs);
		void normalD(PipeReg ** pregs);
	

	
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
