#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>


#define DEBUG 0


#define ARRAY_BLOCK 128

uint32_t **arrays;
size_t *sizes;
uint32_t collecSize, collecUsed = 0;


uint32_t Reg[] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t instruction;
uint32_t opcode;
uint32_t A, B, C, sA;
uint32_t value;
char input;
uint32_t PC = 0;


struct sStack {
        int32_t top;
        uint32_t size;
        uint32_t *stack;
};

typedef struct sStack STACK;

STACK freedStack;

uint32_t pop;




static unsigned fileSize(const char *file) {
	struct stat sb;

	stat(file, &sb);
	
	return sb.st_size;
}



void *loadROM(uint32_t **arrays, const char *file, size_t size) {
	
	FILE *f;
    
	arrays[0] = calloc(size + 1, sizeof(arrays));
	collecUsed++;
	sizes[0] = size + 1;
	f = fopen(file, "r");
	fread(arrays[0], sizeof(*arrays), size, f);

	fclose(f);
	
	return;
}


void fixEndianess(uint32_t **array) {

        unsigned char swaptest[2] = {1,0};

        if (*(short *)swaptest == 1) {
		int i;
                for (i = 0; i < sizes[0]; i++) {
        
	                arrays[0][i] = ((array[0][i] >> 24) & 0xFF) |   
                                       ((array[0][i] << 8) & 0xFF0000) |       
                                       ((array[0][i] >> 8) & 0xFF00) |       
                                       ((array[0][i] << 24) & 0xFF000000);
                }
        }
}




void stackPush(uint32_t elem) {
	if (freedStack.top  == freedStack.size - 1) {
		freedStack.size++;
		freedStack.stack = realloc(freedStack.stack, freedStack.size * sizeof(freedStack.stack));
	}

        freedStack.top++;	
	freedStack.stack[freedStack.top] = elem;

	return;
}


uint32_t stackPop() {
	if (freedStack.top == -1) {
		return 0xFFFFFFFF;
	} else {
		freedStack.top--;
		return freedStack.stack[freedStack.top + 1];
	}
}
	



int main(int argc, char *argv[]) {
	
	arrays = calloc(ARRAY_BLOCK, sizeof(arrays));
	sizes = calloc(ARRAY_BLOCK, sizeof(sizes));

	freedStack.top = -1;
	freedStack.size = 1;
	freedStack.stack = calloc(freedStack.size, sizeof(freedStack.stack));

	collecSize = ARRAY_BLOCK;
	sizes[0] = fileSize(argv[1]) / 4;

	
	loadROM(arrays, argv[1], sizes[0]);

	fixEndianess(arrays);


	printf("\n == MoebiuZ's UM emulator ==\n\n");


	while(1) {

		instruction = arrays[0][PC];

		opcode = instruction >> 28;


#if DEBUG > 0
char *mnemo[] = {"CMOV", "AIND", "AAM", "ADD", "MUL", "DIV", "NAND", "HALT", "ALLOC", "FREE", "OUT", "IN", "LOAD", "ORT"};


if (opcode <= 13) {
	A = (instruction << 23) >> 29;
        B = (instruction << 26) >> 29; 
        C = (instruction << 29) >> 29;
	sA = (instruction >> 25) & 0B111;
        value = (instruction << 7) >> 7;


	printf("\n%08x %s R%d (%08x), R%d (%08x), R%d (%08x) %08x %80x\n", instruction, mnemo[opcode], A, Reg[A], B, Reg[B], C, Reg[C], sA, value);
}
#endif

		switch(opcode) {

			// CMOV (Conditional MOV)
			case 0:
				A = (instruction >> 6) & 0B111;
	        		B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;

				if (Reg[C] != 0) {
					Reg[A] = Reg[B];
	 			}

				PC++;
				break; 
 		
			// AIND  (Index Array)
			case 1: 

				A = (instruction >> 6) & 0B111;
			        B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;
	
				if (arrays[Reg[B]] != NULL) {
					if (Reg[C] < sizes[Reg[B]]) {
						Reg[A] = arrays[Reg[B]][Reg[C]];
					}

				}
				
				PC++;
				break;
 	
			// AAM  (Amend Array)
			case 2: 
			
				A = (instruction >> 6) & 0B111;
			        B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;
				
				if (arrays[Reg[A]] != NULL) {
					if (Reg[B] < sizes[Reg[A]]) {
						arrays[Reg[A]][Reg[B]] = Reg[C];
					}
				}
				
				PC++;
				break;

			// ADD
			case 3: 
			
				A = (instruction >> 6) & 0B111;
			        B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;

				Reg[A] = Reg[B] + Reg[C];

				PC++;
				break;

			// MUL
		 	case 4:
			
				A = (instruction >> 6) & 0B111;
       				B = (instruction >> 3) & 0B111;
  		        	C = instruction & 0B111;

				Reg[A] = Reg[B] * Reg[C];

				PC++;
				break;
 		
			// DIV
			case 5: 

				A = (instruction >> 6) & 0B111;
        			B = (instruction >> 3) & 0B111;
       				C = instruction & 0B111;

				if (Reg[C] == 0) {
					printf("\nMachine Fail: Division by 0\n");
					return 1;
				}

				Reg[A] = Reg[B] / Reg[C];

				PC++;
				break;

			// NAND
		 	case 6: 

				A = (instruction >> 6) & 0B111;
        			B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;


				Reg[A] = (~Reg[B]) | (~Reg[C]);

				PC++;
				break;
			// HALT
		 	case 7: 

				printf("\n == Machine halted ==\n");
				return 2;

			// ALLOC
		 	case 8: 

        			B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;
	
				if (Reg[C] != 0) {

					pop = stackPop();
					
					if (pop == 0xFFFFFFFF) {

						if (collecUsed == collecSize) {
							collecSize += ARRAY_BLOCK;
							arrays = realloc(arrays,  collecSize * sizeof(arrays));
							sizes = realloc(sizes, collecSize * sizeof(sizes));
						}
						
						arrays[collecUsed] = calloc(Reg[C], sizeof(arrays));
						sizes[collecUsed] = Reg[C];
						Reg[B] = collecUsed;


					} else {

						arrays[pop] = calloc(Reg[C], sizeof(arrays));
						sizes[pop] = Reg[C];
						Reg[B] = pop;

					}

					collecUsed++;
				}
				

				PC++;
				break;



			// FREE (Abandon array)
	 		case 9:

        			C = instruction & 0B111;


				if (Reg[C] != 0) {
					if (Reg[C] < collecSize) {
						if (arrays[Reg[C]] != NULL) {
						
							free(arrays[Reg[C]]);
							stackPush(Reg[C]);
							arrays[Reg[C]] = NULL;
							collecUsed--;
						}			
					}	
				}

				PC++;
				break;

			// OUT 
		 	case 10:

        			C = instruction & 0B111;

				putchar(Reg[C]);

				PC++;
				break;

			// IN
		 	case 11:

        			C = instruction & 0B111;

				input = getchar();

				if (input == 13) {
					Reg[C] = 0xFFFFFFFF;
				} else {
					Reg[C] = (uint32_t) input;
				}

				PC++;
				break;

			// LOAD / JMP
			case 12:

        			B = (instruction >> 3) & 0B111;
        			C = instruction & 0B111;

				if (Reg[B] != 0) {

					if (arrays[Reg[B]] != NULL) {

						free(arrays[0]);
						arrays[0] = malloc(sizes[Reg[B]] * sizeof(arrays));
						memcpy(arrays[0], arrays[Reg[B]], sizes[Reg[B]]* sizeof(arrays));
						sizes[0] = sizes[Reg[B]];
			
						PC = Reg[C];

					} else {

						printf("\nMachine Fail: NULL array\n");

					}

				} else {

					PC = Reg[C];

				}	

				break;

			// ORT
	 		case 13:
		
		        	sA = (instruction >> 25) & 0B111;
			        value = (instruction << 7) >> 7;
				
				Reg[sA] = value;
				
				PC++;
				break;
	
			default:
	
				printf("\nMachine Fail: Invalid opcode\n");
				return 1;
		}


	}

	return 0; 
}

