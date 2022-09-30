#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <ctype.h>

#define ioffset 0x00400000
#define doffset 0x10000000
#define memsize 1024

#define getop(inst) (((int)(inst) >> 26) & 0x3F)
#define getrs(inst) (((int)(inst) >> 21) & 0x1F)
#define getrt(inst) (((int)(inst) >> 16) & 0x1F)
#define getrd(inst) (((int)(inst) >> 11) & 0x1F)
#define getsh(inst) (((int)(inst) >> 6) & 0x1F)
#define getfu(inst) (((int)(inst) >> 0) & 0x3F)
#define getim(inst) (((int)(inst) >> 0) & 0xFFFF)
#define getjp(inst) (((int)(inst) >> 0) & 0x3FFFFFF)


//#define DEBUG0
//flags

int atp; // 1 for Always taken, 0 for Always not taken

int m; // memory
int m1;
int m2;

int d; // print every cycle
int p; // PC print
int n; // number of instruction to be fetched
int cc; // completion of cycle
/*
        StringControl Functions

        Delenter   : Delete enter in last
        Makenumber : convert hex string to int
        SignedExntended_Byte : Sign Extension to byte
        SignedExtend_Half    : Sign Extension to Half

*/


void Delenter(char *str)
{
    char enter = '\n';
    for(; *str != '\0'; str++){
        if (*str == enter){
            strcpy(str, str+1);
            str--;
        }
    }
}

int Makenumber(char *str){
    //number
    return strtol(str,NULL,16);
}

unsigned int SignedExtend_Byte(unsigned int numberbyte){
    unsigned int result;
    result = numberbyte;
    if(((0x80)&numberbyte)){
        result = numberbyte + 0xffffff00;
    }
    return result;
}
unsigned int SignedExtend_Half(unsigned int numberhalf){
    unsigned int result;
    result = numberhalf;
    if(((0x8000)&numberhalf)){
        result = numberhalf + 0xffff0000;
    }
    return result;
}

/*

        Memory

        Struct:

        memory[memsize] : memory structure

        Function:

        FindMemory 
        ReadMemory
        WriteMemory
        ReadByteMemory
        WriteByteMemory

        Variables:

        memoryoffset


*/

int memoryoffset;
struct memorystruct{
    int address;
    unsigned int data;
}memory[memsize];

int FindMemory(int cMemAddr){
    int i;
    for (i=0; i<memsize; i++){
        if(memory[i].address == cMemAddr){
            return i;
        }
    }
    return -1;
}

unsigned int ReadMemory(int cMemAddr){
    int i;
    for (i=0; i<memsize; i++){
        if(memory[i].address == cMemAddr){
            return memory[i].data;
        }
    }
    return 0;
}

void WriteMemory(int cMemAddr, unsigned int MemData){
    int i = FindMemory(cMemAddr);
    if(i){
        memory[i].data = MemData;
    }
    else{
        memory[memoryoffset].address = cMemAddr;
        memory[memoryoffset].data = MemData;
        memoryoffset++;
    }
}

unsigned int ReadByteMemory(int cMemAddr){
    int cMemAddr_qu = (cMemAddr>>2)<<2;
    int cMemAddr_re = cMemAddr-cMemAddr_qu;
    int i;
    for (i=0; i<memsize; i++){
        if(memory[i].address == cMemAddr_qu){
            unsigned int fullnumber = memory[i].data;
            unsigned int result;
            //printf("Readed full number 0x%x\n",fullnumber);
            if(cMemAddr_re==0){
                result = fullnumber>>24;
            }
            if(cMemAddr_re==1){
                result = (fullnumber<<8)>>24;
            }
            if(cMemAddr_re==2){
                result = (fullnumber<<16)>>24;
            }
            if(cMemAddr_re==3){
                result = (fullnumber<<24)>>24;
            }
            return SignedExtend_Byte(result);
        }
    }
    return 0;
}

void WriteByteMemory(int cMemAddr, unsigned int MemData){
    int cMemAddr_qu = (cMemAddr>>2)<<2;
    int cMemAddr_re = cMemAddr-cMemAddr_qu;
    unsigned int Wdata = (MemData<<24)>>24;
    int i = FindMemory(cMemAddr_qu);
    if(i){
        unsigned int Odata = ReadMemory(cMemAddr_qu);
        if(cMemAddr_re==0){
            Odata = ( Odata & 0x00ffffff )| (Wdata<<24) ;
        }
        if(cMemAddr_re==1){
            Odata = ( Odata & 0xff00ffff )| (Wdata<<16) ;
        }
        if(cMemAddr_re==2){
            Odata = ( Odata & 0xffff00ff )| (Wdata<<8) ;
        }
        if(cMemAddr_re==3){
            Odata = ( Odata & 0xffffff00 )| (Wdata) ;
        }
        memory[i].data = Odata;
    }
    else{
        memory[memoryoffset].address = cMemAddr_qu;
        unsigned int Ndata;
        if(cMemAddr_re==0){
            Ndata = Wdata;
        }
        if(cMemAddr_re==1){
            Ndata = Wdata<<8;
        }
        if(cMemAddr_re==2){
            Ndata = Wdata<<16;
        }
        if(cMemAddr_re==3){
            Ndata = Wdata<<24;
        }
        memory[memoryoffset].address = cMemAddr_qu;
        memory[memoryoffset].data = Ndata;
        memoryoffset++;
    }
}

/*

    Register

*/

unsigned int reg[32];

//State Registers (Global)
int JP_STALL;
int LO_STALL;
int ATP_STALL;
int B_FAIL_STALL;

unsigned int PC;
unsigned int I_PC;
unsigned int J_PC;
unsigned int B_PC;

unsigned int IF_ID_INSTR;
unsigned int IF_ID_NPC;
unsigned int ID_BPC;
unsigned int ID_APC;

unsigned int ID_EX_NPC;
unsigned int ID_EX_RD1;
unsigned int ID_EX_RD2;
unsigned int ID_EX_RS;
unsigned int ID_EX_RT;
unsigned int ID_EX_IMM;
unsigned int ID_EX_RD;
unsigned int ID_EX_SHAMT;
unsigned int ID_EX_BPC;
unsigned int ID_EX_APC;

unsigned int I_ID_EX_BPC;
unsigned int I_ID_EX_APC;

unsigned int EX_MEM_NPC;
unsigned int EX_MEM_BR_TARGET;
unsigned int EX_MEM_ALUOUT;
unsigned int EX_MEM_RD2;
unsigned int EX_MEM_REGISTERRD;
unsigned int EX_MEM_BPC;
unsigned int EX_MEM_APC;

unsigned int I_EX_MEM_BPC;
unsigned int I_EX_MEM_APC;
unsigned int MEM_WB_NPC;
unsigned int MEM_WB_ALU_OUT;
unsigned int MEM_WB_MEM_OUT;
unsigned int MEM_WB_REGISTERRD;

unsigned int I_IF_ID_INSTR;
unsigned int I_IF_ID_NPC;

unsigned int I_ID_EX_NPC;
unsigned int I_ID_EX_RD1;
unsigned int I_ID_EX_RD2;
unsigned int I_ID_EX_RS;
unsigned int I_ID_EX_RT;
unsigned int I_ID_EX_IMM;
unsigned int I_ID_EX_RD;
unsigned int I_ID_EX_SHAMT;

unsigned int I_EX_MEM_NPC;
unsigned int I_EX_MEM_BR_TARGET;
unsigned int I_EX_MEM_ALUOUT;
unsigned int I_EX_MEM_RD2;
unsigned int I_EX_MEM_REGISTERRD;

unsigned int I_MEM_WB_NPC;
unsigned int I_MEM_WB_ALU_OUT;
unsigned int I_MEM_WB_MEM_OUT;
unsigned int I_MEM_WB_REGISTERRD;

unsigned int REGWRITEDATA = 0;

/*

        Control Signals at ID
        ----------------------
            ID_JUMP            0~1     Decide Jump      ( 0 - Don't      , 1 - Do    )
            int C_ID_SIGNEXT;
        ----------------------

*/

int C_ID_JUMP;
int C_ID_SIGNEXT;

/*

        Control Signals at EX
        ----------------------
        ID_EXE                   USE HERE?
            ID_EXE_BRANCH        X
            ID_EXE_MEMREAD       X
            ID_EXE_MEMTOREG      X
            ID_EXE_MEMWRITE      X
            ID_EXE_REGWRITE      X
            ID_EXE_ALUSRC        O
            ID_EXE_ALUOP         O
            ID_EXE_REGDST        O
        ----------------------
        FOWARDUNIT
            FOWARDA 0~2          O
            FOWARDB 0~2          O
        ----------------------
        
*/

int C_ID_EXE_BRANCH;
int C_ID_EXE_MEMREAD;
int C_ID_EXE_MEMTOREG;
int C_ID_EXE_MEMWRITE;
int C_ID_EXE_REGWRITE;
int C_ID_EXE_ALUSRC;
int C_ID_EXE_ALUOP;
int C_ID_EXE_REGDST;
int C_FOWARDA;
int C_FOWARDB;

int I_C_ID_EXE_BRANCH;
int I_C_ID_EXE_MEMREAD;
int I_C_ID_EXE_MEMTOREG;
int I_C_ID_EXE_MEMWRITE;
int I_C_ID_EXE_REGWRITE;
int I_C_ID_EXE_ALUSRC;
int I_C_ID_EXE_ALUOP;
int I_C_ID_EXE_REGDST;

/*

        Control Signals at MEM
        ----------------------
        EX_MEM
            EX_MEM_BRANCH        O
            EX_MEM_MEMREAD       O
            EX_MEM_MEMWRITE      O
            EX_MEM_ZERO          O
            EX_MEM_MEMTOREG      X
            EX_MEM_REGWRITE      X
        ----------------------
        ----------------------

*/

int C_EX_MEM_BRANCH;
int C_EX_MEM_MEMREAD;
int C_EX_MEM_MEMWRITE;
int C_EX_MEM_ZERO;
int C_EX_MEM_MEMTOREG;
int C_EX_MEM_REGWRITE;

int I_C_EX_MEM_BRANCH;
int I_C_EX_MEM_MEMREAD;
int I_C_EX_MEM_MEMWRITE;
int I_C_EX_MEM_ZERO;
int I_C_EX_MEM_MEMTOREG;
int I_C_EX_MEM_REGWRITE;

/*

        Control Signals at WB
        ----------------------
        MEM_WB
            MEM_WB_MEMTOREG      O
            MEM_WB_REGWRITE      O
        ----------------------
        
*/

int C_MEM_WB_MEMTOREG;
int C_MEM_WB_REGWRITE;

int I_C_MEM_WB_MEMTOREG;
int I_C_MEM_WB_REGWRITE;

/*
    IF Stage

        1. Get Instruction
        2. Update PC

*/

void IF(){
    // I_IF_ID_INSTR, I_IF_ID_NPC
    I_IF_ID_INSTR = ReadMemory((int)PC);
    I_PC = PC + 4;
    I_IF_ID_NPC = I_PC;
}

/*
    ID Stage

        1. Split Instructions
        2. Control Reset & Creation 
        3. Read Register
        4. Imm Extension

*/

void ID(){

    // 1. Split Instructions
    unsigned int op = 0;
    unsigned int rs = 0;
    unsigned int rt = 0;
    unsigned int rd = 0;
    unsigned int imm = 0;
    unsigned int jptarget = 0;
    unsigned int funct = 0;
    unsigned int shamt = 0;
    unsigned int jpaddr = 0;

    op = (unsigned int) getop(IF_ID_INSTR);
    rs = (unsigned int) getrs(IF_ID_INSTR);
    rt = (unsigned int) getrt(IF_ID_INSTR);
    rd = (unsigned int) getrd(IF_ID_INSTR);
    shamt = (unsigned int) getsh(IF_ID_INSTR);
    funct = (unsigned int) getfu(IF_ID_INSTR);
    imm =  (unsigned int) getim(IF_ID_INSTR);
    jptarget = (unsigned int) getjp(IF_ID_INSTR);

    //printf("opcode is 0x%x\n",op);


    // 2. Immediate value reset

    C_ID_JUMP = 0;
    C_ID_SIGNEXT = 0;

    I_C_ID_EXE_BRANCH = 0;
    I_C_ID_EXE_MEMREAD = 0;
    I_C_ID_EXE_MEMTOREG = 0;
    I_C_ID_EXE_MEMWRITE = 0;
    I_C_ID_EXE_REGWRITE = 0;
    I_C_ID_EXE_ALUSRC = 0;
    I_C_ID_EXE_ALUOP = 0;
    I_C_ID_EXE_REGDST = 0;

    I_ID_EX_NPC = 0;
    I_ID_EX_RD1 = 0;
    I_ID_EX_RD2 = 0;
    I_ID_EX_RS = 0;
    I_ID_EX_RT = 0;
    I_ID_EX_IMM = 0;
    I_ID_EX_RD = 0;
    I_ID_EX_SHAMT = 0;

    if(IF_ID_NPC==0){
        goto SKIP;
    }

    //READ

    I_ID_EX_RD1 = reg[rs];
    I_ID_EX_RD2 = reg[rt];
    I_ID_EX_RS = rs;
    I_ID_EX_RT = rt;
    I_ID_EX_IMM = imm;
    I_ID_EX_RD = rd;
    I_ID_EX_SHAMT = shamt;
    I_ID_EX_NPC = IF_ID_NPC;

    //Handle JUMP

    //J
    if(op==0x2){
        C_ID_JUMP = 1;
        jpaddr = (jptarget<<2)+(IF_ID_NPC&0xF0000000);
    }
    //JAL
    if(op==0x3){
        //printf("jal!\n");
        C_ID_JUMP = 1;
        jpaddr = (jptarget<<2)+(IF_ID_NPC&0xF0000000);
        I_ID_EX_RD1 = IF_ID_NPC;
        I_ID_EX_RT = 31;
        I_C_ID_EXE_REGWRITE = 1;
        I_C_ID_EXE_REGDST = 0;
        I_C_ID_EXE_ALUOP = 9;
        I_C_ID_EXE_MEMTOREG = 0;
    }
    //JR
    if(op==0){
        if(funct==0x8){
            C_ID_JUMP = 1;
            jpaddr = I_ID_EX_RD1;
        }
    }

    //DO JUMP
    //STALL(NOT YET), JUMP
    if(C_ID_JUMP == 1){
        J_PC = jpaddr;
        //printf("jump to! 0x%x\n",J_PC);
        JP_STALL = 1;
    }

    //DO ATP BRANCH
    //BEQ
    

    //CONTROL CREATION

    if(op==0){
        //rtype
        //ADDU
        if(funct==0x21){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 2;
            I_C_ID_EXE_REGWRITE = 1;
        }
        //AND
        if(funct==0x24){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 0;
            I_C_ID_EXE_REGWRITE = 1;
        }
        //NOR
        if(funct==0x27) {
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 12;
            I_C_ID_EXE_REGWRITE = 1;
        }
        //OR
        if(funct==0x25){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 1;
            I_C_ID_EXE_REGWRITE = 1;
        }
        //SLTU
        if(funct==0x2b){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 7;
            I_C_ID_EXE_REGWRITE = 1;
        }
        //SLL
        if(funct==0x0){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 15;
            I_C_ID_EXE_REGWRITE = 1;
            I_C_ID_EXE_ALUSRC = 2;
        }
        //SRL
        if(funct==0x2){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 14;
            I_C_ID_EXE_REGWRITE = 1;
            I_C_ID_EXE_ALUSRC = 2;
        }
        //SUBU
        if(funct==0x23){
            I_C_ID_EXE_REGDST = 1;
            I_C_ID_EXE_ALUOP = 6;
            I_C_ID_EXE_REGWRITE = 1;
        }
    }
    //Iformat - Direct Arithmetic
    //ADDIU
    if(op==0x9){
        I_C_ID_EXE_ALUOP = 2;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_REGWRITE = 1;
    }
    //ANDI
    if(op==0xc){
        I_C_ID_EXE_ALUOP = 0;
        C_ID_SIGNEXT = 0;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_REGWRITE = 1;
    }
    //ORI
    if(op==0xd){
        I_C_ID_EXE_ALUOP = 1;
        C_ID_SIGNEXT = 0;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_REGWRITE = 1;
    }
    //LUI
    if(op==0xf){
        I_C_ID_EXE_ALUOP = 8;
        C_ID_SIGNEXT = 0;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_REGWRITE = 1;
    }
    //SLTIU
    if(op==0xb){
        I_C_ID_EXE_ALUOP = 7;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_REGWRITE = 1;
    }

    //Iformat - Load
    //LW
    if(op==0x23){
        I_C_ID_EXE_ALUOP=2;
        C_ID_SIGNEXT=1;
        I_C_ID_EXE_ALUSRC=1;
        I_C_ID_EXE_MEMREAD=1;
        I_C_ID_EXE_REGWRITE=1;
        I_C_ID_EXE_MEMTOREG=1;
    }
    //LB
    if(op==0x20){
        I_C_ID_EXE_ALUOP=2;
        C_ID_SIGNEXT=1;
        I_C_ID_EXE_ALUSRC=1;
        I_C_ID_EXE_MEMREAD=2;
        I_C_ID_EXE_REGWRITE=1;
        I_C_ID_EXE_MEMTOREG=1;
    }

    //Iformat - Save
    //SW
    if(op==0x2b){
        I_C_ID_EXE_REGDST = 1;
        I_C_ID_EXE_ALUOP = 2;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_MEMWRITE = 1;
    }
    //SB
    if(op==0x28){
        I_C_ID_EXE_REGDST = 1;
        I_C_ID_EXE_ALUOP = 2;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_ALUSRC = 1;
        I_C_ID_EXE_MEMWRITE = 2;
    }

    //Iformat Branch
    //BEQ
    if(op==0x4){
        I_C_ID_EXE_ALUOP = 11;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_BRANCH = 1;
    }
    //BNE
    if(op==0x5){
        I_C_ID_EXE_ALUOP = 13;
        C_ID_SIGNEXT = 1;
        I_C_ID_EXE_BRANCH = 1;
    }

    //(LOAD) stall te pipeline
    if(C_ID_EXE_MEMREAD>0){
        if(ID_EX_RT==rs){
            LO_STALL = 1;
            //printf("\noccured\n");
        }
        if(ID_EX_RT==rt){
            LO_STALL = 1;
            //printf("\noccured\n");
        }
    }

SKIP: //label skip

    //Imm Extension
    if(C_ID_SIGNEXT==1){
        I_ID_EX_IMM = SignedExtend_Half(I_ID_EX_IMM);
    }

    if(atp==1){
        if((op==0x4)||(op==0x5)){
            ID_APC = IF_ID_NPC;
            ID_BPC = IF_ID_NPC + (I_ID_EX_IMM<<2);
            I_ID_EX_BPC = ID_BPC;
            I_ID_EX_APC = ID_APC;
            ATP_STALL = 1;
        }
    }

}


unsigned int ALU(int Signal, unsigned int Src1, unsigned int Src2){
/*

    ALUOp
    AND 0000 = 0
    OR  0001 = 1
    ADD 0010 = 2
    SUB 0110 = 6
    SLT 0111 = 7
    NOR 1100 = 12
    SL  1111 = 15
    SR  1110 = 14
    S16 1000 = 8
    J-R 1001 = 9
    Eq  1011 = 11
    NEq 1101 = 13

*/
    unsigned int result;
    if(Signal==0){
        result = Src1 & Src2;
    }
    if(Signal==1){
        result = Src1 | Src2;
    }
    if(Signal==2){
        result = Src1 + Src2;
    }
    if(Signal==6){
        result = Src1 - Src2;
    }
    if(Signal==7){
        result = (Src1 < Src2);
    }
    if(Signal==12){
        result = ~Src1 & ~Src2;
    }
    if(Signal==15){
        result = Src2<<Src1;
    }
    if(Signal==14){
        result = Src2>>Src1;
    }
    if(Signal==8){
        result = Src2<<16;
    }
    if(Signal==9){
        result = Src1;
    }
    if(Signal==11){
        I_C_EX_MEM_ZERO = (Src1==Src2);
        result = 0;
    }
    if(Signal==13){
        I_C_EX_MEM_ZERO = (Src1!=Src2);
        result = 0;
    }
    return result;
}

void EXE(){

    int ALU0 = 0;
    int ALU1 = 0;
    
    I_C_EX_MEM_BRANCH = C_ID_EXE_BRANCH;
    I_C_EX_MEM_MEMREAD = C_ID_EXE_MEMREAD;
    I_C_EX_MEM_MEMWRITE = C_ID_EXE_MEMWRITE;
    I_C_EX_MEM_ZERO = 0;
    I_C_EX_MEM_MEMTOREG = C_ID_EXE_MEMTOREG;
    I_C_EX_MEM_REGWRITE  = C_ID_EXE_REGWRITE;

    I_EX_MEM_NPC = ID_EX_NPC;
    I_EX_MEM_BR_TARGET = 0;
    I_EX_MEM_ALUOUT = 0;
    I_EX_MEM_RD2 = 0;
    I_EX_MEM_REGISTERRD = 0;

    //1. DATA HAZARD
    //(EX)
    C_FOWARDA = 0;
    C_FOWARDB = 0;
    if(C_EX_MEM_REGWRITE>0){
        if((EX_MEM_REGISTERRD == ID_EX_RS)&&(ID_EX_RS != 0)){
            C_FOWARDA = 2;
            //printf("hazard!!\n");
        }
        if((EX_MEM_REGISTERRD == ID_EX_RT)&&(ID_EX_RT != 0)){
            C_FOWARDB = 2;
            //printf("hazard!!\n");
        }
    }
    //(MEM)
    if(C_MEM_WB_REGWRITE>0){
        if((MEM_WB_REGISTERRD == ID_EX_RS)&&(ID_EX_RS != 0)&&(EX_MEM_REGISTERRD != ID_EX_RS)){
            C_FOWARDA = 1;
            //printf("hazard!!\n");
        }
        if((MEM_WB_REGISTERRD == ID_EX_RT)&&(ID_EX_RT != 0)&&(EX_MEM_REGISTERRD != ID_EX_RT)){
            C_FOWARDB = 1;
            //printf("hazard!!\n");
        }
    }

    //ALU0, ALU1 Determine
    if(C_FOWARDA==0){
        ALU0 = ID_EX_RD1;
    }
    if(C_FOWARDA==1){
        ALU0 = REGWRITEDATA;
        //printf("hazard! from mem_wb\n");
    }
    if(C_FOWARDA==2){
        ALU0 = EX_MEM_ALUOUT;
        //printf("hazard! from ex_mem\n");
    }
    if(C_ID_EXE_ALUSRC==2){
        ALU0 = ID_EX_SHAMT;
    }

    //ALU1
    if(C_FOWARDB==0){
        ALU1 = ID_EX_RD2;
    }
    if(C_FOWARDB==1){
        ALU1 = REGWRITEDATA;
        //printf("hazard! from mem_wb\n");
    }
    if(C_FOWARDB==2){
        ALU1 = EX_MEM_ALUOUT;
        //printf("hazard! from ex_mem\n");
    }
    I_EX_MEM_RD2 = ALU1;
    
    if(C_ID_EXE_ALUSRC==1){
        ALU1 = ID_EX_IMM;
    }

    //2. Do ALU
    //printf("ALU0 : 0x%x, ALU1: 0x%x\n",ALU0,ALU1);
    I_EX_MEM_ALUOUT = ALU(C_ID_EXE_ALUOP,ALU0,ALU1);

    //3. Calculate Branch Address, Jump Address, RegJump Address
    I_EX_MEM_BR_TARGET = ID_EX_NPC + (ID_EX_IMM<<2);

    //4. Decide WriteAddr by RegDst

    if(C_ID_EXE_REGDST==0){
        I_EX_MEM_REGISTERRD = ID_EX_RT;
    }
    if(C_ID_EXE_REGDST==1){
        I_EX_MEM_REGISTERRD = ID_EX_RD;
    }

    if(atp==1){
        I_EX_MEM_BPC = ID_EX_BPC;
        I_EX_MEM_APC = ID_EX_APC;
    }
}


/*
    MEM Stage

        1. Decide PC (Branch, Jump, JR, JAL)
        2. Memory Read // w, b
        3. Memory Write // w, b

*/

void MEM(){

    I_C_MEM_WB_MEMTOREG = C_EX_MEM_MEMTOREG;
    I_C_MEM_WB_REGWRITE = C_EX_MEM_REGWRITE;

    I_MEM_WB_NPC = EX_MEM_NPC;
    I_MEM_WB_ALU_OUT = EX_MEM_ALUOUT;
    I_MEM_WB_MEM_OUT = 0;
    I_MEM_WB_REGISTERRD = EX_MEM_REGISTERRD;

    // 1. Decide PC (Branch, Jump, JR, JAL)

    if(C_EX_MEM_BRANCH==1){
        //stall
        if(C_EX_MEM_ZERO==1){
            if(atp==0){
                B_PC = EX_MEM_BR_TARGET;
                B_FAIL_STALL = 1;
            }
            if(atp==1){
                //B_FAIL_STALL = 1;
                //printf("atp suuccessed\n\n");
                B_FAIL_STALL = 0;
            }
        }
        if(C_EX_MEM_ZERO==0){
            if(atp==1){
                //printf("atp failed!!\n\n");
                B_FAIL_STALL = 1;
            }
        }
    }

    //2. Memory Read // w, b
    if(C_EX_MEM_MEMREAD==1){
        I_MEM_WB_MEM_OUT = ReadMemory((int)EX_MEM_ALUOUT);
    }
    if(C_EX_MEM_MEMREAD==2){
        I_MEM_WB_MEM_OUT = ReadByteMemory((int)EX_MEM_ALUOUT);
    }

    //3. Memory Write // w, b
    if(C_EX_MEM_MEMWRITE==1){
        WriteMemory((int)EX_MEM_ALUOUT,EX_MEM_RD2);
        //printf("writeoccured at PC : 0x%x, to PC:0x%x, RD2 : 0x%x\n",EX_MEM_NPC,EX_MEM_ALUOUT,EX_MEM_RD2);
    }
    if(C_EX_MEM_MEMWRITE==2){
        WriteByteMemory((int)EX_MEM_ALUOUT,EX_MEM_RD2);
        //printf("writeoccured at PC : 0x%x, to PC:0x%x, RD2 : 0x%x\n",EX_MEM_NPC,EX_MEM_ALUOUT,EX_MEM_RD2);
    }
}


/*
    WB Stage

        1. Decide WriteData
        2. Write

*/

void WB(){

    if(C_MEM_WB_MEMTOREG==0){
        REGWRITEDATA = MEM_WB_ALU_OUT;
    }
    if(C_MEM_WB_MEMTOREG==1){
        REGWRITEDATA = MEM_WB_MEM_OUT;
    }
    if(C_MEM_WB_REGWRITE==1){
        reg[(int)MEM_WB_REGISTERRD] = REGWRITEDATA;
        //printf("Reg write in %d - 0x%x\n",MEM_WB_REGISTERRD,MEM_WB_ALU_OUT);
    }

}


int main(int argc, char* argv[]){

    //FLAG TEST
    n = 1024;
    m = 0;
    d = 0;
    p = 0;
    atp = 0;
    cc = 1;
    
    //0. find atp m d p n binary file
    int i;

    //find m
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-m")==0){
            m=1;
            // set m's ~
            char memorybound[50];
            strcpy(memorybound,argv[i+1]);
            char *ptr = strtok(memorybound,":");
            m1=Makenumber(ptr);
            ptr = strtok(NULL,":");
            m2=Makenumber(ptr);
        }
    }

    // find n
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-n")==0){
            // set m's ~
            char numberofinst[50];
            strcpy(numberofinst,argv[i+1]);
            n = atoi(numberofinst);
        }
    }

    // find p
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-p")==0){
            // set m's ~
            p = 1;
        }
    }

    // find d
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-d")==0){
            // set m's ~
            d = 1;
        }
    }

    // find atp
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-atp")==0){
            // set m's ~
            atp = 1;
        }
    }



    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-m")==0){
            m=1;
            // set m's ~
            char memorybound[50];
            strcpy(memorybound,argv[i+1]);
            char *ptr = strtok(memorybound,":");
            m1=Makenumber(ptr);
            ptr = strtok(NULL,":");
            m2=Makenumber(ptr);
        }
    }



    //1. load Memory
    memoryoffset = 0;
    FILE *pFile = NULL;
    char *pLine;
    pFile = fopen(argv[argc-1], "r");
    const int max = 1024;
    char line[max];
    pLine = fgets(line, max, pFile);
    Delenter(pLine);
    int textsectionsize = Makenumber(pLine)/4;
    pLine = fgets(line, max, pFile);
    Delenter(pLine);
    int datasectionsize = Makenumber(pLine)/4;
    for(i=0;i<textsectionsize;i++){
        pLine = fgets(line, max, pFile);
        Delenter(pLine);
        int instdata = Makenumber(pLine);
        memory[memoryoffset].address = ioffset + i*4;
        memory[memoryoffset].data = (unsigned int) instdata;
        memoryoffset++;
    }
    for(i=0;i<datasectionsize;i++){
        pLine = fgets(line, max, pFile);
        Delenter(pLine);
        int instdata = Makenumber(pLine);
        memory[memoryoffset].address = doffset + i*4;
        memory[memoryoffset].data = (unsigned int) instdata;
        memoryoffset++;
    }
    fclose(pFile);
    

    //2. PIPELINING
    PC = 0x00400000;
    int whentofinish = 100;
    JP_STALL = 0;
    LO_STALL = 0;
    ATP_STALL = 0;
    B_FAIL_STALL = 0;


    while(n>0){
        printf("===== Cycle %d =====\n",cc);
        //printf("0x%x,0x%x,0x%x,0x%x,0x%x\n",PC,IF_ID_NPC-4,ID_EX_NPC-4,EX_MEM_NPC-4,MEM_WB_NPC-4);
        int end = 0;
        if(p==1){
            printf("Current pipeline PC state:\n");
            printf("{");
            if(PC==0||ReadMemory((int)PC)==0){
                //printf("");
                end += 1;
            }
            else{
                printf("0x%x",PC);
            }
            printf("|");
            if(IF_ID_NPC==0||ReadMemory((int)(IF_ID_NPC-4))==0){
                //printf("");
                end += 1;
            }
            else{
                printf("0x%x",IF_ID_NPC-4);
            }
            printf("|");
            if(ID_EX_NPC==0||ReadMemory((int)(ID_EX_NPC-4))==0){
                //printf("");
                end += 1;
            }
            else{
                printf("0x%x",ID_EX_NPC-4);
            }
            printf("|");
            if(EX_MEM_NPC==0||ReadMemory((int)(EX_MEM_NPC-4))==0){
                //printf("");
                end += 1;
            }
            else{
                printf("0x%x",EX_MEM_NPC-4);
            }
            printf("|");
            if(MEM_WB_NPC==0||ReadMemory((int)(MEM_WB_NPC-4))==0){
                //printf("");
            }
            else{
                printf("0x%x",MEM_WB_NPC-4);
                n--;
            }
            printf("}\n\n");
        }
        else if(p==0){
            if(PC==0||ReadMemory((int)PC)==0){
                end += 1;
            }
            if(IF_ID_NPC==0||ReadMemory((int)(IF_ID_NPC-4))==0){
                end += 1;
            }
            if(ID_EX_NPC==0||ReadMemory((int)(ID_EX_NPC-4))==0){
                end += 1;
            }
            if(EX_MEM_NPC==0||ReadMemory((int)(EX_MEM_NPC-4))==0){
                end += 1;
            }
            if(MEM_WB_NPC==0||ReadMemory((int)(MEM_WB_NPC-4))==0){
                //printf("");
            }
            else{
                n--;
            }
        }
        if(end==4){
            n = 0;
        }
        WB();
        MEM();
        EXE();
        ID();
        IF();
        
        ID_EX_BPC = I_ID_EX_BPC;
        ID_EX_APC = I_ID_EX_APC;
        EX_MEM_BPC = I_EX_MEM_BPC;
        EX_MEM_APC = I_EX_MEM_APC;

        //
        if(JP_STALL+LO_STALL+ATP_STALL+B_FAIL_STALL==0){
#ifdef DEBUG0
            printf("No Stall\n");
#endif
            PC = I_PC;

            IF_ID_NPC = I_IF_ID_NPC;
            IF_ID_INSTR = I_IF_ID_INSTR;

            ID_EX_NPC = I_ID_EX_NPC;
            ID_EX_RD1 = I_ID_EX_RD1;
            ID_EX_RD2 = I_ID_EX_RD2;
            ID_EX_RS = I_ID_EX_RS;
            ID_EX_RT = I_ID_EX_RT;
            ID_EX_IMM = I_ID_EX_IMM;
            ID_EX_RD = I_ID_EX_RD;
            ID_EX_SHAMT = I_ID_EX_SHAMT;

            
            EX_MEM_NPC = I_EX_MEM_NPC;
            EX_MEM_BR_TARGET = I_EX_MEM_BR_TARGET;
            EX_MEM_ALUOUT = I_EX_MEM_ALUOUT;
            EX_MEM_RD2 = I_EX_MEM_RD2;
            EX_MEM_REGISTERRD = I_EX_MEM_REGISTERRD;

            MEM_WB_NPC = I_MEM_WB_NPC;
            MEM_WB_ALU_OUT = I_MEM_WB_ALU_OUT;
            MEM_WB_MEM_OUT = I_MEM_WB_MEM_OUT;
            MEM_WB_REGISTERRD = I_MEM_WB_REGISTERRD;

            C_ID_EXE_BRANCH = I_C_ID_EXE_BRANCH;
            C_ID_EXE_MEMREAD = I_C_ID_EXE_MEMREAD;
            C_ID_EXE_MEMTOREG = I_C_ID_EXE_MEMTOREG;
            C_ID_EXE_MEMWRITE = I_C_ID_EXE_MEMWRITE;
            C_ID_EXE_REGWRITE = I_C_ID_EXE_REGWRITE;
            C_ID_EXE_ALUSRC = I_C_ID_EXE_ALUSRC;
            C_ID_EXE_ALUOP = I_C_ID_EXE_ALUOP;
            C_ID_EXE_REGDST = I_C_ID_EXE_REGDST;

            C_EX_MEM_BRANCH = I_C_EX_MEM_BRANCH;
            C_EX_MEM_MEMREAD = I_C_EX_MEM_MEMREAD;
            C_EX_MEM_MEMWRITE = I_C_EX_MEM_MEMWRITE;
            C_EX_MEM_ZERO = I_C_EX_MEM_ZERO;
            C_EX_MEM_MEMTOREG = I_C_EX_MEM_MEMTOREG;
            C_EX_MEM_REGWRITE = I_C_EX_MEM_REGWRITE;

            C_MEM_WB_MEMTOREG = I_C_MEM_WB_MEMTOREG;
            C_MEM_WB_REGWRITE = I_C_MEM_WB_REGWRITE;

        }
        //JUMP
        if(JP_STALL == 1){
            //printf("JUMP Stall\n");
            PC = J_PC;
            //printf("pc is 0x%x\n",I_PC);

            IF_ID_NPC = 0;
            IF_ID_INSTR = 0;

            ID_EX_NPC = I_ID_EX_NPC;
            ID_EX_RD1 = I_ID_EX_RD1;
            ID_EX_RD2 = I_ID_EX_RD2;
            ID_EX_RS = I_ID_EX_RS;
            ID_EX_RT = I_ID_EX_RT;
            ID_EX_IMM = I_ID_EX_IMM;
            ID_EX_RD = I_ID_EX_RD;
            ID_EX_SHAMT = I_ID_EX_SHAMT;

            
            EX_MEM_NPC = I_EX_MEM_NPC;
            EX_MEM_BR_TARGET = I_EX_MEM_BR_TARGET;
            EX_MEM_ALUOUT = I_EX_MEM_ALUOUT;
            EX_MEM_RD2 = I_EX_MEM_RD2;
            EX_MEM_REGISTERRD = I_EX_MEM_REGISTERRD;

            MEM_WB_NPC = I_MEM_WB_NPC;
            MEM_WB_ALU_OUT = I_MEM_WB_ALU_OUT;
            MEM_WB_MEM_OUT = I_MEM_WB_MEM_OUT;
            MEM_WB_REGISTERRD = I_MEM_WB_REGISTERRD;

            C_ID_EXE_BRANCH = I_C_ID_EXE_BRANCH;
            C_ID_EXE_MEMREAD = I_C_ID_EXE_MEMREAD;
            C_ID_EXE_MEMTOREG = I_C_ID_EXE_MEMTOREG;
            C_ID_EXE_MEMWRITE = I_C_ID_EXE_MEMWRITE;
            C_ID_EXE_REGWRITE = I_C_ID_EXE_REGWRITE;
            C_ID_EXE_ALUSRC = I_C_ID_EXE_ALUSRC;
            C_ID_EXE_ALUOP = I_C_ID_EXE_ALUOP;
            C_ID_EXE_REGDST = I_C_ID_EXE_REGDST;

            C_EX_MEM_BRANCH = I_C_EX_MEM_BRANCH;
            C_EX_MEM_MEMREAD = I_C_EX_MEM_MEMREAD;
            C_EX_MEM_MEMWRITE = I_C_EX_MEM_MEMWRITE;
            C_EX_MEM_ZERO = I_C_EX_MEM_ZERO;
            C_EX_MEM_MEMTOREG = I_C_EX_MEM_MEMTOREG;
            C_EX_MEM_REGWRITE = I_C_EX_MEM_REGWRITE;

            C_MEM_WB_MEMTOREG = I_C_MEM_WB_MEMTOREG;
            C_MEM_WB_REGWRITE = I_C_MEM_WB_REGWRITE;

            JP_STALL = 0;
        }

        //LOAD STORE STALL;
        //DONT PASS IF ID REGISTERS (ALSO PC)
        if(LO_STALL == 1){
            //printf("LOAD Stall\n");

            ID_EX_NPC = 0;
            ID_EX_RD1 = 0;
            ID_EX_RD2 = 0;
            ID_EX_RS = 0;
            ID_EX_RT = 0;
            ID_EX_IMM = 0;
            ID_EX_RD = 0;
            ID_EX_SHAMT = 0;

            
            EX_MEM_NPC = I_EX_MEM_NPC;
            EX_MEM_BR_TARGET = I_EX_MEM_BR_TARGET;
            EX_MEM_ALUOUT = I_EX_MEM_ALUOUT;
            EX_MEM_RD2 = I_EX_MEM_RD2;
            EX_MEM_REGISTERRD = I_EX_MEM_REGISTERRD;

            MEM_WB_NPC = I_MEM_WB_NPC;
            MEM_WB_ALU_OUT = I_MEM_WB_ALU_OUT;
            MEM_WB_MEM_OUT = I_MEM_WB_MEM_OUT;
            MEM_WB_REGISTERRD = I_MEM_WB_REGISTERRD;

            C_ID_EXE_BRANCH = 0;
            C_ID_EXE_MEMREAD = 0;
            C_ID_EXE_MEMTOREG = 0;
            C_ID_EXE_MEMWRITE = 0;
            C_ID_EXE_REGWRITE = 0;
            C_ID_EXE_ALUSRC = 0;
            C_ID_EXE_ALUOP = 0;
            C_ID_EXE_REGDST = 0;

            C_EX_MEM_BRANCH = I_C_EX_MEM_BRANCH;
            C_EX_MEM_MEMREAD = I_C_EX_MEM_MEMREAD;
            C_EX_MEM_MEMWRITE = I_C_EX_MEM_MEMWRITE;
            C_EX_MEM_ZERO = I_C_EX_MEM_ZERO;
            C_EX_MEM_MEMTOREG = I_C_EX_MEM_MEMTOREG;
            C_EX_MEM_REGWRITE = I_C_EX_MEM_REGWRITE;

            C_MEM_WB_MEMTOREG = I_C_MEM_WB_MEMTOREG;
            C_MEM_WB_REGWRITE = I_C_MEM_WB_REGWRITE;

            LO_STALL = 0;
        }

        //BRANCH STALL ATP WHEN IT WAS RIGHT
        if(ATP_STALL == 1){
            //printf("ATP Stall\n");
            PC = ID_BPC;

            ID_EX_BPC = I_ID_EX_BPC;
            ID_EX_APC = I_ID_EX_APC;
            EX_MEM_BPC = I_EX_MEM_BPC;
            EX_MEM_APC = I_EX_MEM_APC;
            
            IF_ID_NPC = 0;
            IF_ID_INSTR = 0;

            ID_EX_NPC = I_ID_EX_NPC;
            ID_EX_RD1 = I_ID_EX_RD1;
            ID_EX_RD2 = I_ID_EX_RD2;
            ID_EX_RS = I_ID_EX_RS;
            ID_EX_RT = I_ID_EX_RT;
            ID_EX_IMM = I_ID_EX_IMM;
            ID_EX_RD = I_ID_EX_RD;
            ID_EX_SHAMT = I_ID_EX_SHAMT;

            
            EX_MEM_NPC = I_EX_MEM_NPC;
            EX_MEM_BR_TARGET = I_EX_MEM_BR_TARGET;
            EX_MEM_ALUOUT = I_EX_MEM_ALUOUT;
            EX_MEM_RD2 = I_EX_MEM_RD2;
            EX_MEM_REGISTERRD = I_EX_MEM_REGISTERRD;

            MEM_WB_NPC = I_MEM_WB_NPC;
            MEM_WB_ALU_OUT = I_MEM_WB_ALU_OUT;
            MEM_WB_MEM_OUT = I_MEM_WB_MEM_OUT;
            MEM_WB_REGISTERRD = I_MEM_WB_REGISTERRD;

            C_ID_EXE_BRANCH = I_C_ID_EXE_BRANCH;
            C_ID_EXE_MEMREAD = I_C_ID_EXE_MEMREAD;
            C_ID_EXE_MEMTOREG = I_C_ID_EXE_MEMTOREG;
            C_ID_EXE_MEMWRITE = I_C_ID_EXE_MEMWRITE;
            C_ID_EXE_REGWRITE = I_C_ID_EXE_REGWRITE;
            C_ID_EXE_ALUSRC = I_C_ID_EXE_ALUSRC;
            C_ID_EXE_ALUOP = I_C_ID_EXE_ALUOP;
            C_ID_EXE_REGDST = I_C_ID_EXE_REGDST;

            C_EX_MEM_BRANCH = I_C_EX_MEM_BRANCH;
            C_EX_MEM_MEMREAD = I_C_EX_MEM_MEMREAD;
            C_EX_MEM_MEMWRITE = I_C_EX_MEM_MEMWRITE;
            C_EX_MEM_ZERO = I_C_EX_MEM_ZERO;
            C_EX_MEM_MEMTOREG = I_C_EX_MEM_MEMTOREG;
            C_EX_MEM_REGWRITE = I_C_EX_MEM_REGWRITE;

            C_MEM_WB_MEMTOREG = I_C_MEM_WB_MEMTOREG;
            C_MEM_WB_REGWRITE = I_C_MEM_WB_REGWRITE;

            ATP_STALL = 0;
        }

        if(B_FAIL_STALL == 1){
            if(atp==0){
                PC = B_PC;
            }
            if(atp==1){
                PC = EX_MEM_APC;
            }

            IF_ID_NPC = 0;
            IF_ID_INSTR = 0;

            ID_EX_NPC = 0;
            ID_EX_RD1 = 0;
            ID_EX_RD2 = 0;
            ID_EX_RS = 0;
            ID_EX_RT = 0;
            ID_EX_IMM = 0;
            ID_EX_RD = 0;
            ID_EX_SHAMT = 0;

            
            EX_MEM_NPC = 0;
            EX_MEM_BR_TARGET = 0;
            EX_MEM_ALUOUT = 0;
            EX_MEM_RD2 = 0;
            EX_MEM_REGISTERRD = 0;

            MEM_WB_NPC = I_MEM_WB_NPC;
            MEM_WB_ALU_OUT = I_MEM_WB_ALU_OUT;
            MEM_WB_MEM_OUT = I_MEM_WB_MEM_OUT;
            MEM_WB_REGISTERRD = I_MEM_WB_REGISTERRD;

            C_ID_EXE_BRANCH = 0;
            C_ID_EXE_MEMREAD = 0;
            C_ID_EXE_MEMTOREG = 0;
            C_ID_EXE_MEMWRITE = 0;
            C_ID_EXE_REGWRITE = 0;
            C_ID_EXE_ALUSRC = 0;
            C_ID_EXE_ALUOP = 0;
            C_ID_EXE_REGDST = 0;

            C_EX_MEM_BRANCH = 0;
            C_EX_MEM_MEMREAD = 0;
            C_EX_MEM_MEMWRITE = 0;
            C_EX_MEM_ZERO = 0;
            C_EX_MEM_MEMTOREG = 0;
            C_EX_MEM_REGWRITE = 0;

            C_MEM_WB_MEMTOREG = I_C_MEM_WB_MEMTOREG;
            C_MEM_WB_REGWRITE = I_C_MEM_WB_REGWRITE;

            B_FAIL_STALL = 0;
        }
        
        if(d==1){
            int k;
            printf("Current register values:\n----------------------------------\n");
            printf("PC: 0x%x\n",PC-4);
            printf("Registers:\n");
            for(k=0;k<32;k++){
                printf("R%d: 0x%x\n",k,reg[k]);
            }
            printf("\n");
            printf("\n");
            // printf memory
            if(m){
                //printf("m1: 0x%x m2: 0x%x\n",m1,m2);
                printf("Memory content: [0x%x..0x%x]\n",m1,m2);
                printf("-------------------------------\n");
                int t = m1;
                while(t<=m2){
                    printf("0x%x:  0x%x\n",t,ReadMemory(t));
                    t += 4;
                }
                printf("\n");
            }
        }

        REGWRITEDATA = 0;
        cc++;

    }

    int k;
    printf("===== Completion cycle: %d =====",cc-1);
    if(p){
        printf("\nCurrent pipeline PC state:\n");
        printf("{||||}\n\n");
    }
    printf("PC: 0x%x\n",PC-4);
    printf("Registers:\n");
    for(k=0;k<32;k++){
        printf("R%d: 0x%x\n",k,reg[k]);
    }
    printf("\n");

    if(m){
        //printf("m1: 0x%x m2: 0x%x\n",m1,m2);
        printf("Memory content: [0x%x..0x%x]\n",m1,m2);
        printf("-------------------------------\n");
        int t = m1;
        while(t<=m2){
            printf("0x%x:  0x%x\n",t,ReadMemory(t));
            t += 4;
        }
        printf("\n");
    }
    
}