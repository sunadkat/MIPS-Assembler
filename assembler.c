#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include "tokenizer.h"
#include "hash_table.h"
#include "initialization.h"
#include "utilities.h"

#define MAX_LINE_LENGTH 256
#define DATA_SEGMENT_START_ADDRESS 8192
#define TEXT_SEGMENT_START_ADDRESS 0
#define TRUE 1
#define FALSE 0

/*
 * =====================================================================================
 *
 * Filename:  newAssembler.c
 *
 * Description: Assembler. Processes a given file containing MIPS Assembly code and
 * translates it into machine code. Input is recieved from a file specified in the
 * command line and output is stored in a with the name given as the second argument.
 *	
 * Invoked as: assembler <input file> <output file>
 *
 * Author:  Karthik Kumar, kkumar91@vt.edu
 *
 * =====================================================================================
 */

void first_pass(char *src_file);

void second_pass(char *src_file, char *dest_file);

char* process_r_type_instr(char* inst, char* rt, char* rs, char* rd);

char* process_i_type_instr(char* token, char* rt, char* rs, char* imm, int32_t pc);

char* process_j_type_instr(char* token, char* imm);

char* process_psuedo_instr(char* token, char* r, char* label);

char* parse_asciiz(char* str, int size);

void destroy();

hash_table_t *code_table;

hash_table_t *register_table;

hash_table_t *symbol_table;

int32_t *instr_ptr;

/*
 * ============================================================================
 * Main function. Gets the arguments from the command line. Creates three 
 * hashtables-one for the opcodes, one for the register, and one for the
 * symbol table. It calls three functions: zeroth pass (which handles the
 * extra credit), first pass (which handles putting the labels into the symbol 
 * table) and second pass (which prints out the output to the specified file).
 *
 *=============================================================================
 */
int32_t main(int argc, char *argv[])
{
	if (argc < 3)
	{
		// Print error message if we dont have two file names as the parameter.
		printf("Usage: %s <input file> <output file>\n", argv[0]);
		return -1;
	}
	
	// Create and initialize a hash table that will have opcodes for all the instructions we need to represent.
	code_table = create_hash_table(127);
	init_opcodes_table(code_table);
	if (code_table == NULL)
	{
		printf("ERROR: Could not create a opcodes hashtable. Aborting...\n");
		destroy();
	}

	// Create and initialize another hash table that will have the numbers for the registers.
	register_table = create_hash_table(31);
	init_register_table(register_table);
	if (register_table == NULL)
	{
		printf("ERROR: Could not create a registers hashtable. Aborting...\n");
		destroy();
	}

	instr_ptr = (int32_t*)(malloc(sizeof(int32_t)));
	if (instr_ptr == NULL)
	{	
		printf("ERROR: Cannot allocate memory for instruction pointer");
		destroy();
	}

	// Create a hash table that will hold labels and the corresponding address.
	symbol_table = create_hash_table(127);

	// Zeroth pass will do the extra credit - it organizies the file into one text and on data section
	zeroth_pass(argv[1]);	

	// Handles the symbol table of address for the labels.
	first_pass(TEMP_FILE_NAME);

	// Handles the output of the assembler
	second_pass(TEMP_FILE_NAME , argv[2]);

	//fclose(temp_file);
	
	// After we finished, delete the temp file we created for the extra credit
	if( remove(TEMP_FILE_NAME ) != 0 )
		printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  	else
    	printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );

	// Destroy hash tables we created.
	destroy_hash_table(code_table);
	destroy_hash_table(register_table);
	destroy_hash_table(symbol_table);

	free(instr_ptr);

	printf("Assembler successfully finished assembling %s. Result is in %s\n", argv[1], argv[2]);

	return 0;
}

void destroy()
{
	// Destroy hash tables we created.
	destroy_hash_table(code_table);
	destroy_hash_table(register_table);
	destroy_hash_table(symbol_table);

	exit(-1);
}

/*
 * ============================================================================
 * This function perfoms the first pass through the source file. It looks through
 * the .text section and adds the addresses of all the labels to the 
 * symbol_table hashtable. It then looks through the .data sections and adds 
 * the address of the data into the same hashtable.
 * ============================================================================
 */
void first_pass(char *src_file)
{
	char line[MAX_LINE_LENGTH + 1];
	char *tok_ptr, *ret, *found, *token = NULL;
	FILE *fptr;
	int32_t text_part = FALSE;
	int32_t data_part = FALSE;

	// Open the file for reading.
	fptr = fopen(src_file, "r");
	if (fptr == NULL)
	{
		// Check to see if we were able to open the file successfully.
		printf("ERROR: Unable to open file %s. Aborting...\n", src_file);
		destroy();
	}
	while (1)
	{
		// Loop thorugh until we reach either .data or .text segments
		if ((ret = fgets(line, MAX_LINE_LENGTH, fptr)) == NULL) 
			break;
		line[MAX_LINE_LENGTH] = 0;

		tok_ptr = line;

		token = parse_token(tok_ptr, " ()\n\t\r,#", &tok_ptr, NULL);
		
		if (token == NULL || *token =='#')
		{
			// If we had no token or if it was a comment, ignore it
			continue;
		}
		if (strcmp(token, ".text") == 0 || strcmp(token, ".data") == 0)
			break;
	}
	while (token != NULL)
	{
		if (strcmp(token, ".text") == 0) 
		{
			text_part = TRUE;
			*instr_ptr = TEXT_SEGMENT_START_ADDRESS;
			// Look at each line and parse the tokens in that line
			while (1)
			{
				if ((ret = fgets(line, MAX_LINE_LENGTH, fptr)) == NULL) 
					break;
			
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;

				token = parse_token(tok_ptr, " ()\n\t,\r", &tok_ptr, NULL);
				if (token == NULL || *token =='#')
				{
					// If we had no token or if it was a comment, ignore it
					continue;
				}

				// Try to look up the token in the opcode table
				found = (char*) (hash_find(code_table, token, strlen(token)));
				if (found == NULL) 
				{
					// If the token was not in the opcode table, look up in the register table
					found = (char*) (hash_find(register_table, token, strlen(token)));
					if (found != NULL)
					{
						// If the token is a register or a constant, ignore
						break;
					}
				}
				// See if the token is a label
				if (strchr(token, ':'))
				{
					// If the token has a colon, then it is a label
					// Since it is a label, we store the address we are at into symbol_table
					char* removed_colon = malloc(sizeof(token) - 1);
					if (removed_colon == NULL)
					{
						// Check to see if malloc failed.
						printf("ERROR: Unable to allocate memory. Aborting...\n");
						destroy();	
					}

					strncpy (removed_colon, token, strlen(token) - 1);

					// Try to look up the label in the opcode table
					char* find = malloc(sizeof(token) - 1);

					if (find == NULL)
					{
						// Check to see if malloc failed.
						printf("ERROR: Unable to allocate memory. Aborting...\n");
						destroy();	
					}

					find = (char*) (hash_find(code_table, removed_colon, strlen(removed_colon)));

					if (find != NULL) 
					{
						// If the label was in the opcode table, throw an error, a label can't be the same as 
						// an instruction
						printf("ERROR: Label %s is the same as an opcode. Aborting...\n", removed_colon);
						destroy();
					}

					find = (char*) (hash_find(register_table, removed_colon, strlen(removed_colon)));

					if (find != NULL)
					{
						// If the label was the same as a register name, throw an error
						printf("ERROR: Label %s is the same as a register name. Aborting...\n", removed_colon);
						destroy();
					}

					// Convert the instruction pointer into a string and insert it into the symbol_table
					char* to_insert = (char*)(malloc(sizeof(char) * 256));
					if (to_insert == NULL)
					{
						// Check to see if malloc failed.
						printf("ERROR: Unable to allocate memory. Aborting...\n");
						destroy();	
					}

					// Convert the instruction pointer to a char* to insert into the table. 
					sprintf(to_insert, "%d", *(int32_t*) instr_ptr);

					if (hash_insert(symbol_table, removed_colon, strlen(removed_colon), to_insert) == FALSE)
					{
						printf("Inserting into the hash table failed. Aborting...\n");
						destroy();
					}

					//int ans =  atoi((char*)hash_find(symbol_table, removed_colon, strlen(removed_colon)));
					free(removed_colon);
				}
				if (strcmp(token, "la") == 0)
				{
					// If the instruction is la, then increment the instr_ptr by 8
					*instr_ptr += 8;
				}
				else if (found != NULL)
				{
					// If the token is an instruction or a register, increment instr_ptr by 4
					*instr_ptr += 4;
				}
				else if (strcmp(token, ".data") == 0)
				{
					// Break out of the loop if we have data
					break;
				}
				else if(strcmp(token, "nop") == 0)
				{
					// Break out if we have a nop
					break;
				}
			}
		}

		// Code to handle the .data section
		else if (strcmp(token, ".data") == 0 )
		{
			data_part = TRUE;
			*instr_ptr = DATA_SEGMENT_START_ADDRESS;
			while (1) 
			{		
	 			if ((ret = fgets(line, MAX_LINE_LENGTH, fptr)) == NULL) 
					break;
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;

				token = parse_token(tok_ptr, "\n\r", &tok_ptr, NULL);

				if (token == NULL || *token =='#')
				{
					// If we had no token or if it was a comment, ignore it
					continue;
				}
				// Handle string data
				if (strstr(token, ".asciiz") != NULL)
				{
					// First, get the label of the token by tokenizing with a colon
					char* label = strtok(token, " \t:");
					while (token != NULL) 
					{		
						// The, get the string itself by parsing by spaces first and then by quotes
						token = strtok(NULL, "\t ");
						token = strtok(NULL, "\"");

						// The amount of space we need to store this string is given by this formula
						int num = (int)ceil((strlen(token) + 1) / 4.0);

						// Convert the instruction pointer to a string and insert it into our symbol table
						char* to_insert = (char*)(malloc(sizeof(char) * 256));
						if (to_insert == NULL)
						{
							// Check to see if malloc failed.
							printf("ERROR: Unable to allocate memory. Aborting...\n");
							destroy();	
						}
						sprintf(to_insert, "%d", *(int32_t*) instr_ptr);

						if (hash_insert(symbol_table, label, strlen(label), to_insert) == FALSE)
						{
							printf("ERROR: Count not insert into a hash table. Aborting...\n");
							destroy();
						}

						// Increment the instr_ptr by the amount of space we need
						*instr_ptr += (num * 4);
						break;										
					}		
				}
				// Handle word or array data.
				else if (strstr(token, ".word") != NULL)
				{
					// First, find the number of colons to see if it was an array or just an int	
					int numOfColons = count_num_occurances(line, ':');
					if (numOfColons == 1) 
					{
						// if it was just an int, parse by colon to get the label and increment the instr_ptr by four
						char* label = strtok(token, " \t:");
						char* to_insert = (char*)(malloc(sizeof(char) * 256));
						if (to_insert == NULL)
						{
							// Check to see if malloc failed.
							printf("ERROR: Unable to allocate memory. Aborting...\n");
							destroy();	
						}

						// Convert the instruction pointer to a string before inserting
						sprintf(to_insert, "%d", *(int32_t*) instr_ptr);

						if (hash_insert(symbol_table, label, strlen(label), to_insert) == FALSE)
						{
							printf("ERROR: Count not insert into a hash table. Aborting...\n");
							destroy();
						}

						*instr_ptr += 4;
					}
					else if (numOfColons > 1)
					{
						// If we had a array, parse by colon to get the label and loop thorugh to get the size 
						char* label = strtok(token, " \t:");
						while (token != NULL)
						{
	  						token = strtok(NULL, "\n\t");
	  						if (token == NULL || *token == '#') 
	    					{
								break;
	    					}
							if (strchr(token, ':'))
							{
								// This first variable is not used, it will sipmly get the first number
								int initialize = atoi(strtok(token, ":"));
								initialize = 42;
								
								// This variable converts a string to an int to get the size of the array
								int size = atoi(strtok(NULL, ":"));
								char* to_insert = (char*)(malloc(sizeof(char) * 256));
								if (to_insert == NULL)
								{
									// Check to see if malloc failed.
									printf("ERROR: Unable to allocate memory. Aborting...\n");
									destroy();	
								}
								sprintf(to_insert, "%d", *(int32_t*) instr_ptr);

								if (hash_insert(symbol_table, label, strlen(label), to_insert) == FALSE)
								{
									printf("ERROR: Count not insert into a hash table. Aborting...\n");
									destroy();
								}

								//int ans =  atoi((char*)hash_find(symbol_table, label, strlen(label)));

								// Increment the instruction pointer by 4 times the number of elements we are storing

								*instr_ptr += (size * 4);
							}	
						}
					}
				}	
				// Get the next data element until we are done reading the file	
				while (token != NULL)
				{
  					token = parse_token(tok_ptr, " \n\t\r", &tok_ptr, NULL);
  					if (token == NULL || *token == '#') 
						/* blank line or comment begins here. go to the next line */
	    				{
					    break;
	    				}

				}
			}
		}
		// Exit the first pass if we have done both text and data segments
		if (text_part == TRUE && data_part == TRUE)
			break;
		
		// Exit the first pass if we have a nop and we finished doing the data part
		else if(strcmp(token, "nop") == 0 && data_part == TRUE)
		{
			break;
		}

		// If we have reached a nop and we haven't done data yet, keep looping until we
		// find ".data"
		else if (strcmp(token, "nop") == 0 && data_part == FALSE)
		{
			while (token != NULL) 
			{
				if ((ret = fgets(line, MAX_LINE_LENGTH, fptr)) == NULL) 
					break;
			
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;

				token = parse_token(tok_ptr, " ()\n\t,\r", &tok_ptr, NULL);
				if (token == NULL || *token =='#')
				{
					// If we had no token or if it was a comment, ignore it
					continue;
				}
				else
					break;
							
			}
		}
	}
	free(token);
	fclose(fptr);
	printf("First pass completed\n");
}

/*
 * ============================================================================
 * Performs the second pass of the assembly process. It looks thorugh the file,
 * decodes the instruction, classifies it as either r-type, j-type or i-type 
 * and processes it based on that. Then it reads through the data sections
 * and converts it into binary. The second argument to this function is the
 * file we are writing the assembled code to.
 * ============================================================================
 */

void second_pass(char *src_file, char* dest_file)
{
	char line[MAX_LINE_LENGTH + 1];
  	char *tok_ptr, *found, *ret, *token = NULL;
  	FILE *src_fptr;
	FILE *dest_fptr;	
	int32_t text_part = FALSE;
	int32_t data_part = FALSE;	

  	src_fptr = fopen(src_file, "r");
	dest_fptr = fopen(dest_file, "w");

 	if (src_fptr == NULL)
	{
		printf("Unable to open input file %s. Aborting...\n", src_file);
   		destroy();
   	}
 	if (dest_fptr == NULL)
   	{
 		printf("Unable to create output file %s. Aborting...\n", dest_file);
   	  	destroy();
   	}

	while (1)
 	{
		// Loop thorough until we find a .data or a .text segment
      		if ((ret = fgets(line, MAX_LINE_LENGTH, src_fptr)) == NULL)
			break;
    	
		line[MAX_LINE_LENGTH] = 0;

		tok_ptr = line;

		token = parse_token(tok_ptr, " ()\n\t\r,", &tok_ptr, NULL);
		if (token == NULL || *token =='#')
		{
			// If we had no token or if it was a comment, ignore it
			continue;
		}
		if (strcmp(token, ".text") == 0 || strcmp(token, ".data") == 0)
			break;

   	} 			
		/* parse the tokens within a line */
	while (token != NULL)
   	{
		if (strcmp(token, ".text") == 0)
		{
			text_part = TRUE;
			*instr_ptr = TEXT_SEGMENT_START_ADDRESS;
			while (1)
			{
				if ((ret = fgets(line, MAX_LINE_LENGTH, src_fptr)) == NULL) 
					break;
			
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;

				// Parse the token with new line, tab, return, comma, space or ()
				token = parse_token(tok_ptr, " ()\n\t,\r", &tok_ptr, NULL);
				if (token == NULL || *token =='#')
				{
					// If we had no token or if it was a comment, ignore it
					continue;
				}
				
				// Look in our op code table to see if it is an instrction we know
				found = (char*) (hash_find(code_table, token, strlen(token)));
				if (found != NULL) 
				{
					// Output will contain the string we write to the file
					char *output = (char*)(malloc(33));
					if (output == NULL)
					{
						// Check to see if malloc failed.
						printf("ERROR: Unable to allocate memory. Aborting...\n");
						destroy();	
					}
					if (strcmp(token, "jr") == 0)
					{
						// If our instruction is a jr, we only need to get one register, the other arguments
						// to our process_r_type_instr functions are null
						char* rs = parse_token(tok_ptr, " ,\t\n#", &tok_ptr, NULL);

						output = process_r_type_instr(token, rs, NULL, NULL);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    								printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    								printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (strcmp(token, "add") == 0 || strcmp(token, "sub") == 0 || strcmp(token, "or") == 0 ||
						strcmp(token, "and") == 0 || strcmp(token, "slt") == 0 || strcmp(token, "sll") == 0 ||
						strcmp(token, "srl") == 0)
					{
						// All the other r type instructions need three registers to be processed
						char* rs = parse_token(tok_ptr, " ,\t\n\r", &tok_ptr, NULL);
						char* rt = parse_token(tok_ptr, " ,\t\n\r", &tok_ptr, NULL);
						char* rd = parse_token(tok_ptr, " ,\t\n\r#", &tok_ptr, NULL);

						output = process_r_type_instr(token, rs, rt, rd);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    							printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    								printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						// Write the output to the file, increment the instr_ptr
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (strcmp(token, "addi") == 0 || strcmp(token, "ori") == 0 || strcmp(token, "andi") == 0
						|| strcmp(token, "slti") == 0 || strcmp(token, "beq") == 0 || strcmp(token, "bne") == 0)
					{
						// These i-type instrucions need two registers and an immediate field to be parserd. I also
						// pass in the current instruction pointer to calculate offsets for branches
						char* rs = parse_token(tok_ptr, " ,\t\n\r", &tok_ptr, NULL);
						char* rt = parse_token(tok_ptr, " ,\t\n\r", &tok_ptr, NULL);
						char* imm = parse_token(tok_ptr, " ,\t\n\r#", &tok_ptr, NULL);

						int32_t pc = *instr_ptr;

						output = process_i_type_instr(token, rs, rt, imm, pc);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    								printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    								printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (strcmp(token, "lw") == 0 || strcmp(token, "sw") == 0)
					{
						// For lw and sw we need to get the dest register, source register and the immediate offset
						char* rs = parse_token(tok_ptr, " ,()\t\n", &tok_ptr, NULL);
						char* imm = parse_token(tok_ptr, " ,()\t\n", &tok_ptr, NULL);
						char* rt = parse_token(tok_ptr, " ,()\t\n#", &tok_ptr, NULL);

						int32_t pc = *instr_ptr;
						output = process_i_type_instr(token, rs, rt, imm, pc);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    								printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    							printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (strcmp(token, "j") == 0 || strcmp(token, "jal") == 0)
					{
						// For jal and j, we just need the label we are jumping too, and our current instruction pointer
						char* rs = parse_token(tok_ptr, " ,()\t\n#", &tok_ptr, NULL);

						output = process_j_type_instr(token, rs);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    								printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    							printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (strcmp(token, "la") == 0)
					{
						// For la, get the label of the address we are tyring to load and the register we want to load it to
						char* r = parse_token(tok_ptr, " ,()\t\n#\r", &tok_ptr, NULL);
						char* label = parse_token(tok_ptr, " ,()\t\n\r#", &tok_ptr, NULL);

						output = process_psuedo_instr(token, r, label);
						if (output == NULL)
						{
							fclose(dest_fptr);
							// If we got an error, close and delete the file
							if( remove(dest_file  ) != 0 )
								printf( "Error deleting file %s. \n", dest_file );
  							else
    							printf( "File %s successfully deleted. \n", dest_file );
							if( remove(TEMP_FILE_NAME ) != 0 )
								printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  							else
    							printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
							destroy();
						}
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 8;
					}

						free(output);
				}

				else if (strcmp(token, "nop") == 0)
				{
					// If the instrucition was just a nop, print out 32 0s

					*instr_ptr += 4;
					char* output = int32_to_bin(0, 32);

					strcat(output, "\n");
					fputs(output, dest_fptr);
				}
				else if (strcmp(token, ".data") == 0)
				{
					break;
				}
				else if (strchr(token, ':'))
				{
					// We ignore labels, so we do nothing here
				}
				else
				{
					// If the instruction we not one of the one we know, throw an error, close and delete output file
					printf("ERROR: Instruction %s not found. Aborting...\n", token);
					fclose(dest_fptr);
					// If we got an error, close and delete the file
					if( remove(dest_file  ) != 0 )
						printf( "Error deleting file %s. \n", dest_file );
					else
						printf( "File %s successfully deleted. \n", dest_file );
					if( remove(TEMP_FILE_NAME ) != 0 )
						printf( "Error deleting file %s. \n", TEMP_FILE_NAME );
  					else
    						printf( "File %s successfully deleted. \n", TEMP_FILE_NAME );
					destroy();
				}
			}
		}
		
		else if (strcmp(token, ".data") == 0)
		{
			fputs("\n", dest_fptr);
			data_part = TRUE;
			*instr_ptr = DATA_SEGMENT_START_ADDRESS;
			while (1)
			{
				if ((ret = fgets(line, MAX_LINE_LENGTH, src_fptr)) == NULL)
					break;
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;

				token = parse_token(tok_ptr, "\n", &tok_ptr, NULL);
				if (token == NULL || *token == '#')
				{
					continue;
				}
				if (strstr(token, ".asciiz") != NULL)
				{
					strtok(token, ":");
					while (token != NULL)
					{
						// Get the string by first parsing by spaces and then by quotes
						token = strtok(NULL, "\t ");
						token = strtok(NULL, "\"");
						if (token == NULL)
							break;

						int num = (int)ceil((strlen(token) + 1) / 4.0);
						
						char* output = parse_asciiz(token, num);
						fputs(output, dest_fptr);
						*instr_ptr += num;
					}
				
				}
				else if (strstr(token, ".word") != NULL)
				{
					// First, find the number of colons to see if it was an array or just an int
					int numOfColons = count_num_occurances(line, ':');
					if (numOfColons == 1)
					{
						// If it was just a number, convert that number to binary and ouput it
					    strtok(token, ":");
						strtok(NULL, ".word");
						char* amount = strtok(NULL, ".word \t");
						int32_t value = (int32_t)(atoi(amount));
						char* output = int32_to_bin(value, 32);
						strcat(output, "\n");
						fputs(output, dest_fptr);
						*instr_ptr += 4;
					}
					else if (numOfColons > 1)
					{
						// If we had a array, parse by colon to get the label and loop thorugh to get the size 
						strtok(token, ":");
						while (token != NULL)
						{
	  						token = strtok(NULL, "\n\t");
	  						if (token == NULL || *token == '#') 
	    					{
								break;
	    					}
							if (strchr(token, ':'))
							{
								// This first variable gets the first number, which is the initial value for each element
								int32_t initial_value = atoi(strtok(token, ":"));
								
								// This variable converts a string to an int to get the size of the array
								int size = atoi(strtok(NULL, ":"));
							
								char* value = int32_to_bin(initial_value, 32);
								int i = 0;
								// Loop through putting each value size times
								for (i = 0; i < size; i++)
								{
									fputs(value, dest_fptr);
									fputs("\n", dest_fptr);
								}

								// Increment the instruction pointer by 4 times the number of elements we are storing
								*instr_ptr += (size * 4);
							}	
					
						}	
					
					}
				}
			}
			break;
		}
		if (text_part == TRUE && data_part == TRUE)
				break;
		else if(strcmp(token, "nop") == 0 && data_part == TRUE)
		{
			break;
		}
		else if (strcmp(token, "nop") == 0 && data_part == FALSE)
		{
			while (1) 
			{
				if ((ret = fgets(line, MAX_LINE_LENGTH, src_fptr)) == NULL) 
					break;
			
				line[MAX_LINE_LENGTH] = 0;
				tok_ptr = line;
				if (strlen(line) == MAX_LINE_LENGTH)
				{
					printf("Line in %s is too long, ignoring line...\n", src_file);
					continue;
				}

				token = parse_token(tok_ptr, " ()\n\t,", &tok_ptr, NULL);
				if (token == NULL || *token =='#')
				{
					// If we had no token or if it was a comment, ignore it
					continue;
				}
				else
					break;
							
			}
		}
	}
  	fclose(src_fptr);
	fclose(dest_fptr);
	printf("Second pass completed\n");
}

/*
 * =============================================================================
 * Process r type instructions. Need the instruction and three registers - rs, 
 * rt, rd. 
 * =============================================================================
 */
char* process_r_type_instr(char* inst, char* rs, char* rt, char* rd)
{
	char *op_code = "000000";
	char *src1, *src2, *dest, *shamt, *funct = NULL;
	char *output = (char*)(malloc(33));
	if (output == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}
	if (strcmp(inst, "jr") == 0)
	{
		// For jr we only need the register we are jumping to (rs)
		if (rs == NULL)
		{
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		}
		src1 = (char*)(hash_find(register_table, rs, strlen(rs)));
		if (src1 == NULL)
		{
			printf("ERROR: Cannot parse command %s, Incorrect arguments. Aborting...\n", inst);	
			return NULL;
		}
		funct = (char*)(hash_find(code_table, inst, strlen(inst)));
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, funct);

	}
	// If any of the given registers are null, we have wrong arguments for this instruction
	else if (rt == NULL || rs == NULL || rd == NULL)
	{
		printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
		return NULL;
	}
	else if (strcmp(inst, "add") == 0 || strcmp(inst, "sub") == 0 || strcmp(inst, "or") == 0 ||
		strcmp(inst, "and") == 0 || strcmp(inst, "slt") == 0)
	{
		// For these instructions, we need all three registers, and a function, which identifies it 
		src1 = (char*)(hash_find(register_table, rt, strlen(rt)));
		src2 = (char*)(hash_find(register_table, rd, strlen(rd)));
		dest = (char*)(hash_find(register_table, rs, strlen(rs))); 
		if (src1 == NULL || src2 == NULL || dest == NULL)
		{
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		}
		shamt = "00000";
		
		funct = (char*)(hash_find(code_table, inst, strlen(inst)));
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, src2);
		strcat(output, dest);
		strcat(output, shamt);
		strcat(output, funct);
	}
	else if (strcmp(inst, "sll") == 0 || strcmp(inst, "srl") == 0)
	{
		// For these instructions, we need two registers, and a shift amount
		src1 = "00000";
		src2 = (char*)(hash_find(register_table, rt, strlen(rt)));
		dest = (char*)(hash_find(register_table, rs, strlen(rs)));
 		if (src2 == NULL || dest == NULL)
		{
			
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		} 
		shamt = int32_to_bin(atoi(rd), 5);
		funct = (char*)(hash_find(code_table, inst, strlen(inst)));
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, src2);
		strcat(output, dest);
		strcat(output, shamt);
		strcat(output, funct);
	}
	return output;
}

/*
 * ==================================================================================
 * For the i-type instructions, we need the instructions, two registers and an
 * immediate field. We also pass in the current pc to calculate offsets
 * ==================================================================================
 */
char* process_i_type_instr(char* inst, char* rs, char* rt, char* imm, int32_t pc)
{
	char *op_code, *src1, *src2, *immediate = NULL;
	char* output = (char*)malloc(33);
	if (output == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}
	if (rt == NULL || rs == NULL || imm == NULL)
	{
		printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
		return NULL;
	}
	else if (strcmp(inst, "addi") == 0 || strcmp(inst, "ori") == 0 || 
		strcmp(inst, "andi") == 0 || strcmp(inst, "slti") == 0 )
	{
		// get the opcode, registers and the imm field
		op_code = (char*)(hash_find(code_table, inst, strlen(inst)));
		src1 = (char*)(hash_find(register_table, rt, strlen(rt)));
		src2 = (char*)(hash_find(register_table, rs, strlen(rs)));
		immediate = int32_to_bin(atoi(imm), 16);
		if (src1 == NULL || src2 == NULL)
		{
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		}
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, src2);
		strcat(output, immediate);
	}

	else if (strcmp(inst, "bne") == 0 || strcmp(inst, "beq") == 0)
	{
		// For these instructions, we need the opcode, two registers and the
		// offset from the current pc to the location of the label
		op_code = (char*)(hash_find(code_table, inst, strlen(inst)));
		src1 = (char*)(hash_find(register_table, rs, strlen(rs)));
		src2 = (char*)(hash_find(register_table, rt, strlen(rt)));
		
		char* found = (char*) hash_find(symbol_table, imm, strlen(imm));
		if (found == NULL)
		{	
			printf("ERROR: Cannot find label %s. Aborting...\n", imm);
			return NULL;
		}
		int32_t addr_of_label = (int32_t)(atoi((found)));

		int32_t offset = (addr_of_label - (4 + pc)) / 4;

		char* ans =  int32_to_bin(offset, 16);

		if (src1 == NULL || src2 == NULL)
		{
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		}
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, src2);
		strcat(output, ans);
	}
	else if (strcmp(inst, "lw") == 0 || strcmp(inst, "sw") == 0)
	{
		// For these two instructions, we need the opcode, a source register, destination register,
		// and a immediate field for the offset
		op_code = (char*)(hash_find(code_table, inst, strlen(inst)));
		src1 = (char*)(hash_find(register_table, rt, strlen(rt)));
		immediate = int32_to_bin(atoi(imm), 16);
		src2 = (char*)(hash_find(register_table, rs, strlen(rs)));
		if (src1 == NULL || src2 == NULL)
		{
			printf("ERROR: Cannot parse command %s. Incorrect arguments. Aborting...\n", inst);
			return NULL;
		}
		strcat(output, op_code);
		strcat(output, src1);
		strcat(output, src2);
		strcat(output, immediate);
	}
	return output;
}

/* 
 * ================================================================================
 * This type of instruction need only an immediate field which contains the label
 * we are jumping to.
 * ================================================================================
 */
char* process_j_type_instr(char* inst, char* imm)
{
	char *op_code;
	char* output = (char*) malloc(33);
	
	if (output == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}
	op_code = (char*)(hash_find(code_table, inst, strlen(inst)));
	if (imm == NULL)
	{
		printf("ERROR: Cannot find label %s. Aborting...\n", imm);
		return NULL;
	}
	char* addr = (char*) hash_find(symbol_table, imm, strlen(imm));
	if (addr == NULL)
	{
		printf("ERROR: Cannot find label %s. Aborting...\n", imm);
		return NULL;
	}
	int32_t offset = (int32_t)(atoi(addr));
	char* ans =  int32_to_bin(offset / 4 , 26);

	strcat(output, op_code);
	strcat(output, ans);
	return output;

}
/*
 * ==================================================================================================
 * Function to process the "la" psuedo instruction. It takes a register and a label as the argument.
 * It looks up in the register table for the register and looks into the symbol table for the address
 * of the given label. Then, it creates two instructions: lui and ori, each with the given register as
 * one of the arguments. For lui we give it bits 16:31 of the offset. For ori, we give it bits 0:15
 * of the offset.
 * ===================================================================================================
 */
char* process_psuedo_instr(char* inst, char* r, char* label)	
{
	char *op_code_lui, *op_code_ori, *reg;
	char* output = (char*) malloc(66);
	if (output == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}
	op_code_lui = (char*)(hash_find(code_table, "lui", strlen("lui")));
	op_code_ori = (char*)(hash_find(code_table, "ori", strlen("ori")));
	if (r == NULL)
	{
		printf("ERROR: Incorrect arguments for instruction %s. Aborting...\n", inst);
		return NULL;
	}
	reg = (char*)(hash_find(register_table, r, strlen(r)));
	
	char* got = (char*) hash_find(symbol_table, label, strlen(label));
	if (got == NULL)
	{
		printf("ERROR: Cannot find label %s. Aborting...\n", label);
		return NULL;
	}	
	int32_t offset = (int32_t)(atoi(got));
	
	char* ans = int32_to_bin(offset, 32);
	if (ans == NULL)
	{
		printf("ERROR: Cannot find label %s. Aborting...\n", label);
		return NULL;
	}
	char *lui_offset = (char*)malloc(17);
	if (lui_offset == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}
	char *ori_offset = (char*)malloc(17);
	if (ori_offset == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}

	// Copy over the top 16 bits of the offset onto the lui instruction
	strncpy(lui_offset, ans, 16);
	int32_t i = 0;
	while (i < 16) {
		ans++;
		i++;
	}
		
	// Copy over the bottom 16 bits of the offset onto te ori instruction
	strncpy(ori_offset, ans, 16);

	// Null terminate both instructions
	lui_offset[strlen(lui_offset)] = '\0';
	ori_offset[strlen(ori_offset)] = '\0';

	strcat(output, op_code_lui);
	strcat(output, "00000");
	strcat(output, reg);
	strcat(output, lui_offset);
	strcat(output, "\n");

	strcat(output, op_code_ori);
	strcat(output, reg);
	strcat(output, reg);
	strcat(output, ori_offset);

	free(lui_offset);
	free(ori_offset);
	return output;
}
/*
 * ==============================================================
 * Parses a string. It takes in the string to parse and the size
 * (the number of lines it will take, where lines means the number 
 * of lines in the ouput file-the number of 32 bit lines that we
 * need to represent this string.
 * ==============================================================
 */
char* parse_asciiz(char* str, int size)
{
	char* output = (char*)(malloc(size * 34));
	if (output == NULL)
	{
		// Check to see if malloc failed.
		printf("ERROR: Unable to allocate memory. Aborting...\n");
		destroy();	
	}

	// value holds the value of each character
	unsigned int value = 0;

	// count keeps track of how many chars we have stored in one int
	int count = 0;

	// looping through the string
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		// Get the char we are looking at now
		int temp = str[i];
		if (temp == 0)
		{
			// Break if have reached the end of the string
			break;
		}
		// This formula keeps adding new characters (in ASCII) to the end of our value
		value = value | (temp << 8 * count);
		count++;

		if (count == 4)
		{
			// Once we have put four chars in one line, convert it to binary, reset counts
			char* bits = int32_to_bin(value, 32);
			strcat(output, bits);
			strcat(output, "\n");
			value = 0;
			count = 0;
		}
	}
	// Convert the remaining characters
	char* bits = int32_to_bin(value, 32);	
	
	strcat(output, bits);
	strcat(output, "\n");
	strcat(output, "\0");
	return output;
}
