#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

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

bool endFlag = FALSE; // if input is q or quit, set the flag true
bool errFlag = FALSE; // when there is an invalid input in hex to decimal function
History *head = NULL, *tail = NULL; // pointer for the front and end of history list
unsigned char memory[1048576]; // for memory space 1048756 = 16 ^ 5
int dumpEnd = 0; // the last memory address which was printed
opNode* opList[20]; // array for opcode list

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
	char temp[100]; // 
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
	tcmd[l + 1] = '\0';

	for (i = 0; i < strlen(tcmd); i++) { //for commands with variables, find the command
		if (tcmd[i] == ' ' || tcmd[i] == '\0')
			break;
		temp[i] = tcmd[i];
	}
	temp[i + 1] = '\0';

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
// function to show the number of the opcode mnemonic
void cmdOpcode(char cmd[]) {
	int i, j = 0, idx = 0, cmdLen = 0;
	opNode* curr;
	int hash;
	char instruction[10] = {'\0', }; // to save the mnemonic
	cmdLen = strlen(cmd);
	while (1) {
		if (cmd[idx] == ' ' || cmd[idx] == '\0')
			break;
		idx++;
	} // where the command ends
	if (idx == cmdLen) {
		printf("Mnemonic is required\n");
		return;
	} // if there isn't a mnemonic, error
	for (i = idx; i < cmdLen; i++) {
		if (cmd[i] != ' ') {
			instruction[j] = cmd[i];
			j++;
		}
	} // copy the mnemonic to array instruction
	hash = createHash(instruction);
	curr = opList[hash];
	while (curr && strncmp(curr->mnemonic, instruction, strlen(curr->mnemonic))) {
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
