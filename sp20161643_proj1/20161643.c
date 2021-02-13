#include "20161643.h"

int main() {
	char command[100]; // array to save command
	char trimCmd[100]; // array to save trimmed command
	int cmdNum; // variable to save command number
	History* temp; // variable to free history link
	int i;
	opNode *curr, *prev;
	funcPtr fPtr[] = { cmdHelp, cmdDir, cmdQuit, cmdHistory, cmdDump, cmdEdit, cmdFill, cmdReset, cmdOpcode, cmdOpcodelist}; // function pointer to reduce the code
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
	return 0;
}
