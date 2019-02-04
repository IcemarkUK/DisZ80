#ifndef _DISZ80_H_INCLUDED_
#define _DISZ80_H_INCLUDED_

#include "types.h"
#include "xml.h"

class CLabel;
class CLabels;

#define _VERSION_ "1.00"


#define MAX_LINELENGTH		128
#define MAX_TOKENS			16
#define	MAX_LABEL_SIZE		14

#define MEMF_CODE			0x01
#define MEMF_START			0x02
#define MEMF_LABEL			0x04
#define MEMF_LINEBREAK		0x08
#define MEMF_LABELBREAK		0x10
#define MEMF_ROUTINE		0x20
#define MEMF_ONLYLABEL		0x40
#define MEMF_ONLYBREAK		0x80
#define MEMF_NOLABEL		0x100


typedef u32 mem_t ;

typedef struct {
    mem_t	wordaddress;
    BOOL	calladdress;
    BOOL	jumpaddress;
    BOOL	loadaddress;
    BOOL	jraddress;
    BOOL	bytevar;
    BOOL	wordvar;
    int		size;
} instruc_info_t ;


class CLabels;
class CDisZ80;


#define SNA_IMAGE_SIZE		49152
#define MEMORY_SIZE			65536*2

#define LONIBBLE(x)			(x&0x0f)
#define HINIBBLE(x)			((x&0xf0)>>4)

#pragma pack ( push, SNAPSHOTS_PACK, 1 )

typedef struct {
    u8	i;
    u16	xhl;
    u16	xde;
    u16	xbc;
    u16	xaf;
    u16	hl;
    u16	de;
    u16	bc;
    u16	iy;
    u16	ix;
    u8	iff2;
    u8	r;
    u16	af;
    u16	sp;
    u8	im;
    u8	border;
    u8	image[];
    
} sna_t;

typedef struct {
    u16	hl;
    u16	de;
    u16	bc;
    u16	af;
    u16	xhl;
    u16	xde;
    u16	xbc;
    u16	xaf;
    u16	ix;
    u16	iy;
    u16	sp;
    
    u8	i;
    u8	r;
    u8	im;
    u8	iff2;
} registers_t ;

//                               // Bits in Z80 F register:
#define S_FLAG      0x80       // 1: Result negative
#define Z_FLAG      0x40       // 1: Result is zero
#define H_FLAG      0x10       // 1: Halfcarry/Halfborrow
#define P_FLAG      0x04       // 1: Result is even
#define V_FLAG      0x04       // 1: Overflow occured
#define N_FLAG      0x02       // 1: Subtraction occured
#define C_FLAG      0x01       // 1: Carry/Borrow occured
//s z h p v n c

#pragma pack ( pop, SNAPSHOTS_PACK, 1 )


class CSna
{
public:
    CSna();
    virtual ~CSna();
    
    u8*	Open ( LPCSTR szFilename );
    void Close();
    
    LPCSTR GetPathName() const					{ return m_filename.c_str(); }
    u16 GetMemFlags(mem_t address) const			{ return memoryflags[address]; }
    void SetMemFlags(mem_t address, u16 flags)	{ memoryflags[address] |= flags ; }
    void ClearMemoryFlags ( void )				{ memset ( memoryflags, 0x00, (MEMORY_SIZE*2)+1 ); }
    
private:
    void* FILE_Load( LPCSTR szFilename);
    void FILE_Unload ( void *mem );
    size_t FILE_Length ( FILE *infile );
    
public:
    TIXML_STRING	m_filename;
    TIXML_STRING	m_output;
    registers_t		reg;
    u8*				mem;
    u16*			memoryflags;
    CLabels*		labels ;
    CDisZ80*		disz80 ;
    mem_t			m_defstartaddress;
    mem_t			m_defendaddress;
    
};



class CDisZ80
{
public:
    CDisZ80();
    ~CDisZ80();
    void Analyse ( void );
    
    void SetMemFlags(mem_t address, u16 flags);
    u16 GetMemFlags(mem_t address) const ;
    
    
    mem_t findPreviousLineStart ( mem_t address );
    mem_t findNextLineStart ( mem_t address );
    void AnalyseDataChunkLineSplits ( void );
    u8 nextByte(void);
    mem_t nextWord(void);
    BOOL findCode( LPSTR string1, LPSTR string2, LPSTR code );
    void AnalyseChunks ( void );
    void AnalyseCodeChunk( CLabel* label );
    void AnalyseRefChunk ( CLabel* label );
    void AnalyseDataChunk ( CLabel* label, BOOL flag );
    void DissasembleInstruction( mem_t address, LPSTR output, instruc_info_t* info );
    
    void RefreshLineCount( void );
    mem_t GetLineAddress ( int line );
    
    u8 getByte(mem_t address);
    mem_t getWord(mem_t address);
    
    void LoadXml( LPCSTR filename );
    
    
public:
    int			pass;
    int			repass;
    int			jrindex;
    BOOL		printaddress;
    BOOL		printreladdress;
    BOOL		printopcodes;
    BOOL		labelbranches;
    BOOL		analyse;
    BOOL		analysebranches;
    BOOL		analysejumps;
    BOOL		analysecalls;
    BOOL		analysedata;
    BOOL		analysewordvar;
    BOOL		analysebytevar;
    BOOL		analysenearestdata;
    BOOL		needlabeltofollow;
    int			maxdataperline;
    int			dataperline;
    int			maxsizedata;
    
    mem_t		curaddress;
    mem_t		thisaddress;
    
    int			lines;
    
    
    u8*			mem;
    u16*		memflags;
    CLabels*	labels;
    CSna*		doc;
};

inline u16 CDisZ80::GetMemFlags(mem_t address) const
{ return memflags[address]; }

inline void CDisZ80::SetMemFlags(mem_t address, u16 flags)
{ memflags[address] |= flags ; }


void SAFESTRCAT( LPSTR x, LPCSTR y );
void SAFESTRCPY( LPSTR x, LPCSTR y );
LPSTR wHex( mem_t number );
LPSTR bHex( u8 number );
LPSTR bBin( u8 number );
LPSTR bDec( u8 number );


/* temporary line buffers */
extern	char	line[10][MAX_LINELENGTH];
extern	int		datasizes[16];








#endif
