#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

typedef void(*funcPtr)(char *);
typedef struct _History {
	char hCmd[100];
	struct _History *link;
}History; // structure for saving history

typedef struct _opNode {
	int opcode;
	char mnemonic[7], format[4];
	struct _opNode *link;
}opNode; // structure for making opcode list

typedef struct _Symbol{
	char symbol[10];
	int location;
	struct _Symbol *link;
}Symbol;

typedef struct _Object{
	char objcode[10];
	int loc;
	int eflag;
	int mflag;
	struct _Object* link;
}Object;

typedef struct _Exsym{
	char symbol[7];
	int csaddr;
	int length;
	struct _Exsym *link;
}Exsym;  // structure for saving external symbols

bool endFlag = FALSE; // if input is q or quit, set the flag true
bool errFlag = FALSE; // when there is an invalid input in hex to decimal function
History *head = NULL, *tail = NULL; // pointer for the front and end of history list
unsigned char memory[1048576] = {0,}; // for memory space 1048756 = 16 ^ 5
unsigned int bptab[1048576] = {0,};
int dumpEnd = 0; // the last memory address which was printed
opNode* opList[20]; // array for opcode list

int line_num = 1; // int to check the line numbers of the asm code
int line_num2 = 0; // int to check the numbers of intermediate file
int start_address = 0; //the address where START points
Symbol* symList[26] = {}; // array for SYMTAB
int LOCCTR = 0; // int for LOCCTR
int symNum = 0; // number of symbols
int enterflag = 0; // flag to check if we need an enter in obj file
Object *ohead = NULL, *otail = NULL; // pointer that points to the head and tail of obj list
int objfree = 0; //to check if free is already done;
Symbol *saveSym = NULL; //save the symbol list from the last successful assemble

unsigned int progaddr = 0; //default progadder 0
Exsym* estab[20]; // array for EXSYMTAB
int file_len = 0; 
int end_exec = 0;
int bpset[1000] = {0,};
int bpnum = 0;
int bpflag = -1;
int reg[10] = {0,};
int loadflag = 0;
// function to change hexadecimal number to decimal number
int hextodec(char cmd[], int start, int end) {
	int i; // loop iterator
	int dec = 0; // converted decimal number
	int num = 1; 
	bool startFlag = FALSE; // flag to find the starting point of the given address or value
	for (i = end - 1; i >= start; i--) {
		if (cmd[i] >= '0' && cmd[i] <= '9') {
			startFlag = TRUE; // set the start flag when the hexadecimal number starts
			dec += (cmd[i] - '0') * num;
			num = num * 16;
		}
		else if (cmd[i] >= 'A' && cmd[i] <= 'F') {
			startFlag = TRUE;
			dec += (cmd[i] - 'A' + 10) * num;
			num = num * 16;
		}
		else if (cmd[i] >= 'a' && cmd[i] <= 'f') {
			startFlag = TRUE;
			dec += (cmd[i] - 'a' + 10) * num;
			num = num * 16;
		}
		else if (cmd[i] == ' ' && startFlag == FALSE) // ignore spaces before the hexadecimal number starts
			continue;
		else { // if there is an input which is out of range, set errFlag /
			errFlag = TRUE;
			return 0;
		}
	}
	return dec;
}
// function to create hash index for hash table
int createHash(char mnec[]) {
	int i;
	int result = 0;
	for (i = 0; i < strlen(mnec); i++) {
		result += (mnec[i] - 'A') * 41; // creating a hash index
	}
	return result % 20;
}
// function to check the input command is an existing command
int checkCmd(char cmd[], char tcmd[]) {
	int i, j, k, l = 0; // variables for loops
	char temp[100]={'\0',}; // 
	for (i = 0; i < strlen(cmd); i++) { // trim spaces at front of the command
		if (cmd[i] != ' ')
			break;
	}
	for (j = strlen(cmd) - 1;; j--) { // trim spaces at the end of the command
		if (cmd[j] != ' ')
			break;
	}
	for (k = i; k <= j; k++) { // save the trimmed comamnd at tcmd
		tcmd[l] = cmd[k];
		l++;
	}
	//printf("%d %d\n",i,j);
	tcmd[l + 1] = '\0';
	for (i = 0; i < strlen(tcmd); i++) { //for commands with variables, find the command
		if (tcmd[i] == ' ' || tcmd[i] == '\0')
			break;
		temp[i] = tcmd[i];
	}
	temp[i + 1] = '\0';
	//printf("%s\n%s\n%s\n",cmd, tcmd, temp);
	if (!strcmp(tcmd, "h") || !strcmp(tcmd, "help")) { // find the matching command and return a number
		return 0;
	}
	else if (!strcmp(tcmd, "d") || !strcmp(tcmd, "dir")) {
		return 1;
	}
	else if (!strcmp(tcmd, "q") || !strcmp(tcmd, "quit")) {
		return 2;
	}
	else if (!strcmp(tcmd, "hi") || !strcmp(tcmd, "history")) {
		return 3;
	}
	else if (!strcmp(temp, "du") || !strcmp(temp, "dump")) {
		return 4;
	}
	else if (!strcmp(temp, "e") || !strcmp(temp, "edit")) {
		return 5;
	}
	else if (!strcmp(temp, "f") || !strcmp(temp, "fill")) {
		return 6;
	}
	else if (!strcmp(tcmd, "reset")) {
		return 7;
	}
	else if (!strcmp(temp, "opcode")) {
		return 8;
	}
	else if (!strcmp(tcmd, "opcodelist")) {
		return 9;
	}
	else if (!strcmp(temp, "assemble")){
		return 10;
	}
	else if (!strcmp(temp, "type")){
		return 11;
	}
	else if (!strcmp(tcmd, "symbol")){
		return 12;
	}
	else if(!strcmp(temp, "progaddr")){
		return 13;
	}
	else if(!strcmp(temp, "loader")){
		return 14;
	}
	else if(!strcmp(tcmd, "run")){
		return 15;
	}
	else if(!strcmp(temp, "bp")){
		return 16;
	}
	else
		return -1;
}
// add command to the history list
void addHistory(char cmd[]) {
	History *new;
	new = (History *)malloc(sizeof(History));
	strcpy(new->hCmd, cmd);
	new->link = NULL; // set new node with passed command

	if (!head) {
		head = new;
		tail = new;
	}
	else {
		tail->link = new;
		tail = new;
	} // add the new node at the end of the list
}
// function to print the commnads
void cmdHelp(char cmd[]) {
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start,end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\n");
	printf("assemble filename\ntype filename\nsymbol\n");
	printf("progaddr\nloader\nbp\nbp clear\nbp address\nrun\n");
	addHistory(cmd);
}
// function to print the files in the current directory
void cmdDir(char cmd[]) {
	DIR *dir = NULL;
	struct dirent *file = NULL;
	struct stat cStat;
	int prtNum = 0;
	dir = opendir(".");
	if (dir != NULL) { // point to the current directory
		while ((file = readdir(dir))!= NULL) { // read the files in the directory
			stat(file->d_name, &cStat);
			printf("\t%s", file->d_name);
			if (S_ISDIR(cStat.st_mode)) // if the read file is a directory print '/'
				printf("/");
			else if (S_IXUSR & cStat.st_mode) // if the file is a exe file print '*'
				printf("*");
			prtNum++;
			if (prtNum % 4 == 0)
				printf("\n");
		}
		closedir(dir);
		if (prtNum % 4 != 0)
			printf("\n");
	}
	addHistory(cmd);
}
// function to end the program
void cmdQuit(char cmd[]) {
	endFlag = TRUE;
}
// function to read the history list and print it
void cmdHistory(char cmd[]) {
	History* temp;
	int count = 1; // for the number of commands
	addHistory(cmd);
	for (temp = head; temp != NULL; temp = temp->link) { // follow the history list and print it in sequence
		printf("%4d %s\n", count, temp->hCmd);
		count++;
	}
}
// function to print the allocated memory
void cmdDump(char cmd[]) {
	int idx = 0, cidx = 0, sidx = 0, eidx = 0;// index which points to the needed location
	int start = -1, end = -1; // starting and end point of the memory
	int i, j;
	int cmdLen; // length of the command

	cmdLen = strlen(cmd);
	while (1) {
		if (cmd[idx] == ' ' || cmd[idx] == '\0')
			break;
		idx++;
	} // find where the command part ends
	if ((idx == 2 || idx == 4) && cmdLen == idx) { //if the input is just du or dump
		start = dumpEnd;
		end = start + 159;
		if (end > 0xFFFFF)
			end = 0xFFFFF;
		dumpEnd = end + 1;
		if (dumpEnd > 0xFFFFF)
			dumpEnd = 0;
	}
	else {
		for (i = idx; i < cmdLen; i++) {
			if (cmd[i] == ',') {
				cidx = i;
				break;
			} // find the index of the comma
		}
		for (i = idx; i < cmdLen; i++) {
			if (cmd[i] != ' ') {
				sidx = i;
				break;
			} // find the index where the starting address starts
		}
		if (cidx == cmdLen - 1) {
			printf("Address needed after comma\n");
			return;
		} // exception : there isn't a address after the comma
		else if (cidx == 0) { // if there isn't a comma, and just the starting address
			start = hextodec(cmd, sidx, cmdLen); // convert the starting address
			if (errFlag == TRUE) {
				printf("Invalid Address\n");
				errFlag = FALSE;
				return;
			} // exception : there was an invalid input
			end = start + 159;
			if (end > 0xFFFFF)
				end = 0xFFFFF;
			dumpEnd = end + 1;
			if (dumpEnd > 0xFFFFF)
				dumpEnd = 0;
		}
		else { // if there were two addresses given
			for (i = cidx + 1; i < cmdLen; i++) {
				if (cmd[i] != ' ') {
					eidx = i;
					break;
				}
			} // find the index where the end address starts
			start = hextodec(cmd, sidx, cidx);
			end = hextodec(cmd, eidx, cmdLen);
			if (errFlag == TRUE) {
				printf("Invalid Address\n");
				errFlag = FALSE;
				return;
			} // exception : there was an invalid input
			dumpEnd = end + 1;
			if (dumpEnd > 0xFFFFF)
				dumpEnd = 0;
		}
	}
	if (start > end) {
		printf("Start should be less than end\n");
		return;
	} // error when start adrress is bigger than end address
	if (start > 0xFFFFF || end > 0xFFFFF) {
		printf("Address should be less than 0xFFFFF\n");
		return;
	} // error when addresses are out of max range
	for (i = start / 16 * 16; i <= end / 16 * 16; i += 16) {
		printf("%05X ", i);
		for (j = i; j < i + 16; j++) {
			if (j >= start && j <= end)
				printf("%02X ", memory[j]);
			else
				printf("   ");
		}
		printf("; ");
		for (j = i; j < i + 16; j++) {
			if ((j >= start && j <= end) && (memory[j] >= 0x20 && memory[j] <= 0x7E))
				putchar(memory[j]);
			else
				putchar('.');
		}
		printf("\n");
	}  // print saved memory into the given format
	if (end == 0xFFFFF)
		dumpEnd = 0;
	addHistory(cmd);
}
// function to edit memory
void cmdEdit(char cmd[]) {
	int idx = 0, cidx = 0, aidx = 0, vidx = 0; // index to save needed location
	int address = 0, value = 0; // variable to save addresss and value
	int i; // loop iterator
	int cmdLen; 
	cmdLen = strlen(cmd);
	while (1) { 
		if (cmd[idx] == ' ' || cmd[idx] == '\0')
			break;
		idx++;
	} // find where the command ends
	for (i = idx; i < cmdLen; i++) {
		if (cmd[i] == ',') {
			cidx = i;
			break;
		}
	} // check the index of the comma
	if (cidx == 0) {
		printf("Need comma to identify parameters\n");
		return;
	} // if there is no commna, error
	else if (cidx == cmdLen - 1) {
		printf("Value is needed after comma\n");
		return;
	} // if there is no value after comma, error
	for (i = idx; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			aidx = i;
			break;
		}
	} // find the index of where the address starts
	for (i = cidx + 1; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			vidx = i;
			break;
		}
	} // find the index of where the value starts
	address = hextodec(cmd, aidx, cidx); // change address to decimal number
	value = hextodec(cmd, vidx, cmdLen); // change value to decimal number
	if (errFlag == TRUE) {
		printf("Invalid input\n");
		errFlag = FALSE;
		return;
	} // if there was an invalid input error
	if (address < 0x00000 || address > 0xFFFFF) {
		printf("Address out of range\n");
		return;
	} // if address is out of range error
	if (value < 0 || value > 0xFF)
	{
		printf("Value out of range\n");
		return;
	} // if value is out of range error
	memory[address] = value; 
	addHistory(cmd);
}
// function to fill the address from start to end with value
void cmdFill(char cmd[]) {
	int idx = 0, cidx1 = 0, cidx2 = 0, sidx = 0, eidx = 0, vidx = 0; // index to point to the needed location 
	int start = 0, end = 0, value = 0; // variables to save decimal values
	int i;
	int cmdLen;
	cmdLen = strlen(cmd);
	while (1) {
		if (cmd[idx] == ' ' || cmd[idx] == '\0')
			break;
		idx++;
	} // index where the command part ends
	for (i = idx; i < cmdLen; i++) {
		if (cmd[i] == ',') {
			cidx1 = i;
			break;
		}
	} // index of the first comma
	for (i = cidx1 + 1; i < cmdLen; i++) {
		if (cmd[i] == ',') {
			cidx2 = i;
			break;
		}
	} // index of the second comma
	if (cidx1 == 0 || cidx2 == 0) {
		printf("Need two commas to identify parameters\n");
		return;
	} // if there are less than two commas, error
	else if (cidx1 == cmdLen - 1) {
		printf("Invalid Command\n");
		return;
	} // if the command ends with a comma, error
	else if (cidx2 == cmdLen - 1) {
		printf("Need to input value\n");
		return;
	} // if there is no input value, error
	for (i = idx; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			sidx = i;
			break;
		}
	} // index of the starting point of the start address
	for (i = cidx1 + 1; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			eidx = i;
			break;
		}
	} // index of the starting point of the end address
	for (i = cidx2 + 1; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			vidx = i;
			break;
		}
	} // index of the starting point of the value
	start = hextodec(cmd, sidx, cidx1);
	end = hextodec(cmd, eidx, cidx2);
	value = hextodec(cmd, vidx, cmdLen);
	if (errFlag == TRUE) {
		printf("Invalid Input\n");
		errFlag = FALSE;
		return;
	} // if there was an invalid input, error
	if (start < 0x00000 || start > 0xFFFFF || end < 0x00000 || end > 0xFFFFF) {
		printf("Address out of range\n");
		return;
	} // if addresses are out of range, error
	if (start > end) {
		printf("Start should be less than end\n");
		return;
	} // if start address is bigger than end address error
	if (value < 0 || value > 0xFF) {
		printf("Value out of range\n");
		return;
	} // if value is out of range, error
	for (i = start; i <= end; i++) {
		memory[i] = value;
	}
	addHistory(cmd);
}
// function to reset memory
void cmdReset(char cmd[]) {
	memset(memory, 0, sizeof(memory));
	loadflag = 0;
	addHistory(cmd);
}
// function to open opcode.txt and create opcode list
void loadOpcode() {
	FILE* fp;
	char mnemonic[6], format[4];
	int opcode, idx;
	opNode *new, *temp;
	fp = fopen("opcode.txt", "r");
	if(fp == NULL){
		printf("opcode.txt doesn't exist\n");
		endFlag = TRUE;
		return;
	} // if opcode.txt doesn't exist error
	while (fscanf(fp, "%x %s %s", &opcode, mnemonic, format) != EOF) {
		new = (opNode *)malloc(sizeof(opNode));
		new->link = NULL;
		new->opcode = opcode;
		strcpy(new->format, format);
		strcpy(new->mnemonic, mnemonic);
		idx = createHash(mnemonic); // create hash with given mnemonic
		if (opList[idx]) { // add opNode structure to opcode list
			temp = opList[idx];
			while (temp->link)
				temp = temp->link;
			temp->link = new;
		}
		else
			opList[idx] = new;
	}
	fclose(fp);
	return;
}

//function to find the instruction/filename from command
int findName(char cmd[], char name[]){
	int i, j = 0;
	int idx = 0, idx2 = 0, cmdLen = 0;
	cmdLen = strlen(cmd);
	while(1){
		if(cmd[idx] == ' ' || cmd[idx] == '\0')
			break;
		idx++;
	}
	idx2 = idx;
	while(1){
		if(cmd[idx2] != ' ')
			break;
		idx2++;
	}
	if(idx == cmdLen)// if there is no instruction/filname
		return -1;
	for(i = idx2; i < cmdLen; i++){
		name[j] = cmd[i];
		j++;
	}
	return 0;
}

// function to show the number of the opcode mnemonic
void cmdOpcode(char cmd[]) {
	opNode* curr;
	int hash;
	char instruction[10] = {'\0', }; // to save the mnemonic
	if(findName(cmd, instruction) == -1){
		printf("Mnemonic is required\n");
		return;
	}
	hash = createHash(instruction);
	curr = opList[hash];
	while (curr && strncmp(curr->mnemonic, instruction, strlen(instruction))) {
		curr = curr->link;
		if (!curr)
			break;
	} // find the node that has the same instruction
	if (!curr) {
		printf("Invalid Mnemonic\n");
		return;
	}
	else {
		printf("opcode is %X\n", curr->opcode);
	}
	addHistory(cmd);
}

// function that prints opcodelist
void cmdOpcodelist(char cmd[]) {
	int i;
	int count;
	opNode* curr;
	for (i = 0; i < 20; i++) {
		printf("%3d : ", i);
		curr = opList[i];
		count = 0;
		while (curr) {
			printf("[%s,%x]", curr->mnemonic, curr->opcode);
			curr = curr->link;
			if (curr)
				printf(" -> ");
			count++;
		}
		if (count == 0)
			printf("empty");
		printf("\n");
	}
	addHistory(cmd);
}

// function to create symbol hash
int createSHash(char symb[]){
	int result;
	result = symb[0] - 'A';
	return result;
}

// function to get the node that has the matching instruction
opNode* getFormat(char instruction[]){
	int hash;
	opNode* curr;
	hash = createHash(instruction);
	curr = opList[hash];
	while(curr && strncmp(curr->mnemonic, instruction, strlen(instruction))){
		curr = curr->link;
		if(!curr)
			break;
	}
	return curr;
}

// function to get the Symbol* that has the matching label 
Symbol* getSymbol(char symb[]){
	int hash;
	Symbol *find;
	hash = createSHash(symb);
	if(hash<0 || hash>25)
		hash = 0;
	find = symList[hash];
	while(find && strncmp(find->symbol, symb, strlen(find->symbol))){
		find = find->link;
		if(!find)
			break;
	}
	return find;
}

//pass 1 algorithm to assemble file. creates intermediate file for pass 2
int pass1(char cmd[], char filename[]){
	FILE *fp; 
	FILE *midFile;
	char temp1[50], temp2[50], temp3[50];
	char line[150];
	int scan_num;
	int i;
	int hash;
	int format = 0, ret = 1;
	int tempsnum = 0;
	Symbol *curr, *next, *temp;
	
	start_address = 0;
	LOCCTR = 0;
	if(!(fp = fopen(filename, "r"))){
		printf("File Not Found\n");
		return -1;
	}
	if(!(midFile = fopen("intermediate", "w"))){
		printf("Failed creating intermediate file\n");
		return -1;
	}
	while(fgets(line, 110, fp)){// read 1 line from asm file
		memset(temp1, '\0', sizeof(temp1));
		memset(temp2, '\0', sizeof(temp2));
		memset(temp3, '\0', sizeof(temp3));
		format = 0;
		scan_num = sscanf(line, "%s %s %s", temp1, temp2, temp3);
		for(i=0;i<strlen(line);i++){ // if comma exists change scan_num
			if(line[i] == ','){
				scan_num--;
				break;
			}
		}
		fprintf(midFile, "%4d\t", line_num * 5); // print line number
		line_num++;
		if(temp1[0] >= '0' && temp1[0] <='9'){ // error when first character is number
			printf("Number can't be firt character\n");
			ret = -1;
			break;
		}

		// if it is a comment
		if(temp1[0] == '.'){
			fprintf(midFile, "%4s\t", "");
			fprintf(midFile, "%s", line);
			continue;
			}
		else{ // if label exists
			if(scan_num == 3){
				if(strcmp(temp2, "START")){ // search if label exists in SYMTAB
					hash = createSHash(temp1);
					curr = symList[hash];
					while(curr && strcmp(temp1, curr->symbol)) curr = curr->link;
					if(curr){ // when it is existing symbol error
						printf("Duplicate Symbol\n");
						ret = -1;
						break;
					}
					else{ 
						temp = (Symbol *)malloc(sizeof(Symbol));
						strcpy(temp->symbol, temp1);
						temp->location = LOCCTR;
						temp->link = NULL;
						if(!symList[hash]) symList[hash] = temp;
						else{
							next = symList[hash];
							while(next->link) next = next->link;
							next->link = temp;
						}
						tempsnum++;
					}
				}
				if(!strcmp(temp2, "START")){ // when label is START
					if(line_num != 2){ // when first line is not START, error
						printf("Start should be the first instruction\n");
						ret = -1;
						break;
					}
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "%s", line);
					start_address = hextodec(temp3,0,strlen(temp3));
					if(errFlag == TRUE){ // when invaild starting address, error
						printf("Invalid start address\n");
						errFlag = FALSE;
						ret = -1;
						break;
					}
					LOCCTR = start_address; // adjust LOCCTR to start address
					continue;
				}//adjust LOCCTR
				if(!strcmp(temp2,"WORD")){ // when WORD, +3 to LOCCTR
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "%s", line);
					LOCCTR += 3;				
					continue;
				}
				else if(!strcmp(temp2,"RESW")){ // when RESW  +3 * operand to LOCCTR
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "%s", line);
					LOCCTR += 3 * atoi(temp3);
					continue;
				}
				else if(!strcmp(temp2,"RESB")){ // when RESB +operand to LOCCTR
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "%s", line);
					LOCCTR += atoi(temp3);
					continue;
				}
				else if(!strcmp(temp2,"BYTE")){ // when BYTE
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "%s", line);
					if(temp3[0] == 'C')
						LOCCTR += strlen(temp3) - 3;
					else if (temp3[0] == 'X') {
						if((int)((strlen(temp3) - 3) % 2) == 0)
							LOCCTR += (int)(strlen(temp3) - 3) / 2;
						else
							LOCCTR += (int)(strlen(temp3) - 3) / 2 + 1;
					}
					continue;
				}
				else{ 
					if(temp2[0] == '+'){ // format 4
						if(!getFormat(temp2+1)){ // when opcode doesn't exist error
							printf("Invalid Mnemonic\n");
							ret = -1;
							break;
						}
						fprintf(midFile, "%4d\t", LOCCTR);
						fprintf(midFile, "%s", line);
						LOCCTR += 4;
						continue;
					}
					else{
						//format = atoi(getFormat(temp2)->format);
						if(!getFormat(temp2)){ // when opcode doesn't exist, error
							printf("Invalid Mnemonic\n");
							ret = -1;
							break;
						}
						else{ // add format to LOCCTR
							format = atoi(getFormat(temp2)->format);
							fprintf(midFile, "%4d\t", LOCCTR);
							fprintf(midFile, "%s", line);
							LOCCTR += format;
							continue;
						}
					}
				}
			}
			else if(!strcmp(temp1, "BASE")){ //when lable doesn't exist, mark with '!', when loc isn't needed mark with -1
				fprintf(midFile, "%4d\t",-1);
				fprintf(midFile, "!");
				fprintf(midFile, "%s",line);
				continue;
			}
			else{//when lable doesn't exist, mark with '!', when loc isn't needed mark with -1
				if(!strcmp(temp2, "START") && temp3[0] == '\0'){ // when START doesn't have an operand, ERROR
					printf("No starting address\n");
					ret = -1;
					break;
				}
				else if(!strcmp(temp1, "END")){
					fprintf(midFile, "%4d\t",-1);
					fprintf(midFile, "!");
					fprintf(midFile, "%s", line);
					break;
				}
				else if(temp1[0] == '+'){ // format 4
					if(!getFormat(temp1+1)){// when opcode doens't exist, error
						printf("Invalid Mnemonic\n");
						ret = -1;
						break;
					}
					fprintf(midFile, "%4d\t", LOCCTR);
					fprintf(midFile, "!");
					fprintf(midFile, "%s",line);
					LOCCTR += 4;
					continue;
				}
				else{ // when opcode doesn't exist
					//format = atoi(getFormat(temp1)->format);
					if(!getFormat(temp1)){
						printf("Invalid Mnemonic\n");
						ret = -1;
						break;
					}
					else{
						format = atoi(getFormat(temp1)->format);
						fprintf(midFile, "%4d\t", LOCCTR);
						fprintf(midFile,"!%s", line);
						LOCCTR += format;
						continue;
					}
				}
			}
		}
	}
	fclose(fp);
	fclose(midFile);
	if(ret != -1) // when not error save Symnum  
		symNum = tempsnum;
	return ret;
}

// function to print .lst file with given format
void printlst(FILE * lst, int num, int loc, char label[], char opcode[], char operand[], char address[]){
	fprintf(lst, "%4d\t", num);
	if(loc != -1)
		fprintf(lst, "%04X\t", loc);
	else
		fprintf(lst, "%-4s\t", "");
	if(!strcmp(label, "!"))
		fprintf(lst, "%-6s ", "");
	else
		fprintf(lst, "%-6s ", label);
	fprintf(lst, "%-6s ", opcode);
	fprintf(lst, "%-10s ", operand);
	fprintf(lst, "%-10s\n", address);
}

// function to create object code list for printing .obj file
void createobj(char TA[], int loc, int mf){
	Object* obj;
	obj = (Object *)malloc(sizeof(Object));
	strcpy(obj->objcode, TA);
	obj->loc = loc;
	obj->eflag = enterflag;
	obj->mflag = mf;
	if(!ohead){
		ohead = obj;
		otail = obj;
	}
	else{
		otail->link = obj;
		otail = otail->link;
	}
	enterflag = 0;
}
// function for pass 2 algorithm creates .lst .obj file with intermediate file
int pass2(char filename[]){
	FILE *midFile, *lst, *obj; // file pointer for creating obj, lst file 
	int num, loc;  // to read line number, and location
	char label[50], opcode[50], operand[50]; // array for saving lable, opcode operand
	char line[150];
	char rmoperand[50], rmopcode[50]; // array for saving operand, opcode without prefix
	char TA[10], tempstr[100] = {'\0',}; //array for saving target address and object code
	char reg[][3] = {"A", "X", "L", "B", "S", "T", "F", "0", "PC", "SW"};  // register array
	int N, I, X, B, P, E, xbpe, PC = 0; // value of NIXBPE to check the addressing mode, and program counter
	char fname[50] = {'\0',}, fname2[50] = {'\0',}; // array for filename
	int i, cidx; // loop variable and index of comma 
	int mf;
	int ret = 1;
	int format = 0, BASE = 0, disp = 0;
	int startFlag = 0, ENDFLAG = 0;
	Symbol* curr;
	opNode* node;
	Object* obnode;

	for(i = 0; filename[i] != '.'; i++)
		fname[i] = filename[i];
	strcpy(fname2, fname);
	strcat(fname, ".lst");
	strcat(fname2, ".obj");

	if(!(midFile = fopen("intermediate", "r"))){
		printf("Failed opening intermediate file\n");
		return -1;
	}
	if(!(lst = fopen(fname, "w"))){
		printf("Failed creating lst file\n");
		return -1;	
	}
	if(!(obj = fopen(fname2, "w"))){
		printf("Failed creating obj file\n");
		return -1;
	}
	PC = start_address;
	while(fgets(line, 150, midFile)){
		num = 0;
		loc = 0;
		N = I = X = B = E = mf = 0;
		P = 1;
		i = cidx = disp = xbpe = 0;
		memset(label, '\0', sizeof(label));
		memset(opcode, '\0', sizeof(opcode));
		memset(operand, '\0', sizeof(operand));
		memset(rmoperand, '\0', sizeof(rmoperand));
		memset(rmopcode, '\0', sizeof(rmopcode));
		memset(TA, '\0', sizeof(TA));

		sscanf(line, "%d %d %s %s %[^\n]", &num, &loc, label, opcode, operand);
		//printf("%d %d %s %s %s\n", num,loc,label,opcode,operand);
		line_num2 = num;
		if(!strcmp(opcode, "END")){
			//write last listing line
			ENDFLAG = 1;
			printlst(lst, num, loc, label, opcode, operand, "");
			break;
		}	
		if(!strcmp(opcode, "START")){
			// write header record (H)
			// initialize first text record
			printlst(lst, num, loc, label, opcode, operand, "");
			fprintf(obj,"H%-6s%06X%06X\n",label,loc,LOCCTR-loc);
			startFlag = 1;
			continue;
		}
		if(opcode[0] == '\0'){
			//when comment
			fprintf(lst, "%s", line);
			continue;
		}
		else{
			//not comment
			//opcode should exist(checked in pass 1)
			if(!strcmp(opcode, "BASE")){ // when BASE
				if(!(curr=getSymbol(operand))){ // if it doesn't exist in SYMTAB error
					printf("Undefiend symbol\n");
					ret = -1;
					break;
				}
				else{
					BASE = curr->location; //set base
 					printlst(lst, num, loc, label, opcode, operand, "");
				}
				continue;
			}
			switch(operand[0]){// check if operand has prefix
				case '#': // immediate addressing
					N = 0;
					I = 1;
					B = 0;
					if(operand[1] >= '0' && operand[1] <= '9')
						P = 0;
					strcpy(rmoperand, operand + 1);
					break;
				case '@': // indirect addressing
					N = 1;
					I = 0;
					strcpy(rmoperand, operand + 1);
					break;
				default : // simple addressing
					strcpy(rmoperand, operand);
					N = 1;
					I = 1;
					break;
			}
			if(opcode[0] == '+'){ // when format 4
				node = getFormat(opcode + 1);
				format = atoi(node->format);
				if(format != 3){ // when + is used for wrong opcode
					printf("+ should be used for 3/4 format\n");
					ret = -1;
					break;
				}
				format = 4;
				B = 0;
				P = 0;
				E = 1;
			}
			else { 
				node = getFormat(opcode);
				if(node)
					format = atoi(node->format);
			}
			//PC update	
			if(!strcmp(opcode, "BYTE")){
				if(operand[0] == 'C'){
					PC += strlen(operand) - 3;
					for(i = 2; i <= strlen(operand) - 2; i++){
						sprintf(TA + (2*(i-2)), "%X",(int)operand[i]);
					}	
				}
				else if (operand[0] == 'X'){
					if ((int)(strlen(operand) - 3) % 2 == 0) {
						PC += (strlen(operand) - 3) / 2;
						for (i = 2; i <= strlen(operand) - 2; i++)
							TA[i - 2] = operand[i];
					}
					else {
						PC += (int)(strlen(operand) - 3) / 2 + 1;
						TA[0] = '0';
						for (i = 2; i <= strlen(operand) - 2; i++)
							TA[i - 1] = operand[i];
					}
				}
				printlst(lst, num, loc, label, opcode, operand, TA);
				createobj(TA, loc, 0);			
				continue;
			}
			else if(!strcmp(opcode, "WORD")){
				PC += 3;
				sprintf(TA, "%06X", atoi(operand));
				printlst(lst, num, loc, label, opcode, operand, TA);
				createobj(TA, loc, 0);
				continue;
			}
			else if(!strcmp(opcode, "RESB")){
				PC += atoi(rmoperand);
				printlst(lst, num, loc, label, opcode, operand, "");
				enterflag = 1; //there needs to be an enter in obj file
				continue;
			}
			else if(!strcmp(opcode, "RESW")){
				PC += atoi(rmoperand) * 3;
				printlst(lst, num, loc, label, opcode, operand, "");
				enterflag = 1; //enter is needed in obj file
				continue;
			}
			else
				PC += format;
			
			for(i = 0; i < strlen(operand); i++){ // check for comma in operand
				if(operand[i] == ','){
					cidx = i;
					break;
				}
			}
			if(format == 1){ // when format 1
				if(operand[0] != '\0'){ // format 1 can't have operand
					printf("Operand is not allowed for format 1\n");
					ret = -1;
					break;
				}
				sprintf(TA, "%X", node->opcode);
				strcat(TA,"0000");	
			}
			else if(format == 2){ // when format 2
				sprintf(TA, "%X", node->opcode);
				if(cidx!=0){ // if there are 2 register
					for(i=0;i<10;i++){
						if(!strncmp(reg[i], operand, cidx))
							break;					
					}
					if(i == 10 || i == 7){ // check if valid register
						printf("Unidentified register %s\n",operand);
						ret = -1;
						break;
					}
					sprintf(TA+2, "%d",i);
					for(i=0;i<10;i++){ 
						if(!strncmp(reg[i], operand + cidx + 2, strlen(reg[i])))
							break;
					}
					if(i == 10 || i == 7){ // check if valid register
						printf("Unidentified register %s\n",operand+cidx+2);
						ret = -1;
						break;					
					}
					sprintf(TA+3,"%d", i);
				}
				else{ // 1 register
					for(i=0;i<10;i++){
						if(!strncmp(reg[i], operand, strlen(reg[i])))
							break;
					}
					if(i == 10 || i == 7){//check if valid register
						printf("Unidentified register %s\n",operand);
						ret = -1;
						break;
					}
					sprintf(TA+2, "%d",i);
					TA[3] = '0';
				}
			}
			else if(format == 3){ // format 3
				sprintf(TA, "%02X", (node->opcode) + (2 * N + I));
				if(cidx != 0){ // if there is a register 
					if(!strncmp(operand + cidx + 2, "X", strlen("X"))) // if register is X set X = 1
						X = 1;
					else{
						printf("Invalid Register with BUFFER\n");
						ret = -1;
						break;
					}
				}	
				if(!(curr = getSymbol(rmoperand))){ //if there is a symbol match
					sprintf(TA+2, "%04X", atoi(rmoperand));
					printlst(lst, num, loc, label, opcode, operand, TA);
					createobj(TA, loc, 0);
					continue;
				}
				else{ // check if PC relative or base relative
					disp = curr->location - PC;
					if(disp < -2048 || disp > 2047){
						disp = curr->location - BASE;
						B = 1;
						P = 0;
						if(disp<0 || disp>4095){
							printf("Displacement out of range\n");
							ret = -1;
							break;
						}	
					}
					if(disp < 0)
						disp = disp & 4095;
				}
				xbpe = (int)(pow(2,3))*X+(int)(pow(2,2))*B+(int)(pow(2,1))*P+E;
				sprintf(TA+2,"%04X",(int)(pow(16,3))*xbpe + disp);
			}
			else if(format ==4){ // format 4
				sprintf(TA, "%02X", (node->opcode) + (2 * N + I));
				xbpe = (int)(pow(2,3))*X+(int)(pow(2,2))*B+(int)(pow(2,1))*P+E;
				curr = getSymbol(rmoperand);
				if(curr){
					sprintf(TA+2, "%06X", (int)(pow(16,5))*xbpe + getSymbol(rmoperand)->location);
					mf = 1;
				}
				else
					sprintf(TA+2, "%06X", (int)(pow(16,5))*xbpe + atoi(rmoperand));
			}
			printlst(lst, num, loc, label, opcode, operand, TA);
			//printf("%d %X %s %s %s %s\n",num, loc, label, opcode ,operand, TA);
			createobj(TA, loc, mf);
			continue;
			//if opcode exists
		}
	}
	if((ENDFLAG == 0 || startFlag == 0) && ret != -1){// if there is no START or END error
		printf("Code should have START/END\n");
		ret = -1;	
	}
	if(ret != -1){ //when not error write .obj file
		//printf("ob");
		obnode = ohead;
		while(obnode){ // print text record
			fprintf(obj,"T%06X", obnode->loc);
			while(((int)(strlen(tempstr) + strlen(obnode->objcode))/2) <= 29){
				strcat(tempstr,obnode->objcode);
				obnode = obnode->link;
				if(!obnode)
				break;
				if(obnode->eflag == 1)
					break;
			}
			fprintf(obj, "%02X", (int)strlen(tempstr)/2);
			fprintf(obj, "%s\n", tempstr);
			memset(tempstr, '\0', sizeof(tempstr));
		}
		obnode = ohead;
		while(obnode){ // print modification record
			if(obnode->mflag == 1){
				fprintf(obj,"M%06X05\n",obnode->loc+1);
			}
			obnode = obnode->link;
		}
		fprintf(obj, "E%06X\n", ohead->loc);
	}
	fclose(midFile);
	fclose(lst);
	fclose(obj);
	if(ret == -1){
		while(ohead){
			obnode = ohead;
			ohead = ohead->link;
			free(obnode);		
		}
		objfree = 1;
		remove(fname);
		remove(fname2);	
	}//remove .lst, .obj file and obj list when error
	return ret;
}

// compare to make alphabetical order
int compare(const void *a, const void *b) {
	return (strcmp(((Symbol *)a)->symbol, ((Symbol *)b)->symbol));
}

// call pass 1 and pass 2 and initialize 
void cmdAssemble(char cmd[]){
	char filename[100] = {'\0',};
	int i , j = 0;
	Symbol *curr;
	Object* node;

	line_num = 1;
	line_num2 = 0;
	//symNum = 0;
	//freeFlag = 0;
	objfree = 0;

	for(i = 0; i < 26; i++){
		while(symList[i]){
			curr = symList[i];
			symList[i] = symList[i] -> link;
			free(curr);
		}
	}
	while(ohead){
		node = ohead;
		ohead = ohead->link;
		free(node);
	}
	if(findName(cmd,filename) == -1){
		printf("Filename required\n");
		return;
	}
	if(pass1(cmd,filename)!=1){
		printf("PASS1 Error at line : %d\n",(line_num - 1) * 5);
		return;
	}

	if(pass2(filename)==1){
		printf("\x1b[32mSuccessfully ");
		printf("\x1b[0massemble %s.\n",filename);
		if (saveSym != NULL) {
			free(saveSym);
		}
		//save the SYMTAB of the last successful assmeble
		if (symNum != 0) { // create a list that saves the value of symbols
			saveSym = (Symbol *)malloc(sizeof(Symbol) * symNum);
			for (i = 0; i < 26; i++) {
				curr = symList[i];
				while (curr) {
					strcpy(saveSym[j].symbol, curr->symbol);
					saveSym[j].location = curr->location;
					j++;
					curr = curr->link;
				}
			}
			qsort(saveSym, symNum, sizeof(Symbol), compare); // sort the contents
		}
	}
	else{
		printf("PASS2 Error at line : %d\n", line_num2);
	}
	addHistory(cmd);
	return;
}

// function for command Type
void cmdType(char cmd[]){
	FILE *fp;
	char c;
	char filename[100] = {'\0',};
	DIR *currDir;
	struct dirent *currFile;
	struct stat currStat;
	int dirFlag = 0;
	
	if(findName(cmd, filename)==-1){
		printf("Filename required\n");
		return;		
	}
	if((currDir = opendir("."))){ // error when directory is input
		while((currFile = readdir(currDir))){
			stat(currFile->d_name, &currStat);
			if(!strncmp(currFile->d_name, filename, sizeof(filename))){
				if(S_ISDIR(currStat.st_mode))
					dirFlag = 1;
			}
		}
		closedir(currDir);
	}
	if(dirFlag == 1){
		printf("Directory name cannot be an input\n");
		return;
	}
	if((fp = fopen(filename, "r"))!=NULL){ // print the contents of the file on screen
		while(~(c = fgetc(fp)))
			putchar(c);
		fclose(fp);
	}
	else{
		printf("File Not Found\n");
		return;
	}
	addHistory(cmd);
}

// function for command symbol
void cmdSymbol(char cmd[]){
	int i;
	//print saved symbol
	for(i=0;i<symNum;i++)
			printf("\t%s\t%04X\n",saveSym[i].symbol, saveSym[i].location);

	if(symNum == 0)
		printf("No symbol in SYMTAB\n");
	addHistory(cmd);
}

//function for command Progaddr
void cmdProgaddr(char cmd[]){
	int idx;
	int i;
	int cmdLen;
	cmdLen = strlen(cmd);
	if(cmdLen == 8){ // if there is no address Error
		printf("Address required for progaddr\n");
		return;
	}

	for (i = 8; i < cmdLen; i++){
		if(cmd[i] != ' '){
			idx = i;
			break;
		}
	}
	progaddr = hextodec(cmd, idx, cmdLen);
	if(errFlag == TRUE || progaddr > 0xFFFFF || progaddr < 0){ // calculate address to hex and set progaddr
		printf("Invalid program address\n");
		errFlag = FALSE;
		progaddr = 0;
		return;
	}
	addHistory(cmd);
}
//function for printing loadmap
void printloadmap(){
	int i;
	Exsym *curr;
	int total_length = 0;
	printf("control symbol address length\n");
	printf("section name\n");
	printf("---------------------------------\n");
	for(i = 0; i < 20; i++){
		curr = estab[i];
		while(curr){
			if(curr->length != 0){
				printf("%-6s%7s   %04X    %04X\n", curr->symbol, " ", curr->csaddr, curr->length);
				total_length += curr->length;
			}
			else
				printf("%6s%7s   %04X\n", " ", curr->symbol, curr->csaddr); 
			curr = curr->link;
		}
	}
	printf("---------------------------------\n");
	printf("\t total length %04X\n", total_length);
}
//pass1 -> assign address to external symbol
int loadpass1(int count, char f1[][30]){
	FILE* fp;
	int i, j, hash, linenum;
	unsigned int csaddr, cslen, loc;
	char filename[30], sym[7];
	char line[150];
	Exsym *curr, *node;

	csaddr = progaddr;
	for(i = 0; i < 20; i++){
		while(estab[i]){
			curr = estab[i];
			estab[i] = estab[i]->link;
			free(curr);			
		}
	}
	for(i = 0; i < count; i++){
		linenum = 1;
		strcpy(filename, f1[i]);
		if(!(fp = fopen(filename, "r"))){
			printf("File %s does not exist\n", filename);
			return -1;
		}	
		fgets(line, 150, fp);
		sscanf(line, "%*c%6s%*6x%6x", sym, &cslen); //get the symbol and length of cs at the first line
		hash = createHash(sym);
		curr = estab[hash];
		while(curr && strcmp(curr->symbol, sym)) //search for duplicate symbols
			curr = curr->link;
		if(curr){
			printf("Duplicate symbol %s at line %d in %s\n", sym, linenum, filename);
			return -1;
		}
		node = (Exsym*)malloc(sizeof(Exsym));
		strcpy(node->symbol, sym);
		node->csaddr = csaddr;
		node->length = cslen;
		node->link = NULL;
		if(estab[hash]){ //if there is no duplicate symbol, add it to the external symbol tab
			curr = estab[hash];
			while(curr->link)
				curr = curr->link;
			curr->link = node;
		}
		else
			estab[hash] = node;
		while(line[0] != 'E'){
			memset(line, '\0', sizeof(line));
			fgets(line, 150, fp);
			linenum++;
			if(line[0] != 'D') //only read line with D record
				continue;
			for(j = 1; j < strlen(line)-1; j += 12){
				sscanf(line + j, "%6s%6x", sym, &loc);
				hash = createHash(sym);
				curr = estab[hash];
				while(curr && strcmp(curr->symbol, sym)) //check for duplicate symbols
					curr = curr->link;
				if(curr){
					printf("Duplicated symbol %s at line %d in %s\n", sym, linenum, filename);
					return -1;
				}
				node = (Exsym*)malloc(sizeof(Exsym));
				strcpy(node->symbol, sym);
				node->csaddr = loc + csaddr;
				node->length = 0;
				node->link = NULL;
				if(estab[hash]){ //add the external symbol if there is no duplicate
					curr = estab[hash];
					while(curr->link)
						curr = curr->link;
					curr->link = node;
				}
				else
					estab[hash] = node;
			}
		}
		csaddr += cslen;
		fclose(fp);
	}
	file_len = csaddr - progaddr; //save the file length 
	return 1;
}
//pass2 -> link and load
int loadpass2(int count, char f1[][30]){
	FILE* fp;
	int i, j, linenum, hash;
	int objcode;
	unsigned int addr, tlen;
	unsigned int csaddr, cslen, rnum, refer[30] = {0,}, hb, maddr;
	char filename[30], sym[7];
	char line[150], sign;

	Exsym *curr;
	csaddr = progaddr;
	for(i = 0; i < count; i++){
		linenum = 1;
		refer[0] = 0;
		strcpy(filename, f1[i]);
		if(!(fp = fopen(filename, "r"))){
			printf("File %s does not exist\n", filename);
			return -1;
		}	
		fgets(line, 150, fp);
		sscanf(line, "%*c%6s%*6x%6x", sym, &cslen);
		refer[1] = csaddr;

		while(line[0] != 'E'){ //until the end record
			memset(line, '\0', sizeof(line));
			fgets(line, 150, fp);
			linenum++;
			if(line[0] == 'R'){//check reference number
				refer[0] = 1;
				for(j = 1; j < strlen(line)-1; j+=8){
					sscanf(line+j, "%2X%6s", &rnum, sym);
					hash = createHash(sym);
					curr = estab[hash];
					while(curr && strcmp(curr->symbol, sym))
						curr = curr->link;
					if(!curr){ //if there is a symbol undefined, error
						printf("In %s line %d undefined symbol %s\n", filename, linenum, sym);
						return -1;
					}
					refer[rnum] = curr->csaddr; //give the reference number the actual address of the symbol
				}
			}
			else if(line[0] == 'T'){ //read text record and save it in memory
				sscanf(line+1, "%6x%2x", &addr, &tlen);
				for(j = 0; j < tlen; j++){
					sscanf(line+9+j*2, "%2X", &objcode);
					memory[j+csaddr+addr] = (unsigned char)objcode;
				} 
			}
			else if(line[0] == 'M'){//read modification record
				sscanf(line+1, "%6x%2x%c%2x", &addr, &hb, &sign, &rnum);
				maddr = memory[csaddr + addr] % (hb % 2 ? 0x10 : 0x100); //check if the length of the half byte is 5 / 6
				for(j = 1; j < (hb + 1) / 2; j++){//merge the saved values in the memory to modify
					maddr *= 0x100;
					maddr += memory[csaddr + addr + j];
				}
				maddr += (sign == '-' ? -1 : 1) * refer[rnum];
				for(j = (hb -1)/2; j > 0; j--){ //save the modified value back to memory
					memory[csaddr + addr + j] = maddr % 0x100;
					maddr /= 0x100;
				}
				if(hb % 2 == 1) //if half byte == 5, modify half byte
					memory[csaddr + addr] = (memory[csaddr + addr]&((1<<8)-(1<<4))) + maddr;
				else
					memory[csaddr + addr] = maddr;
			}
		}
		csaddr += cslen;
		fclose(fp);
	}
	end_exec = progaddr;
	return 1;
}
//when comand loader is input
void cmdLoader(char cmd[]){
	int count = 0;
	int i, j = 0;
	int cmdLen;
	int pass1, pass2;
	char f1[3][30] = {'\0'};
	cmdLen = strlen(cmd);
	if(cmdLen == 6){ //if there is no filename error
		printf("Filename required for loader\n");
		return;
	}
	//check the number of files and save each filename
	for (i = 6; i < cmdLen; i++){
		if(cmd[i] != ' '){
			count++;
			break;
		}
	}
	for(j = 0; i < cmdLen; i++){
		if(cmd[i] ==  ' ')
			break;
		f1[0][j++] = cmd[i];
	}
	for (; i < cmdLen;i++){
		if(cmd[i] != ' '){
			count++;
			break;
		}
	}
	for(j = 0; i < cmdLen; i++){
		if(cmd[i] == ' ')
			break;
		f1[1][j++] = cmd[i];
	} 
	for (; i < cmdLen; i++){
		if(cmd[i] != ' '){
			count++;
			break;
		}
	}
	for(j = 0; i < cmdLen; i++){
		f1[2][j++] = cmd[i];
	}
	//if pass1 and pass2 is successful, print loadmap and add command to  history
	pass1 = loadpass1(count, f1);
	if(pass1 < 0){
		printf("[\x1b[31mError\x1b[0m] Pass1\n");
		return;
	}
	pass2 = loadpass2(count ,f1);
	if(pass2 < 0){
		printf("[\x1b[31mError\x1b[0m] Pass2\n");
		return;
	}
	loadflag = 1;
	printloadmap();
	addHistory(cmd);
}
//function for command bp
void cmdBp(char cmd[]){
	int i, cmdlen;
	int idx, bp;
	cmdlen = strlen(cmd);
	for(i=2;i<cmdlen;i++){
		if(cmd[i] != ' '){
			idx = i;
			break;
		}
	}

	if(strlen(cmd) == 2){ //when command == bp, print all the breakpoints
		printf("\tbreakpoint\n");
		printf("\t---------\n");
		for(i=0; i<bpnum; i++){
			printf("\t%X\n", bpset[i]);
		}
	}
	else if(!strcmp(cmd+idx, "clear")){ //when command == bp clear, remove all the breakpoints
		memset(bpset, 0, sizeof(bpset));
		memset(bptab, 0, sizeof(bptab));
		bpnum = 0;
		bpflag = -1;
		printf("\t[\x1b[32mok\x1b[0m] clear all breakpoints\n");
	}
	else{ //when address is input
		bp = hextodec(cmd,idx,cmdlen);
		if(errFlag == TRUE || bp > 0xFFFFF || bp < 0){//if address is out of range, error
			printf("Invalid Breakpoint\n");
			errFlag = FALSE;
			return;
		}
		if(bptab[bp] == 1){ //when the breakpoint already exist, error
			printf("Existing break point at %d\n", bp);
			return;
		}
		//add bp to bpset, bptab
		printf("\t[\x1b[32mok\x1b[0m] create breakpoint %X\n", bp);
		bpset[bpnum] = bp;
		bptab[bp] = 1;
		bpnum++;
	}
	addHistory(cmd);
}
//function for printing values in the registers
void printReg(int reg[]){
	printf("A : %06X  X : %06X\n", reg[0], reg[1]);
	printf("L : %06X PC : %06X\n", reg[2], reg[8]);
	printf("B : %06X  S : %06X\n", reg[3], reg[4]);
	printf("T : %06X\n",reg[5]);
}
//function for getting the target address of from objcode in memory(only used in format 3,4)
int getTarget(int curr, int reg[], int nixbpe){
	enum registers {A,X,L,B,S,T,F,NA,PC,SW};
	int target, format, disp;
	format = 3 + (nixbpe & 1);
	reg[PC] += format; // get format
	if(format == 4){ //when format 4
		target = (((memory[curr+1] & 0x0F) << 16) | (memory[curr+2] << 8) | (memory[curr+3]));
	}
	else if(nixbpe & 6){ // base, pc relative
		disp = (((memory[curr+1] & 0x0F) << 8) | memory[curr+2]);
		if(disp & (1<<11)) // if negative number
			disp = -((~disp + 1) & 0xFFF);
		if(nixbpe & 4)
			target = reg[B] + disp;
		else
			target = reg[PC] + disp;
	}
	else{
		target = (((memory[curr+1] & 0x0F) << 8) | memory[curr+2]);
	}
	if(nixbpe & 8){ // index
		target += reg[X];
	}
	if((nixbpe & 0x30) == 0x20) //when indirect addressing, return the address that the target address is pointing to
		target = ((memory[target] << 16) | (memory[target+1] << 8) | memory[target+2]);
	return target;
}
//function for calculating the value from the address
int calcVal(int TA, int nixbpe){
	int result;
	nixbpe = nixbpe & 0x30; //check for n,i flags

	if(nixbpe == 0x10) // immediate addressing
		result = TA;
	else{//Indirect, simple addressing
		result = ((memory[TA] << 16) | (memory[TA+1] << 8) | memory[TA+2]);	
	}
	return result;
}
//function for command run
void cmdRun(char cmd[]){
	enum registers {A,X,L,B,S,T,F,NA,PC,SW};
	int nixbpe;
	int curr, end, target;
	int opcode, value;
	int regnum, regnum2;
	int i;
	if(strlen(cmd) != 3){
		printf("Invalid command\n");
		return;
	}
	if(loadflag == 0){
		printf("Loaded program doesn't exist\n");
		return;
	}
	//set the initail values
	reg[PC] = end_exec;	
	curr = end_exec;
	end = progaddr + file_len;
	if(bpflag == -1)//L register shouldn't be modified when program is run
		reg[L] = end;
	if(bptab[0] == 1 && bpflag == -1){ //when pc points to the breakpoint print registers
				bpflag = 1;
				printReg(reg);
				printf("\tStop at checkpoint [%X]\n",curr);
				addHistory(cmd);
				return;
	}
	while(reg[PC]!= end){
		opcode = memory[curr] & 0xFC; //find the opcode from the first 6bits
		nixbpe = ((memory[curr] & 0x03) << 4) | ((memory[curr+1] & 0xF0) >> 4); //find the nixbpe from the next 6bits
		if(opcode == 0x0C || opcode == 0x78 || opcode == 0x54 || opcode == 0x14 || opcode == 0x10){ // STA, STB, STCH, STL, STX
			target = getTarget(curr, reg, nixbpe);
			if(opcode == 0x54){ //when STCH
				memory[curr] = (unsigned char)(reg[A] & 0xF);
			}
			else{
				if(opcode == 0x0C) //STA
					regnum = A;
				else if(opcode == 0x78) //STB
					regnum = B;
				else if(opcode == 0x14) //STL
					regnum = L;
				else if(opcode == 0x10) //STX
					regnum = X;
				for(i = 0; i < 3; i++) //store the value of the register to memory
					memory[target + i] = (unsigned char)((reg[regnum]>>(2-i)*8)&0xFF);
			}
		}
		else if(opcode == 0x00 || opcode == 0x68 || opcode == 0x50 || opcode == 0x74 || opcode == 0x08 || opcode == 0x04 ){ //LDA, LDB, LDCH, LDT, LDL
			target = getTarget(curr, reg, nixbpe);
			value = calcVal(target, nixbpe);

			if(opcode == 0x50){
				reg[A] = ((reg[A] & 0xFFFF00) | (value >> 16));
			} //LDCH
			else{
				if(opcode == 0x00) //LDA
					regnum = A;
				else if(opcode == 0x68) //LDB
					regnum = B;
				else if(opcode == 0x74) //LDT
					regnum = T;
				else if(opcode == 0x08) //LDL
					regnum = L;
				else if(opcode == 0x04) //LDX
					regnum = X;
				reg[regnum] = value; //save the value to the register
			}
		}
		else if(opcode == 0x48){ // JSUB
			target = getTarget(curr, reg, nixbpe);
			reg[L] = reg[PC];
			reg[PC] = target;
		}
		else if(opcode == 0x28){ // COMP
			target = getTarget(curr, reg, nixbpe);
			value = calcVal(target, nixbpe);
			if( reg[A] > value)
				reg[SW] = '>';
			else if (reg[A] < value)
				reg[SW] = '<';
			else
				reg[SW] = '=';
		}
		else if(opcode == 0x3C || opcode == 0x30 || opcode == 0X38 ){ // J, JEQ, JLT
			target = getTarget(curr, reg, nixbpe);
			if(opcode == 0x3C)
				reg[PC] = target;
			else if(opcode == 0x30 && reg[SW] == '=')
				reg[PC] = target;
			else if(opcode == 0x38 && reg[SW] == '<')
				reg[PC] = target;		
		}
		else if(opcode == 0xB4){ // CLEAR
			reg[PC] += 2;
			regnum = (memory[curr + 1] & 0xF0) >> 4;
			reg[regnum] = 0;
		}
		else if(opcode == 0xE0 || opcode == 0xDC || opcode == 0xD8){ // TD, WD, RD
			reg[PC] += (3 + ((memory[curr+1] & 0x10) >> 4));
			if(opcode == 0xE0)
				reg[SW] = '<';
		}
		else if(opcode == 0xA0){ // COMPR
			reg[PC] += 2;
			regnum = ((memory[curr + 1] & 0xF0) >> 4);
			regnum2 = (memory[curr + 1] & 0x0F);

			if(reg[regnum] > reg[regnum2])
				reg[SW] = '>';
			else if(reg[regnum] < reg[regnum2])
				reg[SW] = '<';
			else
				reg[SW] = '=';		
		}
		else if(opcode == 0x4C){ // RSUB
			reg[PC] = reg[L];
		}
		else if(opcode == 0x2C){ // TIX
			target = getTarget(curr, reg, nixbpe);
			value = calcVal(target, nixbpe);
			reg[X] += 1;
			if(reg[X] > value)
				reg[SW] = '>';
			else if(reg[X] < value)
				reg[SW] = '<';
			else
				reg[SW] = '=';
		}
		else if(opcode == 0xB8){ // TIXR
			reg[PC] += 2;
			regnum = ((memory[curr+1] & 0xF0) >> 4);
			reg[X] += 1;
			if(reg[X] > reg[regnum])
				reg[SW] = '>';
			else if(reg[X] < reg[regnum])
				reg[SW] = '<';
			else
				reg[SW] = '=';
		}	
		curr = reg[PC];
		if(bptab[curr] == 1){ //when pc points to the breakpoint print registers
				bpflag = 1;
				end_exec = curr;
				printReg(reg);
				printf("\tStop at checkpoint [%X]\n",curr);
				addHistory(cmd);
				return;
		}
	}
	//program ends, reset values
	end_exec = progaddr;
	bpflag = -1;
	printReg(reg);
	memset(reg, 0, sizeof(reg));
	printf("\tEnd Program\n");
	addHistory(cmd);
	return;
}
