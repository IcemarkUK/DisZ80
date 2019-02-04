#ifndef _CLABELS_H_INCLUDED_
#define _CLABELS_H_INCLUDED_

#include "xml.h"

enum labeltype_t {
    LABEL_IGNORE=0,
    LABEL_BYTEDATA,
    LABEL_WORDDATA,
    LABEL_CHARDATA,
    LABEL_REFDATA,
    LABEL_CODE,
    LABEL_ULTIMATECHARDATA,
    LABEL_DATAFOLLOWS_00,
    LABEL_DATAFOLLOWS_FF,
    LABEL_ROMCHARDATA,
    LABEL_CHARFOLLOWS_HI,
} ;

typedef struct  {
    labeltype_t	type;
    int			dataperline;
    int			format;
} datatype_t ;

enum LABELFLAGS {
    LF_KEEP			= BIT(0),
    LF_HEADER		= BIT(1),
    LF_DATAFOLLOWS	= BIT(2),
    LF_ANALYSED		= BIT(3),
    LF_NOLABEL		= BIT(4),
};


class CLabel
{
public:
    CLabel ();
    ~CLabel ();
    CLabel ( mem_t address, LPCSTR szLabel, labeltype_t type );
    
    LPSTR Name() const;
    LPCSTR Comment() const;
    LPCSTR Header() const;
    
public:
    TIXML_STRING	m_name;
    TIXML_STRING	m_header;
    TIXML_STRING	m_comment;
    TIXML_STRING	m_beforefile;
    TIXML_STRING	m_afterfile;
    mem_t			m_address;
    mem_t			m_endaddress;
    mem_t			m_remap;
    mem_t			m_size;
    int				m_count;
    datatype_t		m_data[10];
    flags32_t		m_flags;
    //u8			m_data[6];
    u8				m_terminator;
    CLabel*			m_parent;
};



class CLabels
{
public:
    
    CLabels();
    ~CLabels();
    
    void Read ( void );
    void Read ( LPCSTR szFilename );
    void Read ( xml::node* node );
    void ReadLabels ( xml::node* base, CLabel* parent );
    
    CLabel* add ( mem_t address, LPCSTR szLabel, labeltype_t type, flags32_t flags );
    CLabel* find( mem_t address );
    CLabel* findSymbol( LPCSTR text );
    void qsort ( CLabel* Item[], int Low, int High);
    void sort ( void );
    LPSTR get( int index );
    LPSTR ConvertToLabel ( mem_t address, BOOL ldinstruc );
    int bsearch ( CLabel* lpKey, CLabel* Item[], int Low, int High );
    
    CLabel* nearest ( mem_t address );
    
    CLabel* GetAt ( int nSubscript ) ;
    CLabel* operator[] ( int nSubscript ) ;
    
    BOOL HasLabel ( mem_t address ) const ;
    
public:
    LPSTR		lpStrSeg;
    LPSTR		lpCurStrSeg;
    LPSTR		szDefLabelFile;
    LPSTR		szLabelFile;
    int			sortedLabels;
    int			unsortedLabels;
    int			nearestLabel;
    BOOL		labelprinted;
    BOOL		labelsadded;
    flags32_t	defaultflags;
    CLabel**	labels;
    
    CSna*		doc;
    
};


inline CLabel* CLabels::GetAt ( int nSubscript )
{ return labels[nSubscript]; }
inline CLabel* CLabels::operator[] ( int nSubscript ) 
{ return GetAt(nSubscript); }

#define	MAX_LABELS			1000000
//65536 /sizeof(CLabel*)
#define MAX_STRING_SEGMENT	1000000 
//65536


#endif //_CLABELS_H_INCLUDED_
