#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

void showShortHelp(char** argv);
void showFullHelp(char** argv);
void setup();
void uninstall();
void copyToClipBoard(const char* content);

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
        if(strcmp(argv[1], "/c") == 0) {
            // Open file in read mode & Find its size & Read its content & Store it to clipboard
            FILE* file = fopen(argv[2], "r");

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);

            fseek(file, 0, SEEK_SET);
            char* content = malloc(fileSize + 1); // +1 in order to null-terminate the string
            fread(content, fileSize, 1, file);
            content[fileSize] = '\0';

            fclose(file);
            copyToClipBoard(content);
            free(content);
        } else if(strcmp(argv[1], "/p") == 0) {
            // Retrieve file path from command-line & Store it to clipboard
            const char* filePath = argv[2];
            copyToClipBoard(filePath);
        }  else if(strcmp(argv[1], "/d") == 0) {
            // Start a shell with the supplied path as working directory
            char* commandBuffer = malloc(strlen(argv[2])+15);
            sprintf(commandBuffer, "cmd /k \"cd /d %s\"",argv[2]);
            system(commandBuffer);
        }
        else
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
	printf("\t/p <filePath|directoryPath> - Copy the absolute path of what is located at either <filePath> or <directoryPath>\n");
	printf("\t/d <directoryPath> - Open a shell with the working directory pointing to <directoryPath>");
	printf("\nImportant:\n");
	printf("\tIT IS CRUCIAL TO RUN WINTOOLS AS ADMINISTRATOR WHEN YOU WANT TO INSTALL/UNINSTALL IT.\n");
	printf("\tThat's because it modifies registry keys !\n");
}

void setup()
{
    HKEY hKeyCurrentUser, hKeyLocalMachine, hKeyTemp;
    char* currentDir;
    currentDir = _getcwd(NULL, 0);
    char* currentFile = malloc(strlen(currentDir) + strlen("\\Wintools"));
    strcpy(currentFile, currentDir);
    strcat(currentFile, "\\Wintools");
    char *tmpCommand = "%s %s", *copyPathCommand = "/p %1", *copyContentCommand = "/c %1", *openShellCommand = "/d %1";
    char* builtCommand;

    // Create a menu for the files
    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\*\\shell\\Wintools", NULL, NULL,
                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyCurrentUser, NULL);

    // Create a value for the Wintools cascading menu name + sub commands
    RegSetValueEx(hKeyCurrentUser, TEXT("MUIVerb"), 0, REG_SZ, (LPBYTE)"Wintools", strlen("Wintools")*sizeof(char));
    RegSetValueEx(hKeyCurrentUser, TEXT("SubCommands"), 0, REG_SZ, (LPBYTE)"copyPath;copyContent;openShell", strlen("copyPath;copyContent")*sizeof(char));



    // Same for folders, without the copyContent function, with openShell function
    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\directory\\shell\\Wintools", NULL, NULL,
                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyCurrentUser, NULL);
    RegSetValueEx(hKeyCurrentUser, TEXT("MUIVerb"), 0, REG_SZ, (LPBYTE)"Wintools", strlen("Wintools")*sizeof(char));
    RegSetValueEx(hKeyCurrentUser, TEXT("SubCommands"), 0, REG_SZ, (LPBYTE)"copyPath;openShell", strlen("copyPath;openShell")*sizeof(char));
	
    // Release the key
    RegCloseKey(hKeyCurrentUser);

    // Register shortcuts to each cascading menu action
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell", 0, KEY_QUERY_VALUE, &hKeyLocalMachine);

    // Actions

    // Copy path
    RegCreateKeyEx(hKeyLocalMachine, "copyPath", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)"Copy path", strlen("Copy path")*sizeof(char));
    RegCreateKeyEx(hKeyTemp, "command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);

    builtCommand = malloc(strlen(currentFile) + strlen(copyPathCommand) + 1);
    sprintf(builtCommand, tmpCommand, currentFile, copyPathCommand);
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)builtCommand, strlen(builtCommand)*sizeof(char));
    free(builtCommand);

    // Copy content
    RegCreateKeyEx(hKeyLocalMachine, "copyContent", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)"Copy content", strlen("Copy content")*sizeof(char));
    RegCreateKeyEx(hKeyTemp, "command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);

    builtCommand = malloc(strlen(currentFile) + strlen(copyContentCommand) + 1);
    sprintf(builtCommand, tmpCommand, currentFile, copyContentCommand );
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)builtCommand, strlen(builtCommand)*sizeof(char));
    free(builtCommand);

    // Open shell
    RegCreateKeyEx(hKeyLocalMachine, "openShell", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)"Open shell here", strlen("Open shell here")*sizeof(char));
    RegCreateKeyEx(hKeyTemp, "command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTemp, NULL);

    builtCommand = malloc(strlen(currentFile) + strlen(openShellCommand) + 1);
    sprintf(builtCommand, tmpCommand, currentFile, openShellCommand );
    RegSetValueEx(hKeyTemp, TEXT(""), 0, REG_SZ, (LPBYTE)builtCommand, strlen(builtCommand)*sizeof(char));
    free(builtCommand);

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
}

void copyToClipBoard(const char* content)
{
    const size_t contentLength = strlen(content) + 1;

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

