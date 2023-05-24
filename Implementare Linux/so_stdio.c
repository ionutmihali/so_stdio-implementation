#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include "so_stdio.h"
#include <string.h>

#define SIZE_BUFFER (4096)

typedef struct _so_file
{
    int _fd; //File descriptor
    int _posFile; //Pozitia in fisier
    int _posBuffer; //Pozitia in bufffer
    int _openFlag; //Tipul de fisier
    char _buffer[SIZE_BUFFER];
    int _lastOperation; //ultima operatie efectuata: 0-nicio operatie, 1-write, 2-read
    int _error; //Eroare
    int _end; //Am ajuns la sfarsitul fisierului
    int _nrElem; //Elemente citite
    int _f; //s-a efectuat fflush
    int _fromRead; //a venit din so_fread apelul functiei
    pid_t _pid; // pid-ul procesului copil
}SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    SO_FILE* f = (SO_FILE*)malloc(sizeof(SO_FILE));
    f->_openFlag=0;
    if (f == NULL) 
    {

		printf("Malloc error.");
        f->_error=1;
        free(f);
		return NULL;
	}

    if(strcmp(mode,"r")==0)
    {
        f->_fd = open(pathname, O_RDONLY);
        f->_openFlag = 1;
	}
    else if(strcmp(mode,"r+")==0)
    {
        f->_fd= open(pathname, O_RDWR);
        f->_openFlag = 2;
    }
    else if(strcmp(mode,"w")==0)
    {
        f->_fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0664);
        f->_openFlag = 3;
    }
    else if(strcmp(mode,"w+")==0)
    {
        f->_fd = open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0644);
        f->_openFlag = 4;
    }
    else if(strcmp(mode,"a")==0)
    {
        f->_fd = open(pathname, O_APPEND | O_WRONLY | O_CREAT, 0644);
        f->_openFlag = 5;
    }
    else if(strcmp(mode,"a+")==0)
    {
        f->_fd = open(pathname, O_APPEND | O_RDWR | O_CREAT, 0644);
        f->_openFlag = 6;
    }
    
    if(f->_openFlag == 0)
    {
        printf("Opening flag error.\n");
        f->_error=1;
        free(f);
        return NULL;
    }

    if (f->_fd < 0) {
		printf("Opening file error.\n");
        f->_error=1;
		free(f);
		return NULL;
	}

    f->_posBuffer=0;
    f->_posFile=0;
    f->_lastOperation=0;
    f->_error=0;
    f->_end=0;
    f->_nrElem=0;
    f->_f=0;
    f->_fromRead=0;
    f->_pid = -1;
    for(int i=0; i<SIZE_BUFFER; i++)
    {
        f->_buffer[i]=0;
    }

    return f;

}

int so_fclose(SO_FILE *stream)
{
    if(stream->_lastOperation == 1 && stream->_f==0)  
    {
		int b = so_fflush(stream);
        if(b<0)
        {
            stream->_error=1;
            free(stream);
            return SO_EOF;
        }
	}

	int ret = close(stream->_fd);
	if(ret < 0) 
    {
		if(stream != NULL) 
        {
			free(stream);
			stream = NULL;
		}
	}

	if(stream != NULL) 
    {
		free(stream);
		stream = NULL;
	}
    else
    {
        return SO_EOF;
    }

	return 0;
}

int so_fileno(SO_FILE *stream)
{
    return stream->_fd;
}

int so_feof(SO_FILE *stream)
{
    return stream->_end;
}

int so_ferror(SO_FILE *stream)
{
    return stream->_error;
}

int so_fflush(SO_FILE *stream)
{
    if(stream->_lastOperation == 1 && stream->_f==0 && stream->_posBuffer!=0)
    {
        int ret = write(stream->_fd, stream->_buffer, stream->_posBuffer);
		if(ret == -1) {
            printf("FFLUSH error\n");
            stream->_error=1;
			return SO_EOF; 
		}

        stream->_posBuffer=0;

		return 0;
	}
	else 
    {
        stream->_posBuffer=0;
        stream->_f=1;

		return 0;
	}
}

long so_ftell(SO_FILE *stream)
{
    if(stream->_posFile>=0)
        return stream->_posFile;
    else 
        return -1;
}

int testReading(SO_FILE *stream, int m)
{
    if(m==1 || m==2 || m==6 || m==4)
    {
        return 1;
    }
    return 0;
}

int testWriting(SO_FILE *stream, int m)
{
    if(m==3 || m==4 || m==2 || m==5 || m==6)
    {
        return 1;
    }
    return 0;
}

int so_fgetc(SO_FILE *stream)
{
    if(testReading(stream, stream->_openFlag)==1)
    {
        if(stream->_posBuffer==0 || stream->_posBuffer==SIZE_BUFFER || stream->_lastOperation==1)
        {
            int b = read(stream->_fd, stream->_buffer, SIZE_BUFFER);
            if(b==0)
            {
                stream->_end=1;
                return SO_EOF;
            }

            if(b==-1)
            {
                printf("FGETC error\n");
                stream->_error=1;
                return SO_EOF;
            }

            stream->_lastOperation =2;
            stream->_posBuffer=0;
        }

        if(stream->_fromRead==0 && stream->_buffer[stream->_posBuffer]=='\0')
        {
            stream->_end=1;
            return SO_EOF;
        }

        stream->_lastOperation=2;
        int pos = stream->_posBuffer;
        stream->_posBuffer++;
        stream->_posFile++;
        return (unsigned char)stream->_buffer[pos];
    }
    else
    {
        printf("Permision of reading error\n");
        stream->_error=1;
        return SO_EOF;
    }
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    if(stream->_openFlag==5)
    {
        so_fseek(stream,0, SEEK_END);
    }

    int nr = nmemb*size;
    int count = 0;
    char* v = (char*)malloc(sizeof(char)*nr);
 
    stream->_fromRead=1;
    
    while(count < nmemb)
    {
        if(stream->_end==0)
        {
            unsigned char c = so_fgetc(stream);
            v[count] = c;
            if(stream->_error == 1) 
            {
                free(v);
                stream->_fromRead=0;
                return 0;
            }
            count++;
        }
        else
        {
            stream->_lastOperation=2;
            memcpy(ptr, v, nr);
            free(v);
            stream->_fromRead=0;
            return count;
        }
    }

    if(stream->_posFile == SEEK_END)
    {
        stream->_end=1;
    }

    stream->_lastOperation=2;
    memcpy(ptr, v, nr);
    free(v);
    stream->_fromRead=0;
    return count;
}

int so_fputc(int c, SO_FILE *stream)
{
    
    if(testWriting(stream, stream->_openFlag)==1)
    {
        if(stream->_posBuffer == SIZE_BUFFER)
        {
            stream->_f=0;
			int b = so_fflush(stream);
			if(b == -1) 
            {
                printf("FPUTC error\n");
				stream->_error = 1;
				return SO_EOF;
			}
		}

        stream->_buffer[stream->_posBuffer]=(char)c;
        stream->_posBuffer++;
        stream->_posFile++;
        stream->_nrElem++;

    }
    else
    {
        printf("Permision of Writing error\n");
        stream->_error=1;
        return SO_EOF;
    }
    stream->_lastOperation=1;
    return c;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int nr = size* nmemb;
    char* aux = (char*)ptr;
    int count = 0;
    stream->_f=0;

    if(stream->_openFlag==5 || stream->_openFlag==6)
    {
        int b = so_fseek(stream, 0, SEEK_END);
        if(b==-1)
        {
            stream->_error=1;
            return SO_EOF;
        }
    }
    
    while(count < nmemb)
    {
        int b = so_fputc((int)aux[count], stream);
        if(stream->_error==1)
        {
            return SO_EOF;
        }
		count++;
    }
    stream->_lastOperation=1;

    if(count==0)
    {
        stream->_error=1;
        return SO_EOF;
    }
    
    return count;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if(stream->_lastOperation == 1) 
    {
		int b = so_fflush(stream);
		if(b == SO_EOF) {
            printf("FSEEK error\n");
            stream->_error=1;
			return SO_EOF;
		}
	}

	int p = lseek(stream->_fd, offset, whence);
	if(p==-1)
    {
        return -1;
    }
	stream->_posFile = p;
	return 0;
}

SO_FILE *so_popen(const char *command, const char *type) 
{
	
	int pipe_fd[2];
	int b = pipe(pipe_fd);
	if(b < 0) 
    {
		printf("Cannot create a pipe!");
		return NULL;
	}

    int fd = 0;
	pid_t pid = fork();

    if(pid < 0) 
    {
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return NULL;
	}
	else if (pid == 0) 
    {
		if(strcmp(type, "r") == 0) 
        {
			close(pipe_fd[0]);
			dup2(pipe_fd[1], STDOUT_FILENO);
			close(pipe_fd[1]);
		}
		else if(strcmp(type, "w") == 0)
        { 
			close(pipe_fd[1]);
			dup2(pipe_fd[0], STDIN_FILENO);
			close(pipe_fd[0]);
		}

		int ok = execlp("sh", "sh", "-c", command, NULL);
		exit(1);
	}
	else 
    {
		if(strcmp(type, "r") == 0) 
        {
			close(pipe_fd[1]);
			fd = pipe_fd[0];
		}
		else 
        { 
			close(pipe_fd[0]);
			fd = pipe_fd[1];
		}
	}

	SO_FILE* f = (SO_FILE*)malloc(sizeof(SO_FILE));
	if(f == NULL) 
    {
		printf("Cannot allocate memory\n");
		f->_error = 1;
		free(f);
		return NULL;
	}

	f->_fd = fd;

	if(strcmp(type,"r")==0)
    {
        f->_openFlag = 1;
	}
    else if(strcmp(type,"w")==0)
    {
        f->_openFlag = 3;
    }
    else
    {
        printf("Opening flag error.\n");
        f->_error=1;
        free(f);
        return NULL;
    }

	f->_lastOperation=0;
	f->_posFile = 0;
	f->_end = 0;
	f->_posBuffer = 0;
	f->_error = 0;
	f->_nrElem = 0;
	f->_pid = pid;
    f->_f=0;
    f->_fromRead=0;

    for(int i=0; i<SIZE_BUFFER; i++)
        f->_buffer[f->_posBuffer]=0;

	return f;
}

int so_pclose(SO_FILE *stream) 
{

	pid_t pid = stream->_pid;
	if(pid == -1) 
    {
        free(stream);
		return SO_EOF;
	}
	else 
    {
		int check = so_fclose(stream);
		if(check < 0) 
        {
            free(stream);
			return SO_EOF;
		}

		int status;
		check = waitpid(pid, &status, 0);
		if(check < 0) 
        {
            free(stream);
			return check;
		}
        
		return status;
	}
}