#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include "so_stdio.h"
#include <string.h>
#include <Windows.h>
#define BUFFER_SIZE (4096)

struct _so_file {
    HANDLE _handle; // handle ul fisierului
    int _mode; //modul de deschidere al fisierului
    int _lastOperation; //ultima operatie: 1-write, 2-read
    char _buffer[BUFFER_SIZE]; // bufferul in care scriem sau din care citim 
    int _end; //end of file
    int _error; // eroare
    int _posBuffer; //pozitia actuala in buffer
    DWORD _posFile; //pozitia in fisier
    DWORD _nrElem; //cati bytes am in buffer
    int _f;
    int _fromRead;
    PROCESS_INFORMATION _pi; // pid ul procesului copil
};

SO_FILE* so_fopen(const char* pathname, const char* mode)
{
    SO_FILE* f = (SO_FILE*)malloc(sizeof(SO_FILE));
    if (f == NULL) 
    {
        printf("Malloc error.\n");
        free(f);
        return NULL;
    }

    f->_mode = 0;
    if (mode == "r")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 1;
    }
    else if (mode == "r+")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 2;
    }
    else if (mode == "w")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 3;
    }
    else if (mode == "w+")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 4;
    }
    else if (mode == "a")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            FILE_APPEND_DATA | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 5;
    }
    else if (mode == "a+")
    {
        f->_handle = CreateFileA((LPCSTR)pathname,
            FILE_APPEND_DATA | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        f->_mode = 6;
    }
    else
    {
        printf("Mode error.\n");
        f->_error = 1;
        free(f);
        return NULL;
    }

    if (f->_handle == INVALID_HANDLE_VALUE)
    {
        printf("Handle error.\n");
        free(f);
        return NULL;
    }

    f->_end = 0;
    f->_error = 0;
    f->_lastOperation = 0;
    f->_nrElem = 0;
    f->_posBuffer = 0;
    f->_posFile = 0;
    f->_f = 0;
    f->_fromRead = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        f->_buffer[i] = 0;
    }

    return f;
}

int so_fclose(SO_FILE* stream)
{
    if (stream->_lastOperation == 1 && stream->_f == 0)
    {
        int b = so_fflush(stream);
        if (b < 0)
        {
            stream->_error = 1;
            free(stream);
            return SO_EOF;
        }

        int bb = CloseHandle(stream->_handle);
        if (bb == 0) {
            stream->_error = 1;
            if (stream != NULL) {
                stream->_handle = INVALID_HANDLE_VALUE;
                free(stream);
                stream = NULL;
            }
        }
        if (stream != NULL) {
            free(stream);
            stream = NULL;
        }
        else
        {
            return SO_EOF;
        }
    }

    return 0;
}

HANDLE so_fileno(SO_FILE* stream)
{
    return stream->_handle;
}

int so_feof(SO_FILE* stream)
{
    return stream->_end;
}

int so_ferror(SO_FILE* stream)
{
    return stream->_error;
}

int so_fflush(SO_FILE* stream)
{
    if (stream->_lastOperation == 1 && stream->_f==0 && stream->_posBuffer!=0)
    {
        DWORD bit;
        int ret = WriteFile(
            stream->_handle,
            &stream->_buffer, 
            stream->_posBuffer,
            &bit, 
            NULL  
        );
        if (ret == 0) {
            printf("FFLUSH error\n");
            stream->_error = 1;
            return SO_EOF;
        }

        stream->_posBuffer = 0;
        return 0;
    }
    else {
        stream->_posBuffer = 0;
        stream->_f = 1;
        return 0;
    }
    return SO_EOF;
}

long so_ftell(SO_FILE* stream)
{
    if (stream->_posFile >= 0)
        return stream->_posFile;
    else
        return -1;
}

int testReading(SO_FILE* stream, int m)
{
    if (m == 1 || m == 2 || m == 6 || m == 4)
    {
        return 1;
    }
    return 0;
}

int testWriting(SO_FILE* stream, int m)
{
    if (m == 3 || m == 4 || m == 2 || m == 5 || m == 6)
    {
        return 1;
    }
    return 0;
}

int so_fgetc(SO_FILE* stream)
{
    if (testReading(stream, stream->_mode) == 1)
    {
        DWORD bit;
        if (stream->_posBuffer == 0 || stream->_posBuffer == BUFFER_SIZE || stream->_lastOperation == 1)
        {
            int b = ReadFile(
                stream->_handle,  
                &stream->_buffer,  
                BUFFER_SIZE, 
                &bit, 
                NULL
            );
            if (b == 0)
            {
                stream->_error = 1;
                printf("FGETC error\n");
                return SO_EOF;
            }

            stream->_lastOperation = 2;
            stream->_posBuffer = 0;
        }

        if (stream->_fromRead == 0 && stream->_buffer[stream->_posBuffer] == '\0')
        {
            stream->_end = 1;
            return SO_EOF;
        }

        stream->_lastOperation = 2;
        int pos = stream->_posBuffer;
        stream->_posBuffer++;
        stream->_posFile++;
        return (int)stream->_buffer[pos];
    }
    else
    {
        printf("Permision of reading error\n");
        stream->_error = 1;
        return SO_EOF;
    }
}

size_t so_fread(void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{
    if (stream->_mode == 5)
    {
        so_fseek(stream, 0, SEEK_END);
    }
    else
    {
        so_fseek(stream, 0, SEEK_SET);
    }

    int nr = nmemb * size;
    int count = 0;
    stream->_fromRead = 1;

    char* v = (char*)malloc(sizeof(char) * nr);
    for (int i = 0; i < nr; i++)
        v[i] = 0;

    while (count < nr)
    {
        if (stream->_end == 0)
        {
            unsigned char c = so_fgetc(stream);
            v[count] = (unsigned char)c;
            if (stream->_error == 1)
            {
                free(v);
                stream->_fromRead = 0;
                return 0;
            }
            count++;
        }
        else
        {
            stream->_buffer[count] = '\0';
            stream->_lastOperation = 2;
            memcpy(ptr, v, count);
            free(v);
            stream->_fromRead = 0;
            return count;
        }
    }

    if (stream->_posFile == SEEK_END)
    {
        stream->_end = 1;
    }

    stream->_lastOperation = 2;
    memcpy(ptr, v, nr);
    free(v);
    stream->_fromRead = 0;
    return count;

}

int so_fputc(int c, SO_FILE* stream)
{

    if (testWriting(stream, stream->_mode) == 1)
    {
        if (stream->_posBuffer == BUFFER_SIZE)
        {
            stream->_f = 0;
            int b = so_fflush(stream);
            if (b == SO_EOF)
            {
                printf("FPUTC error\n");
                stream->_error = 1;
                return SO_EOF;
            }
        }

        stream->_buffer[stream->_posBuffer] = (char)c;
        stream->_posBuffer++;
        stream->_posFile++;
        stream->_nrElem++;

    }
    else
    {
        printf("Permision of Writing error\n");
        stream->_error = 1;
        return SO_EOF;
    }
    stream->_lastOperation = 1;
    return c;
}

size_t so_fwrite(const void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{
    int nr = size * nmemb;
    char* aux = (char*)ptr;
    int count = 0;
    if (stream->_mode == 5 || stream->_mode == 6)
    {
        int b = so_fseek(stream, 0, SEEK_END);
        if (b == -1)
        {
            stream->_error = 1;
            return 0;
        }
    }

    while (count < nmemb)
    {
        so_fputc((int)aux[count], stream);
        if (stream->_error == 1) {
            return 0;
        }
        count++;
    }
    stream->_lastOperation = 1;

    if (count == 0)
    {
        stream->_error = 1;
        return 0;
    }

    return count;
}

int so_fseek(SO_FILE* stream, long offset, int whence)
{
    if (stream->_lastOperation == 1) {
        int b = so_fflush(stream);
        if (b == SO_EOF) {
            printf("FSEEK error\n");
            stream->_error = 1;
            return -1;
        }
    }

    int p = SetFilePointer(
        stream->_handle,
        offset, 
        NULL, 
        (DWORD)whence 
    );
    if (p == INVALID_SET_FILE_POINTER)
    {
        return -1;
    }

    stream->_posFile = p;
    return 0;
}

SO_FILE* so_popen(const char* command, const char* type)
{
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    HANDLE hRead, hWrite, hProcess=NULL, hChild=NULL;
    char com_exec[1024];
    strcpy(com_exec, command);

    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    ZeroMemory(&pi, sizeof(pi));

    int b = CreatePipe(
        &hRead,   
        &hWrite, 
        &sa, 
        0
    );
    if (b == -1) 
    {
        return NULL;
    }


    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    si.dwFlags |= STARTF_USESTDHANDLES;

    if (strcmp(type, "r")==0) 
    {
        hProcess = hRead;
        hChild = hWrite;
        si.hStdInput = hWrite;
    }
    else if (strcmp(type, "w")==0) 
    {
        hProcess = hWrite;
        hChild = hRead;
        si.hStdInput = hRead;
    }

    SetHandleInformation(hProcess, HANDLE_FLAG_INHERIT, 0);

    b = CreateProcess(
        NULL, 
        (LPCSTR)com_exec,  
        NULL,  
        NULL,  
        TRUE,  
        0,  
        NULL, 
        NULL, 
        &si, 
        &pi  
    );
    if (b == -1) 
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return NULL;
    }

    CloseHandle(hChild);

    SO_FILE* f = (SO_FILE*)malloc(sizeof(SO_FILE));
    if (f == NULL) 
    {
        printf("Cannot allocate memory\n");
        CloseHandle(hProcess);
        free(f);
        return NULL;
    }
    f->_handle = hProcess;

    if (strcmp(type, "r") == 0)
        f->_mode = 1;
    else if(strcmp(type, "w") == 0)
        f->_mode = 3;
    else
    {
        printf("Opening flag error.\n");
        f->_error = 1;
        free(f);
        return NULL;
    }

    f->_lastOperation = 0;
    f->_posBuffer = 0;
    f->_posFile = 0;
    f->_end = 0;
    f->_error = 0;
    f->_nrElem= 0;
    f->_pi.hProcess = pi.hProcess;
    f->_pi.hThread = pi.hThread;

    for (int i = 0; i < BUFFER_SIZE; i++)
        f->_buffer[i] = 0;

    return f;
}

int so_pclose(SO_FILE* stream)
{

    if ((stream->_pi.hProcess == INVALID_HANDLE_VALUE) || (stream->_pi.hThread == INVALID_HANDLE_VALUE)) 
    {
        return SO_EOF;
    }
    else 
    {
        PROCESS_INFORMATION p = stream->_pi;

        int check = so_fclose(stream);
        if (check < 0) 
        {
            return SO_EOF;
        }

        check = WaitForSingleObject(p.hProcess, INFINITE);
        if (check == WAIT_FAILED) 
        {
            return SO_EOF;
        }

        int status = GetExitCodeProcess(p.hProcess, &check);
        CloseHandle(p.hProcess);
        CloseHandle(p.hThread);

        return check;
    }
}

