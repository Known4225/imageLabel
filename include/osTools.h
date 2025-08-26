/*
 ██████╗ ███████╗████████╗ ██████╗  ██████╗ ██╗     ███████╗   ██╗  ██╗
██╔═══██╗██╔════╝╚══██╔══╝██╔═══██╗██╔═══██╗██║     ██╔════╝   ██║  ██║
██║   ██║███████╗   ██║   ██║   ██║██║   ██║██║     ███████╗   ███████║
██║   ██║╚════██║   ██║   ██║   ██║██║   ██║██║     ╚════██║   ██╔══██║
╚██████╔╝███████║   ██║   ╚██████╔╝╚██████╔╝███████╗███████║██╗██║  ██║
 ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
https://patorjk.com/software/taag/#p=display&f=ANSI%20Shadow
*/

#ifndef OS_TOOLS_H
#define OS_TOOLS_H

#include "list.h"
#include "glad.h"
#include "glfw3.h"

/* required forward declarations (for packaging) */
typedef struct GLFWcursor GLFWcursor;
extern const uint32_t moveCursorData[1024];
extern const uint32_t leftDiagonalCursorData[1024];
extern const uint32_t rightDiagonalCursorData[1024];
#define GLFW_DLESIZE_CURSOR     0x00036007
#define GLFW_DRESIZE_CURSOR     0x00036008
#define GLFW_MOVE_CURSOR        0x00036009

typedef struct {
    GLFWwindow *osToolsWindow;
    GLFWcursor *standardCursors[10];
} osToolsGLFWObject;

typedef struct {
    const char *text; // clipboard text data (heap allocated)
} osToolsClipboardObject;

typedef struct {
    char executableFilepath[4096 + 1]; // filepath of executable
    list_t *selectedFilenames; // output filenames from osToolsFileDialogPrompt (erased with every call, only has multiple items when multiselect is used)
    list_t *globalExtensions; // list of global extensions accepted by file dialog
} osToolsFileDialogObject;

typedef struct {
    list_t *mappedFiles;
} osToolsMemmapObject;

/* global objects */
extern osToolsGLFWObject osToolsGLFW;
extern osToolsClipboardObject osToolsClipboard;
extern osToolsFileDialogObject osToolsFileDialog;
extern osToolsMemmapObject osToolsMemmap;

/* OS independent functions */
void osToolsIndependentInit(GLFWwindow *window);

/* returns clipboard text */
const char *osToolsClipboardGetText();

/* takes null terminated strings */
int32_t osToolsClipboardSetText(const char *input);

/*
GLFW_ARROW_CURSOR
GLFW_IBEAM_CURSOR
GLFW_CROSSHAIR_CURSOR
GLFW_HAND_CURSOR
GLFW_HRESIZE_CURSOR
GLFW_VRESIZE_CURSOR
GLFW_DLESIZE_CURSOR
GLFW_DRESIZE_CURSOR
GLFW_MOVE_CURSOR
*/
void osToolsSetCursor(uint32_t cursor);

void osToolsHideAndLockCursor();

void osToolsShowCursor();

typedef enum {
    OSTOOLS_CSV_ROW = 0,
    OSTOOLS_CSV_COLUMN = 1,
    OSTOOLS_CSV_FIELD_DOUBLE = 2,
    OSTOOLS_CSV_FIELD_INT = 3,
    OSTOOLS_CSV_FIELD_STRING = 4,
} osToolsCSV;

list_t *osToolsLoadInternal(char *filename, osToolsCSV rowOrColumn, char delimeter, osToolsCSV fieldType);

/* packages a CSV file into a list (headers are strings, all fields are doubles) - use OSTOOLS_CSV_ROW to put it in a list of lists where each list is a row of the CSV and use OSTOOLS_CSV_COLUMN to output a list of lists where each list is a column of the CSV */
list_t *osToolsLoadCSV(char *filename, osToolsCSV rowOrColumn);

/* packages a CSV file into a list (headers are strings, all fields are doubles) - use OSTOOLS_CSV_ROW to put it in a list of lists where each list is a row of the CSV and use OSTOOLS_CSV_COLUMN to output a list of lists where each list is a column of the CSV */
list_t *osToolsLoadCSVDouble(char *filename, osToolsCSV rowOrColumn);

/* packages a CSV file into a list (headers are strings, all fields are ints) - use OSTOOLS_CSV_ROW to put it in a list of lists where each list is a row of the CSV and use OSTOOLS_CSV_COLUMN to output a list of lists where each list is a column of the CSV */
list_t *osToolsLoadCSVInt(char *filename, osToolsCSV rowOrColumn);

/* packages a CSV file into a list (headers are strings, all fields are strings) - use OSTOOLS_CSV_ROW to put it in a list of lists where each list is a row of the CSV and use OSTOOLS_CSV_COLUMN to output a list of lists where each list is a column of the CSV */
list_t *osToolsLoadCSVString(char *filename, osToolsCSV rowOrColumn);

#ifdef OS_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shobjidl.h>

typedef struct {
    char name[32];
    HANDLE comHandle;
} win32ComPortObject;

extern win32ComPortObject win32com;

/* opens a com port */
int win32comInit(win32ComPortObject *com, char *name);

/* returns number of bytes sent */
int win32comSend(win32ComPortObject *com, unsigned char *data, int length);

/* returns number of bytes received */
int win32comReceive(win32ComPortObject *com, unsigned char *buffer, int length);

/* closes a com port */
int win32comClose(win32ComPortObject *com);

#define WIN32TCP_NUM_SOCKETS 32

typedef struct {
    char *address;
    char *port;
    SOCKET connectSocket[WIN32TCP_NUM_SOCKETS];
    char socketOpen[WIN32TCP_NUM_SOCKETS];
} win32SocketObject;

extern win32SocketObject win32Socket;

int win32tcpInit(char *address, char *port);

SOCKET *win32tcpCreateSocket();

int win32tcpSend(SOCKET *socket, unsigned char *data, int length);

int win32tcpReceive(SOCKET *socket, unsigned char *buffer, int length);

int win32tcpReceive2(SOCKET *socket, unsigned char *buffer, int length);

void win32tcpDeinit();
#endif

#ifdef OS_LINUX
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

int32_t osToolsInit(char argv0[], GLFWwindow *window);

/* add a single extension to the global file extensions */
void osToolsFileDialogAddGlobalExtension(char *extension);

/* copies the data from the list to the global extensions (you can free the list passed in immediately after calling this) */
void osToolsFileDialogSetGlobalExtensions(list_t *extensions);

/* 
openOrSave: 0 - open, 1 - save
multiselect: 0 - single file select, 1 - multiselect, can only use when opening (cannot save to multiple files)
folder: 0 - file dialog, 1 - folder dialog
filename: refers to autofill filename ("null" or empty string for no autofill)
extensions: pass in a list of accepted file extensions or pass in NULL to use global list of extensions
*/
int32_t osToolsFileDialogPrompt(int8_t openOrSave, int8_t multiselect, int8_t folder, char *filename, list_t *extensions);

uint8_t *osToolsMapFile(char *filename, uint32_t *sizeOutput);

int32_t osToolsUnmapFile(uint8_t *data);

/* lists files in a directory (does NOT list folders), format [name, size, name, size, ...] */
list_t *osToolsListFiles(char *directory);

/* non-recursive, lists folders in a directory, format [name, name, ...] */
list_t *osToolsListFolders(char *directory);

/* lists files and folders in a directory, format [name, size, name, size, ...] (size is -1 for folders) */
list_t *osToolsListFilesAndFolders(char *directory);

/* create a folder */
void osToolsCreateFolder(char *folder);

/* delete a folder (and all files and subfolders) */
void osToolsDeleteFolder(char *folder);

#endif /* OS_TOOLS_H */
