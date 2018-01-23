#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define NAME "Wintools"
#define PATH "\\Wintools"
#define FILE_ACTIONS_LEN 3
#define DIR_ACTIONS_LEN 2
#define ALL_ACTIONS_LEN 4

void showShortHelp(char** argv);
void showFullHelp(char** argv);
void setup();
void uninstall();
void copyToClipBoard(const char* content);
void pasteClipboardToFile(const char* filepath);
	
typedef struct  {
	char* displayName;
	char* registryName;
	char* commandLineOption;
} MenuAction;

typedef MenuAction MenuActions[];

// List of actions supported in the context menu
MenuActions allActions = {
	{ "Copy path","copyPath","/p %1" },
	{ "Copy content","copyContent","/c %1"},
	{ "Paste content","pasteContent","/v %1" },
	{ "Open shell here","openShell","/d %1" }
};


int main(int argc, char** argv)
{

    /* I have chosen to use Windows-like command-line parameters as this tool works only on this OS.
     * If we assume <d> is a parameter :
     * 		Windows-like : ./Wintools /d   <---- This one ! <3
     *  	Unix-like : ./wintools -d
     * */

    // No argument, <exit>
    if(argc == 1) {
        setup();
        return 0;
    }

    // One argument, either <help> or <setup> or <uninstall>
    if(argc == 2) {
        if(strcmp(argv[1], "/h") == 0)
            showFullHelp(argv);
        else if(strcmp(argv[1], "/u") == 0)
            uninstall();
        else
            showShortHelp(argv);
		
		return 0;
    }

    // Two arguments, first is either <copy content to clipboard> or <copy absolute path to clipboard> or <open shell here>
    // second is <file path>
    if(argc == 3) {
        if(strcmp(argv[1], "/c") == 0) { // Copy content to clipboard
            // Open file in read mode & Find its size & Read its content & Store it to clipboard
            FILE* file = fopen(argv[2], "r");

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);

            rewind(file);
            char* content = calloc(1, fileSize + 1); // +1 in order to null-terminate the string
            fread(content, fileSize, 1, file);
            content[fileSize] = '\0';

            fclose(file);
            copyToClipBoard(content);
            free(content);
        } else if(strcmp(argv[1], "/p") == 0) { // Copy absolute path to clipboard
            // Retrieve file path from command-line & Store it to clipboard
            const char* filePath = argv[2];
            copyToClipBoard(filePath);
        }  else if(strcmp(argv[1], "/d") == 0) { // Open shell here
            // Start a shell with the supplied path as working directory
            char* commandBuffer = malloc(strlen(argv[2])+15);
            sprintf(commandBuffer, "cmd /k \"cd /d %s\"",argv[2]);
            system(commandBuffer);
        } else if(strcmp(argv[1], "/v") == 0) { // Paste clipboard in file
			pasteClipboardToFile(argv[2]);
		} else
            showShortHelp(argv);
    }

    return 0;
}

void showShortHelp(char** argv)
{
    printf("\nPlease use this tool with the Windows contextual menu only.\n");
    printf("\nCurious ? You can grab some help with : %s /h.\n", argv[0]);
}

void showFullHelp(char** argv)
{
	printf("Wintools - Make your contextual menu better\n");
	printf("\nUsage : %s | %s [/option] [path]\n", argv[0], argv[0]);
	printf("\nIf no options are provided, Wintools will install its functionalities into the contextual menu\n");
	printf("\nOptions:\n");
	printf("\t/u - Unintall Wintools from the contextual menu\n");
	printf("\t/c <filePath> - Copy content of the file at <filepath> to the clipboard\n");
	printf("\t/v <filePath> - Paste what's inside the clipboard to the file at <filePath>\n");
	printf("\t/p <filePath|directoryPath> - Copy the absolute path of what is located at either <filePath> or <directoryPath>\n");
	printf("\t/d <directoryPath> - Open a shell with the working directory pointing to <directoryPath>");
	printf("\nImportant:\n");
	printf("\tIT IS CRUCIAL TO RUN WINTOOLS AS ADMINISTRATOR WHEN YOU WANT TO INSTALL/UNINSTALL IT.\n");
	printf("\tThat's because it modifies registry keys !\n");
}

void setup()
{

	// Registry key handlers
    HKEY hKeyCurrentUser, hKeyLocalMachine, hKeyTemp;
	
	// Retrieve absolute file path of this program
    char* currentDir =  _getcwd(NULL, 0);
	
	char* currentFile = malloc(strlen(currentDir) + strlen(PATH)+1); strcpy(currentFile, currentDir); strcat(currentFile, PATH); strcat(currentFile,"\0");
	

	// List of actions only available on right-clicking a file
	MenuActions fileActions = {allActions[0], allActions[1], allActions[2]};
	char* fileActionsAsRegistryString = calloc(1,1);
	for(int i = 0; i < FILE_ACTIONS_LEN; i++) {
		char* tmp = realloc(fileActionsAsRegistryString, sizeof(fileActionsAsRegistryString) + 1 + sizeof(fileActions[i].registryName)); // Clean realloc
		strcat(tmp,fileActions[i].registryName);
		strcat(tmp,";");
		fileActionsAsRegistryString = tmp;
	}

	// List of actions only avaiable on right-clicking a directory
	MenuActions dirActions = {allActions[0], allActions[3]};
	char* dirActionsAsRegistryString = calloc(1,1);
	for(int i = 0; i <  DIR_ACTIONS_LEN; i++) {
		char* tmp = realloc(dirActionsAsRegistryString, sizeof(dirActionsAsRegistryString) + 1 + sizeof(dirActions[i].registryName));
		dirActionsAsRegistryString = 
		strcat(dirActionsAsRegistryString,dirActions[i].registryName);
		strcat(dirActionsAsRegistryString,";");
		dirActionsAsRegistryString = tmp;
	}
	
	// First interpolated string is the absolute path of Wintools. Second one is the parameters used to start a command (e.g. /p %1)
    char *commandTemplate = "%s %s";
	
	// String used multiple times to hold the result of sprintf used with commandTemplate for each command
	char* builtCommand;
	
    // Open the current user key to create the cascading menu
    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\*\\shell\\Wintools",NULL, NULL,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyCurrentUser, NULL);
    
	// Create a menu for the files
	RegSetValueEx(hKeyCurrentUser, TEXT("MUIVerb"), 0, REG_SZ, (LPBYTE)NAME, strlen(NAME));
    RegSetValueEx(hKeyCurrentUser, TEXT("SubCommands"), 0, REG_SZ, (LPBYTE)fileActionsAsRegistryString, strlen(fileActionsAsRegistryString));

    // Create a menu for the folders
    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\directory\\shell\\Wintools",NULL, NULL,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyCurrentUser, NULL);
    RegSetValueEx(hKeyCurrentUser, TEXT("MUIVerb"), 0, REG_SZ, (LPBYTE)NAME, strlen(NAME));
    RegSetValueEx(hKeyCurrentUser, TEXT("SubCommands"), 0, REG_SZ, (LPBYTE)dirActionsAsRegistryString , strlen(dirActionsAsRegistryString));
	
    // Release the key
    RegCloseKey(hKeyCurrentUser);
	
	// Free the variables
	free(fileActionsAsRegistryString); free(dirActionsAsRegistryString);

    // Open the local machine key to register each action to a command-line call
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell", 0, KEY_QUERY_VALUE, &hKeyLocalMachine);

    for(int i = 0; i < ALL_ACTIONS_LEN; i++) {
		RegCreateKeyEx(hKeyLocalMachine, allActions[i].registryName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);
		RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)allActions[i].displayName, strlen(allActions[i].displayName));
		RegCreateKeyEx(hKeyTemp, "command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);

		builtCommand = calloc(1,strlen(currentFile) + strlen(allActions[i].commandLineOption) + 1);
		sprintf(builtCommand, commandTemplate, currentFile, allActions[i].commandLineOption);
		RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)builtCommand, strlen(builtCommand));
	}
	free(builtCommand);
	free(currentFile);

	// Close keys
    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyLocalMachine); 
}

void uninstall()
{
	RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\directory\\shell\\Wintools");
	RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\*\\shell\\Wintools");
	RegDeleteKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\copyPath");
	RegDeleteKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\copyContent");
	RegDeleteKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\openShell");
	RegDeleteKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\pasteClipboard");
}

void copyToClipBoard(const char* content)
{
    const size_t contentLength = (strlen(content)+1)*sizeof(char);

    // Lock and allocate memory to store the content & Unlock at the end
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, contentLength);
    memcpy(GlobalLock(hMem), content, contentLength);
    GlobalUnlock(hMem);

    // Open the clipboard & Empty it & Fill it with the content & Close it
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void pasteClipboardToFile(const char* filepath) {
	HANDLE h;
	
	// Open & Retrieve & Close clipboard
	OpenClipboard(0);
	h = GetClipboardData(CF_TEXT);
	CloseClipboard();
	
	// Write to file
	FILE *file = fopen(filepath, "w");
	fputs((char*)h, file);
	fclose(file);
}
