#include "hash_table.h"
#include <string.h>

#define lw "100011"
#define sw "101011"
#define add "100000"
#define sub "100010"
#define addi "001000"
#define or "100101"
#define and "100100"
#define ori "001101"
#define andi "001100"
#define slt "101010"
#define slti "001010"
#define sll "000000"
#define srl "000010"
#define beq "000100"
#define bne "000101"
#define j "000010"
#define jr "000000000000000001000"
#define jal "000011"
#define lui "001111"
#define $v0 "00010"
#define $v1 "00011"	
#define $a0 "00100"	
#define $a1 "00101"
#define $a2 "00110"
#define $a3 "00111"
#define $t0 "01000"
#define $t1 "01001"
#define $t2 "01010"
#define $t3 "01011"
#define $t4 "01100"
#define $t5 "01101"
#define $t6 "01110"
#define $t7 "01111"	
#define $s0 "10000"
#define $s1 "10001"
#define $s2 "10010"
#define $s3 "10011"
#define $s4 "10100"
#define $s5 "10101"
#define $s6 "10110"
#define $s7 "10111"
#define $t8 "11000"
#define $t9 "11001"
#define $gp "11100"
#define $sp "11101"
#define $fp "11110"
#define $ra "11111"
#define $k0 "11010"
#define $k1 "11011"
#define $at "00001"
#define $zero "00000"
#define la "-1"

/*
 * =====================================================================================
 *
 * Filename:  initialization.h
 *
 * Description: Initializes our two hash tables with data. The first hash table will 
 * contain the opcodes for all the instructions (in binary form). The other hash table
 * will contain all the binary identifiers for the registers.
 *
 * Author:  Karthik Kumar, kkumar91@vt.edu
 *
 * =====================================================================================
 */

void init_opcodes_table(hash_table_t *code_table);

void init_register_table(hash_table_t *register_table);

/*
 * Initializes our hash table with the opcodes. It simply calls the hash_insert method
 * for each intruction
 */
void init_opcodes_table(hash_table_t *code_table) 
{
	hash_insert(code_table, "lw", strlen("lw"), lw); 
	
	hash_insert(code_table, "sw", strlen("sw"), sw);

	hash_insert(code_table, "add", strlen("add"), add);
	
	hash_insert(code_table, "sub", strlen("sub"), sub);

	hash_insert(code_table, "addi", strlen("addi"), addi);

	hash_insert(code_table, "or", strlen("or"), or);
	
	hash_insert(code_table, "and", strlen("and"), and);
	
	hash_insert(code_table, "ori", strlen("ori"), ori);
	
	hash_insert(code_table, "andi", strlen("andi"), andi);
	
	hash_insert(code_table, "slt", strlen("slt"), slt);
	
	hash_insert(code_table, "slti", strlen("slti"), slti);
	
	hash_insert(code_table, "sll", strlen("sll"), sll);
	
	hash_insert(code_table, "srl", strlen("srl"), srl);
	
	hash_insert(code_table, "beq", strlen("beq"), beq);
	
	hash_insert(code_table, "j", strlen("j"), j);
	
	hash_insert(code_table, "jr", strlen("jr"), jr);
	
	hash_insert(code_table, "jal", strlen("jal"), jal);

	hash_insert(code_table, "lui", strlen("lui"), lui);
	
	hash_insert(code_table, "bne", strlen("bne"), bne);

	hash_insert(code_table, "la", strlen("la"), bne);
}

/*
 * Initializes the register table. Calls the hash_insert method
 * several times.
 */
void init_register_table(hash_table_t *register_table) 
{
	hash_insert(register_table, "$v0", strlen("$v0"), $v0);
	
	hash_insert(register_table, "$v1", strlen("$v1"), $v1);
	
	hash_insert(register_table, "$a0", strlen("$a0"), $a0);
	
	hash_insert(register_table, "$a1", strlen("$a1"), $a1);
	
	hash_insert(register_table, "$a2", strlen("$a2"), $a2);
	
	hash_insert(register_table, "$a3", strlen("$a3"), $a3);
	
	hash_insert(register_table, "$t0", strlen("$t0"), $t0);
	
	hash_insert(register_table, "$t1", strlen("$t1"), $t1);
	
	hash_insert(register_table, "$t2", strlen("$t2"), $t2);
	
	hash_insert(register_table, "$t3", strlen("$t3"), $t3);
	
	hash_insert(register_table, "$t4", strlen("$t4"), $t4);

	hash_insert(register_table, "$t5", strlen("$t5"), $t5);
	
	hash_insert(register_table, "$t6", strlen("$t6"), $t6);
	
	hash_insert(register_table, "$t7", strlen("$t7"), $t7);
	
	hash_insert(register_table, "$t8", strlen("$t8"), $t8);
	
	hash_insert(register_table, "$t9", strlen("$t9"), $t9);
	
	hash_insert(register_table, "$s0", strlen("$s0"), $s0);
	
	hash_insert(register_table, "$s1", strlen("$s1"), $s1);
	
	hash_insert(register_table, "$s2", strlen("$s2"), $s2);
	
	hash_insert(register_table, "$s3", strlen("$s3"), $s3);
	
	hash_insert(register_table, "$s4", strlen("$s4"), $s4);
	
	hash_insert(register_table, "$s5", strlen("$s5"), $s5);
	
	hash_insert(register_table, "$s6", strlen("$s6"), $s6);
	
	hash_insert(register_table, "$s7", strlen("$s7"), $s7);

	hash_insert(register_table, "$gp", strlen("$gp"), $gp);
	
	hash_insert(register_table, "$sp", strlen("$sp"), $sp);
	
	hash_insert(register_table, "$fp", strlen("$fp"), $fp);

	hash_insert(register_table, "$ra", strlen("$ra"), $ra);

	hash_insert(register_table, "$k0", strlen("$k0"), $k0);
	
	hash_insert(register_table, "$k1", strlen("$k1"), $k1);

	hash_insert(register_table, "$at", strlen("$at"), $at);

	hash_insert(register_table, "$zero", strlen("$zero"), $zero);
}
