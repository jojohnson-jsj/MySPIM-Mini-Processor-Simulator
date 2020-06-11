#include "spimcore.h"

/*
Final Project for CDA-3103C
4/14/2020
Tyler Hostler-Mathis
Jose Johnson
Cameron Berezuk
*/


// cam
/* ALU */
/* 10 Points */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult, char *Zero)
{
    // handle all ALU cases
    switch(ALUControl)
    {
      case 0:
        *ALUresult = A + B;
        break;

      case 1:
        *ALUresult = A - B;
        break;

      case 2:
        if ((int)A < (int)B)
          *ALUresult = 1;
        else 
          *ALUresult = 0;
        break;

      case 3:
        if (A < B)
          *ALUresult = 1;
        else 
          *ALUresult = 0;
        break;

      case 4:
        *ALUresult = A & B;
        break;

      case 5:
        *ALUresult = A | B;
        break;

      case 6:
        *ALUresult = B << 16;
        break;

      case 7:
        *ALUresult = ~A;
        break;

      default:
        break;
    }

  // assert zero if necessary
  *Zero = !(*ALUresult);

  return;
}


// cam
/* instruction fetch */
/* 10 Points */
int instruction_fetch(unsigned PC, unsigned *Mem, unsigned *instruction)
{
  if (PC > 0xFFFF || PC % 4 != 0)
    return 1;

  *instruction = Mem[(PC >> 2)];

  return 0;
}


/* instruction partition */
/* 10 Points */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1, unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
	// Create bit masks to & instruction with
	// First create correct number of 1s, then shift left
	*op = 		(instruction & (((1 << 6) - 1) << (31 - 5))) >> (31 - 5);
	*r1 = 		(instruction & (((1 << 5) - 1) << (25 - 4))) >> (25 - 4);
	*r2 = 		(instruction & (((1 << 5) - 1) << (20 - 4))) >> (20 - 4);
	*r3 = 		(instruction & (((1 << 5) - 1) << (15 - 4))) >> (15 - 4);
	*funct = 	instruction & ((1 << 6) - 1);
	*offset = instruction & ((1 << 16) - 1);
	*jsec = 	instruction & ((1 << 26) - 1);
}



/* instruction decode */
/* 15 Points */
int instruction_decode(unsigned op, struct_controls *controls)
{
  // zero everything out first
	controls->RegDst =   0;
	controls->Jump =     0;
	controls->Branch =   0;
	controls->MemRead =  0;
	controls->MemtoReg = 0;
	controls->MemWrite = 0;
	controls->ALUSrc =   0;
	controls->RegWrite = 0;
  controls->ALUOp = 0b000;

  switch (op)
  {
    case 0b000000:  // r-type
      controls->RegDst = 1;
      controls->RegWrite = 1;
      controls->ALUOp = 0b111;
      break;

    case 0b100011:  // lw
      controls->MemRead = 1;
      controls->MemtoReg = 1;
      controls->ALUSrc = 1;
      controls->RegWrite = 1;
      controls->ALUOp = 0b000;
      break;

    case 0b000100:  // beq
      controls->Branch = 1;
      controls->ALUOp = 0b001;
      break;

    case 0b000010:  // jump
      controls->Jump = 1;
      controls->ALUOp = 0b1000; // special case for jump escape
      break;

    case 0b001000:  // addi
      controls->ALUSrc = 1;
      controls->RegWrite = 1;
      controls->ALUOp = 0b000;
      break;

    case 0b101011:  // sw
      controls->MemWrite = 1;
      controls->ALUSrc = 1;
      controls->ALUOp = 0b000;
      break;

    case 0b001111:  // lui
      controls->ALUSrc = 1;
      controls->RegWrite = 1;
      controls->ALUOp = 0b110;
      break;

    case 0b001010:  // slti
      controls->RegWrite = 1;
      controls->ALUSrc = 1;
      controls->ALUOp = 0b010;
      break;

    case 0b001011:  // sltui
      controls->RegWrite = 1;
      controls->ALUSrc = 1;
      controls->ALUOp = 0b011;
      break;

    // failure
    default:
      return 1;
  }

  // success
  return 0;
}

/* Read Register */
/* 5 Points */
void read_register(unsigned r1, unsigned r2, unsigned *Reg, unsigned *data1, unsigned *data2)
{
	*data1 = Reg[r1];
	*data2 = Reg[r2];
}

/* Sign Extend */
/* 10 Points */
void sign_extend(unsigned offset,unsigned *extended_value)
{
  unsigned mostSig = offset >> 15;            // Figuring out MSB by right shifting up to it;

  if (mostSig == 0)                           // If MSB is 0 the number is positive
    *extended_value = offset & 0x0000FFFF;

  else                                        // If MSB isn't 0 then the number is negative and
    *extended_value = offset | 0xFFFF0000;    // needs to be expanded
}

// cam
/* ALU operations */
/* 10 Points */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
  // special case for jump
  if (ALUOp == 8)
    return 0;

  if (ALUOp == 7 && ALUSrc == 0) // R TYPE
  {
    if (funct == 32)
      ALUOp = 0;
    else if (funct == 34)
      ALUOp = 1;
    else if (funct == 42)
      ALUOp = 2;
    else if (funct == 43)
      ALUOp = 3;
    else if (funct ==  36)
      ALUOp = 4;
    else if (funct == 37)
      ALUOp = 5;
    else 
      return 1;

    ALU(data1, data2, ALUOp, ALUresult, Zero);
  }
  else if (ALUSrc == 1) // I TYPE 
  {
    if (ALUOp >= 7 || ALUOp < 0)
      return 1;
    else 
      ALU(data1, extended_value, ALUOp, ALUresult, Zero);
  }
  else 
    return 1;

  return 0;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
  if (MemRead || MemWrite)
  {
    if (ALUresult % 4 != 0 || ALUresult > 0xFFFF)                  // Invalid address, so halt
      return 1;

    if (MemWrite == 1)                       // Writing to memory
      Mem[ALUresult >> 2] = data2;

    if (MemRead == 1)                        // Reading from memory
      *memdata = Mem[ALUresult >> 2];
  }

  return 0;
}

/* Write Register */
/* 10 Points */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
  if (RegWrite == 1)                 // Permission to write
  {
    if (MemtoReg == 1)               // Writing from memory to a register
      if (RegDst == 0)
        Reg[r2] = memdata;
      else
        Reg[r3] = memdata;

    else                             // Writing from the ALU to a register
      if (RegDst == 1)
        Reg[r3] = ALUresult;
      else
        Reg[r2] = ALUresult;
  }
}

/* PC update */
/* 10 Points */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
  *PC += 4;                                    // Moving to the next word

  if (Jump == 1)                               // Jump is 1
    *PC = (*PC & 0xF0000000) | (jsec << 2);   // PC is the upper 4 bits of the PC & with the Jump
                                               // Shifted by 2 to the left

  if (Branch == 1 && Zero == 1)                // If Branch and Zero are both 1, PC is the
    *PC += extended_value << 2;                // extended value shifted over 2
}
