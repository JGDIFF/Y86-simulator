CC = g++
CFLAGS = -g -c -Wall -std=c++11 -O0
OBJ = yess.o ConditionCodes.o Loader.o Memory.o RegisterFile.o Tools.o\
	FetchStage.o DecodeStage.o ExecuteStage.o MemoryStage.o WritebackStage.o\
	Simulate.o F.o D.o E.o M.o W.o PipeReg.o PipeRegField.o


.C.o:
	$(CC) $(CFLAGS) $< -o $@

yess: $(OBJ)
Memory.o: Memory.h Tools.h
RegisterFile.o: RegisterFile.h Tools.h
ConditionCodes.o: ConditionCodes.h Tools.h
Tools.o: Tools.h
Loader.o: Loader.h Memory.h 

FetchStage.o: FetchStage.h RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h E.h Instructions.h Memory.h Tools.h DecodeStage.h ExecuteStage.h

DecodeStage.o: DecodeStage.h RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h E.h Instructions.h Memory.h Tools.h ExecuteStage.h MemoryStage.h
 
ExecuteStage.o: ExecuteStage.h RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h E.h Instructions.h Memory.h Tools.h MemoryStage.h WritebackStage.h

MemoryStage.o: MemoryStage.h RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h E.h Instructions.h Memory.h Tools.h 

WritebackStage.o: WritebackStage.h RegisterFile.h PipeRegField.h PipeReg.h F.h D.h M.h E.h Instructions.h Memory.h Tools.h MemoryStage.h

Simulate.o: Simulate.h RegisterFile.h Memory.h PipeReg.h PipeRegField.h Status.h FetchStage.h DecodeStage.h ExecuteStage.h MemoryStage.h \
WritebackStage.h Memory.h 

F.o: F.h RegisterFile.h PipeReg.h PipeRegField.h Status.h
D.o: D.h RegisterFile.h PipeReg.h PipeRegField.h Status.h
E.o: E.h RegisterFile.h PipeReg.h PipeRegField.h Status.h
M.o: M.h RegisterFile.h PipeReg.h PipeRegField.h Status.h
W.o: W.h RegisterFile.h PipeReg.h PipeRegField.h Status.h

PipeReg.o: PipeReg.h

PipeRegField.o: PipeRegField.h






clean:
	rm $(OBJ) yess

run:
	make clean
	make yess
	./run.sh

