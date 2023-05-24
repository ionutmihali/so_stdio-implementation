Conceptele si modul de implementare ale functionalitatilor sunt asemanatoare in cele doua sisteme de operare(Linux/Windows), diferenta s face la modul de utilizare a apelurilor de sistem.


STRUCTURA FOLOSITA PENTRU DEFINIREA TIPULUI DE DATE SO_FILE
typedef struct _so_file
{
    int/HANDLE _fd; -File descriptor/Handle
    int/DWORD _posFile; -Pozitia in fisier
    int _posBuffer; -Pozitia in bufffer
    int _openFlag; -Permisiunea de deschidere: 1-read, 2-read+, 3-write, 4-write+, 5-append, 6-append+
    char _buffer[SIZE_BUFFER]; -continutul fisierului/care trebuie scris in fisier
    int _lastOperation; -ultima operatie efectuata: 0-nicio operatie, 1-write, 2-read
    int _error; -Flag pentru erori: 0-nicio eroare, 1-am intalnit eroare
    int _end; -Flag pentru sfasitul fisierului: 0-nu am ajuns la final, 1-am ajuns la final
    int/DWORD _nrElem; -Elemente citite/scrise
    int _f; -Flag pentru efectuarea operatiuni de fflush: 0-nu am efectuat, 1-am efectuat
    int _fromRead; -modul de apel al functiei so_fgetc(): 0-din main, 1-din functia so_read()
    pid_t/PROCESS_INFORMATION _pid; -pid-ul procesului copil
}SO_FILE;


BUFFERING
Pentru implementarea acestei functionalitati am introdus in structura membrii _lastOperation, dupa o operatie de scriere ar trebui golit buffer-ul si scris continutul acestuia in fisier.
WRITE: sirul este citit caracter cu caracter prin intermediul functiei so_fputc(), memorat in buffer-ul structurii. Cand buffer-ul este plin, s-a ajuns la BUFFER_SIZE(4096) sau operatia de scriere s-a incheiat, continutul este transcris in fisier.
READ: fisierul este citit caracter cu caracter cu ajutorul functiei so_getc() si adus in buffer. Cand s-a ajuns la dimensiune maxima a bufferului, acesta este golit la adresa de memorie si reluat procesul.

ALTE FUNCTII
Functiile testReading si testWriting verifica permisiunile cu care a fost deschis fisierul, returnand 1 in cazul in care se poate efectua operatiunea si 0, in caz contrar.
