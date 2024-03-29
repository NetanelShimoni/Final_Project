/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001, 2006                                                         *
 *  Contents: main routine                                                  *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "PPMd.h"

#define BACKSLASH '\\'
#define USAGE_STR "Usage: PPMd <e|d> [switches] <FileName[s] | Wildcard[s]>\n"
static const char* pFName;
static DWORD StartFilePosition;
static BOOL EncodeFlag;
static clock_t StartClock;
static struct ARC_INFO { // FileLength & CRC? Hmm, maybe in another times...
    DWORD signature,attrib;
    WORD  info,FNLen,time,date;
} ai;

#if defined(_WIN32_ENVIRONMENT_)
#include <conio.h>

inline void EnvSetNormAttr(const char* FName) { SetFileAttributes(FName,FILE_ATTRIBUTE_NORMAL); }
inline int                         EnvGetCh() { return getch(); }
inline void           EnvGetCWD(char* CurDir) { GetCurrentDirectory(260,CurDir); }
inline void EnvSetDateTimeAttr(const char* WrkStr)
{
    FILETIME ft;
    DosDateTimeToFileTime(ai.date,ai.time,&ft);
    HANDLE hndl=CreateFile(WrkStr,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
    SetFileTime(hndl,&ft,NULL,&ft);         CloseHandle(hndl);
    SetFileAttributes(WrkStr,ai.attrib);
}
struct ENV_FIND_RESULT: public WIN32_FIND_DATA {
    const char*  getFName() const { return cFileName; }
    void copyDateTimeAttr() const {
        ai.attrib=dwFileAttributes;
        FileTimeToDosDateTime(&ftLastWriteTime,&ai.date,&ai.time);
    }
};
struct ENV_FILE_FINDER {
    HANDLE hndl;
    ENV_FIND_RESULT Rslt;
    ENV_FIND_RESULT getResult() { return Rslt; }
    BOOL isFileValid() {
        return ((Rslt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }
    BOOL findFirst(const char* Pattern) {
        return ((hndl=FindFirstFile(Pattern,&Rslt)) != INVALID_HANDLE_VALUE);
    }
    BOOL findNext() { return FindNextFile(hndl,&Rslt); }
    void findStop() { FindClose(hndl); }
};

#elif defined(_DOS32_ENVIRONMENT_)
#include <conio.h>
#include <dos.h>
#include <direct.h>
#if defined(__DJGPP__)
#include <unistd.h>
#include <crt0.h>
#undef BACKSLASH
#define BACKSLASH '/'
char **__crt0_glob_function (char *arg) { return 0; }
void   __crt0_load_environment_file (char *progname) { }
#endif /* defined(__DJGPP__) */

inline void EnvSetNormAttr(const char* FName) { _dos_setfileattr(FName,_A_NORMAL); }
inline int                         EnvGetCh() { return getch(); }
inline void           EnvGetCWD(char* CurDir) { getcwd(CurDir,260); }
inline void EnvSetDateTimeAttr(const char* WrkStr)
{
    FILE* fpOut = fopen(WrkStr,"a+b");
    _dos_setftime(fileno(fpOut),ai.date,ai.time);
    fclose(fpOut);
    _dos_setfileattr(WrkStr,ai.attrib);
}
struct ENV_FIND_RESULT: public find_t {
    const char*  getFName() const { return name; }
    void copyDateTimeAttr() const {
        ai.attrib=attrib;
        ai.time=wr_time;                    ai.date=wr_date;
    }
};
struct ENV_FILE_FINDER {
    ENV_FIND_RESULT Rslt;
    ENV_FIND_RESULT getResult() { return Rslt; }
    BOOL isFileValid() { return TRUE; }
    BOOL findFirst(const char* Pattern) {
        return (_dos_findfirst(Pattern,_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH,&Rslt) == 0);
    }
    BOOL findNext() { return (_dos_findnext(&Rslt) == 0); }
    void findStop() { }
};

#elif defined(_POSIX_ENVIRONMENT_)
#undef BACKSLASH
#define BACKSLASH '/'
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

inline void EnvSetNormAttr(const char* FName) { chmod(FName,S_IWUSR|S_IRUSR); }
inline int                         EnvGetCh() { return getchar(); }
inline void           EnvGetCWD(char* CurDir) { getcwd(CurDir,260); }
inline void EnvSetDateTimeAttr(const char* WrkStr)
{
    struct utimbuf t;
    t.actime=t.modtime=(ai.date << 16)+ai.time;
    utime(WrkStr,&t);                       chmod(WrkStr,ai.attrib);
}
struct ENV_FIND_RESULT {
    dirent de;
    struct stat st;
    const char*  getFName() const { return de.d_name; }
    void copyDateTimeAttr() const {
        ai.attrib=st.st_mode;
        ai.time=st.st_mtime & 0xFFFF;       ai.date=st.st_mtime >> 16;
    }
};
struct ENV_FILE_FINDER {
    const char* pPattern;
    DIR* dir;
    dirent* de;
    struct stat st;
    ENV_FIND_RESULT getResult() {
        ENV_FIND_RESULT Rslt;
        Rslt.de=*de;                        Rslt.st=st;
        return Rslt;
    }
    BOOL isFileValid() {
        return (fnmatch(pPattern,de->d_name,FNM_NOESCAPE) == 0 &&
                stat(de->d_name,&st) == 0 && (st.st_mode & S_IRUSR) != 0 &&
                st.st_nlink == 1);
    }
    BOOL findFirst(const char* Pattern) {
        pPattern=Pattern;
        return ((dir=opendir(".")) && (de=readdir(dir)) != NULL);
    }
    BOOL findNext() { return ((de=readdir(dir)) != NULL); }
    void findStop() { closedir(dir); }
};

#else /* _UNKNOWN_ENVIRONMENT_ */
#pragma message ("unknown environment:")
#pragma message ("    1. _fastcall and _stdcall keywords are disabled")
#pragma message ("    2. wildcards and file attributes are disabled")

#undef  USAGE_STR
#define USAGE_STR "Usage: PPMd <e|d> [switches] FileName[s]\n"
inline void     EnvSetNormAttr(const char* ) {}
inline int                        EnvGetCh() { return getchar(); }
inline void          EnvGetCWD(char* CurDir) { CurDir[0]=0; }
inline void EnvSetDateTimeAttr(const char* ) {}
struct ENV_FIND_RESULT {
    char FName[260];
    const char*  getFName() const { return FName; }
    void copyDateTimeAttr() const {}
};
struct ENV_FILE_FINDER {
    const char* pPattern;
    ENV_FIND_RESULT getResult() {
        ENV_FIND_RESULT Rslt;               strcpy(Rslt.FName,pPattern);
        return Rslt;
    }
    BOOL isFileValid() { return TRUE; }
    BOOL findFirst(const char* Pattern) {
        pPattern=Pattern;                   return TRUE;
    }
    BOOL         findNext() { return FALSE; }
    void findStop() {}
};
#endif /* defined(__WIN32_ENVIRONMENT) */

static const char* const MTxt[] = { "Can`t open file %s\n",
    "read/write error for files %s/%s\n", "Out of memory!\n",
    "User break\n", "unknown command: %s\n", "unknown switch: %s\n",
    "designed and written by Dmitry Shkarin <dmitry.shkarin@mtu-net.ru>\n"
    USAGE_STR
    "Switches (for encoding only):\n"
    "\t-d     - delete file[s] after processing, default: disabled\n"
    "\t-fName - set output file name to Name\n"
    "\t-mN    - use N MB memory - [1,256], default: %d\n"
    "\t-oN    - set model order to N - [2,%d], default: %d\n"
    "\t-rN    - set method of model restoration at memory insufficiency:\n"
    "\t\t-r0 - restart model from scratch (default)\n"
    "\t\t-r1 - cut off model (slow)\n"
};

void _STDCALL PrintInfo(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile)
{
    char WrkStr[320];
    UINT NDec=ftell(DecodedFile);
    NDec += (NDec == 0);
    UINT NEnc=ftell(EncodedFile)-StartFilePosition;
    UINT n1=(8U*NEnc)/NDec;
    UINT n2=(100U*(8U*NEnc-NDec*n1)+NDec/2U)/NDec;
    if (n2 == 100) { n1++;                  n2=0; }
    int RunTime=((clock()-StartClock) << 10)/int(CLOCKS_PER_SEC);
    UINT Speed=NDec/(RunTime+(RunTime == 0));
    UINT UsedMemory=GetUsedMemory() >> 10;
    UINT m1=UsedMemory >> 10;
    UINT m2=(10U*(UsedMemory-(m1 << 10))+(1 << 9)) >> 10;
    if (m2 == 10) { m1++;                   m2=0; }
    if ( !EncodeFlag )                      SWAP(NDec,NEnc);
    sprintf(WrkStr,"%14s:%7d >%7d, %1d.%02d bpb, used:%3d.%1dMB, speed: %d KB/sec",
            pFName,NDec,NEnc,n1,n2,m1,m2,Speed);
    printf("%-79.79s\r",WrkStr);            fflush(stdout);
}
static char* _STDCALL ChangeExtRare(const char* In,char* Out,const char* Ext)
{
    char* RetVal=Out;
    const char* p=strrchr(In,'.');
    if (!p || strrchr(In,BACKSLASH) > p)    p=In+strlen(In);
    do { *Out++ = *In++; } while (In != p);
    *Out++='.';
    while((*Out++ = *Ext++) != 0)           ;
    return RetVal;
}
inline BOOL RemoveFile(const char* FName)
{
    EnvSetNormAttr(FName);                  return (remove(FName) == 0);
}
static BOOL _STDCALL TestAccessRare(const char* FName)
{
static BOOL YesToAll=TRUE;
    FILE* fp=fopen(FName,"rb");
    if ( !fp )                              return TRUE;
    fclose(fp);
    if ( YesToAll )                         return RemoveFile(FName);
    printf("%s already exists, overwrite?: <Y>es, <N>o, <A>ll, <Q>uit?",FName);
    for ( ; ; )
        switch ( toupper(EnvGetCh()) ) {
            case 'A':                       YesToAll=TRUE;
            case '\r': case 'Y':            return RemoveFile(FName);
            case 0x1B: case 'Q':            printf(MTxt[3]); exit(-1);
            case 'N':                       return FALSE;
        }
}
static FILE* FOpen(const char* FName,const char* mode)
{
    FILE* fp=fopen(FName,mode);
    if ( !fp ) { printf(MTxt[0],FName);     exit(-1); }
    setvbuf(fp,NULL,_IOFBF,64*1024);        return fp;
}
inline void PrepareCoding(int SASize,FILE* fp)
{
    if ( !StartSubAllocator(SASize) ) {
        printf(MTxt[2]);                    exit(-1);
    }
    StartClock=clock();                     StartFilePosition=ftell(fp);
}
inline void EncodeFile(const ENV_FIND_RESULT& efr,int MaxOrder,int SASize,BOOL CutOff,const char* ArcName)
{
    char WrkStr[260];
    strcpy(WrkStr,ArcName);
    if (!WrkStr[0] && !TestAccessRare(ChangeExtRare(efr.getFName(),WrkStr,"pmd")))
                return;
    FILE* fpIn = FOpen(efr.getFName(),"rb"), * fpOut = FOpen(WrkStr,"a+b");
    pFName=strrchr(efr.getFName(),BACKSLASH);
    pFName=( pFName )?(pFName+1):(efr.getFName());
    efr.copyDateTimeAttr();
    ai.signature=PPMdSignature;             ai.FNLen=strlen(pFName)+(CutOff << 14);
    ai.info=(MaxOrder-1) | ((SASize-1) << 4) | ((PROG_VAR-'A') << 12);
    fwrite(&ai,sizeof(ai),1,fpOut);         fwrite(pFName,ai.FNLen & 0x1FF,1,fpOut);
    PrepareCoding(SASize,fpOut);            EncodeFile(fpOut,fpIn,MaxOrder,CutOff);
    putchar('\n');
    if (ferror(fpOut) || ferror(fpIn)) {
        printf(MTxt[1],efr.getFName(),WrkStr);
        exit(-1);
    }
    fclose(fpIn);                           fclose(fpOut);
}
inline BOOL DecodeOneFile(FILE* fpIn)
{
    char WrkStr[260];
    int MaxOrder, SASize;
    BOOL CutOff;
    if ( !fread(&ai,sizeof(ai),1,fpIn) )    return FALSE;
    CutOff=ai.FNLen >> 14;
    ai.FNLen=CLAMP(int(ai.FNLen & 0x1FF),1,260-1);
    fread(WrkStr,ai.FNLen,1,fpIn);          WrkStr[ai.FNLen]=0;
    if ( !TestAccessRare(WrkStr) )          return FALSE;
    FILE* fpOut = FOpen(pFName=WrkStr,"wb");
    MaxOrder=(ai.info & 0x0F)+1;            SASize=((ai.info >> 4) & 0xFF)+1;
    DWORD Variant=(ai.info >> 12)+'A';
    if (ai.signature != PPMdSignature || Variant != PROG_VAR) {
        printf(MTxt[0],WrkStr);             exit(-1);
    }
    PrepareCoding(SASize,fpIn);             DecodeFile(fpOut,fpIn,MaxOrder,CutOff);
    putchar('\n');
    if (ferror(fpOut) || ferror(fpIn) || feof(fpIn)) {
        printf(MTxt[1],WrkStr,WrkStr);      exit(-1);
    }
    fclose(fpOut);                          EnvSetDateTimeAttr(WrkStr);
    return TRUE;
}
inline void DecodeFile(const ENV_FIND_RESULT& efr)
{
    FILE* fpIn=FOpen(efr.getFName(),"rb");
    while ( DecodeOneFile(fpIn) )           ;
    fclose(fpIn);
}
inline void TestArchive(char* ArcName,const char* Pattern)
{
    if ( !Pattern[0] ) {
        char CurDir[260];
        EnvGetCWD(CurDir);
        const char* p=strrchr(CurDir,BACKSLASH);
        p = (p && strlen(p+1))?(p+1):("PPMdFile");
        ChangeExtRare(p,ArcName,"pmd");
    } else                                  strcpy(ArcName,Pattern);
    FILE* fp = fopen(ArcName,"rb");
    if ( fp ) {
        if (!fread(&ai,sizeof(ai),1,fp) || ai.signature != PPMdSignature ||
                                            (ai.info >> 12)+'A' != PROG_VAR) {
            printf(MTxt[0],ArcName);        exit(-1);
        }
        fclose(fp);
    }
}
struct FILE_LIST_NODE {
    FILE_LIST_NODE* next;
    ENV_FIND_RESULT efr;
    FILE_LIST_NODE(const ENV_FIND_RESULT& Data,FILE_LIST_NODE** PrevNode) {
        efr=Data;                           next=*PrevNode;
        *PrevNode=this;
    }
    void destroy(FILE_LIST_NODE** PrevNode) {
        *PrevNode=next;                     delete this;
    }
};
using namespace std;
#include <iostream>
int main(int argc, char *argv[])
{
    assert(TestCompilation());
    char ArcName[260];
    BOOL DeleteFile=FALSE, CutOff=FALSE;
    int i, MaxOrder=4, SASize=10;
    std::cout<<">"<<argv[1][0]<<endl;

    printf("Fast PPMII compressor for textual data, variant %c, "__DATE__"\n",PROG_VAR);
    if (argc < 3) { printf(MTxt[6],SASize,MAX_O,MaxOrder);      return -1; }
    switch ( toupper(argv[1][0]) ) {
        case 'E': EncodeFlag=TRUE;                              break;
        case 'D': EncodeFlag=FALSE;                             break;
        default : printf(MTxt[4],argv[1]);                      return -1;
    }
    for (ArcName[0]=0,i=2;i < argc && (argv[i][0] == '-' || argv[i][0] == '/');i++)
        switch ( toupper(argv[i][1]) ) {
            case 'D': DeleteFile=TRUE;                          break;
            case 'F': TestArchive(ArcName,argv[i]+2);           break;
            case 'M': SASize=CLAMP(atoi(argv[i]+2),1,256);      break;
            case 'O': MaxOrder=CLAMP(atoi(argv[i]+2),2,int(MAX_O));
                        break;
            case 'R': CutOff=CLAMP(atoi(argv[i]+2),0,1);        break;
            default : printf(MTxt[5],argv[i]);   				return -1;
        }
    FILE_LIST_NODE* pNode, * pFirstNode=NULL, ** ppNode=&pFirstNode;
    for (ENV_FILE_FINDER eff;i < argc;i++) {
        if ( eff.findFirst(argv[i]) )
            do {
                if ( eff.isFileValid() ) {
                    pNode = new FILE_LIST_NODE(eff.getResult(),ppNode);
                    if ( !pNode ) {
                        printf(MTxt[2]);    return -1;
                    }
                    ppNode=&(pNode->next);
                }
            } while ( eff.findNext() );
        eff.findStop();
    }
    while ((pNode=pFirstNode) != NULL) {
        ENV_FIND_RESULT& efr=pNode->efr;
        if ( EncodeFlag )                   EncodeFile(efr,MaxOrder,SASize,CutOff,ArcName);
        else                                DecodeFile(efr);
        if ( DeleteFile )                   remove(efr.getFName());
        pNode->destroy(&pFirstNode);        
    }
    // StopSubAllocator();
    return 0;
}
