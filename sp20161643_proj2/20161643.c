#include "20161643.h"

int main() {
	char command[100] = {'\0',}; // array to save command
	char trimCmd[100] = {'\0',}; // array to save trimmed command
	int cmdNum; // variable to save command number
	History* temp; // variable to free history link
	int i;
	opNode *curr, *prev;
	Symbol *sym;
	Object *node;	
	funcPtr fPtr[] = { cmdHelp, cmdDir, cmdQuit, cmdHistory, cmdDump, cmdEdit, cmdFill, cmdReset, cmdOpcode, cmdOpcodelist, cmdAssemble, cmdType, cmdSymbol}; // function pointer to reduce the code
	loadOpcode(); // load opcode list from opcode.txt
	while (!endFlag) {
		memset(command, '\0', sizeof(command));
		memset(trimCmd, '\0', sizeof(trimCmd));
		printf("sicsim>");
		fgets(command, sizeof(command), stdin);
		command[strlen(command) - 1] = '\0';
		cmdNum = checkCmd(command, trimCmd); // check the command number
		if (cmdNum == -1) 
			printf("Invalid Command\n");
		else {
			fPtr[cmdNum](trimCmd); // call the function that has the right command number
		}
	}
	while(head){ // free history list
		temp = head;
		head = head->link;
		free(temp);	
	}
	for(i=0; i < 20; i++){ // free opcode list
		curr = opList[i];
		while(curr){
			prev = curr;
			curr = curr->link;
			free(prev);
		}
	}
	for (i = 0; i < 26; i++) { // free symbol list
			while (symList[i]) {
				sym = symList[i];
				symList[i] = symList[i]->link;
				free(sym);
			}
	}
	if(saveSym != NULL) // free saved symbol
		free(saveSym);
	if(objfree != 1){
		while (ohead) { // free object list
			node = ohead;
			ohead = ohead->link;
			free(node);
		}
	}
	return 0;
}
