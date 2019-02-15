
#include "disz80.h"
#include "CLabels.h"
#include "portable.hpp"

//#define _CLEAN_
#define _SELFMOD_

extern int testcount;


char	path_buffer[MAX_PATH];
char	drive[MAX_PATH];
char	dir[MAX_PATH];
char	fname[MAX_PATH];
char	ext[MAX_PATH];


CLabel::CLabel ()
{
    CLEARSTRUCT(m_data);
    m_address = 0;
    m_name = NULL;
    m_size = 0;
    m_count=0;
    m_flags = 0;
    m_parent=NULL ;
    m_data[0].dataperline=0;
    m_data[0].format=16;
    m_data[0].type = LABEL_IGNORE ;
}

CLabel::~CLabel ()
{
}

CLabel::CLabel ( mem_t address, LPCSTR szLabel, labeltype_t type )
{
    CLEARSTRUCT(m_data);
    
    m_endaddress = 0;
    m_address = address ;
    m_name = szLabel;
    m_size = 0;
    m_count=0;
    m_flags = 0;
    m_parent=NULL ;
    
    if ( type == LABEL_DATAFOLLOWS_00) {
        type = LABEL_CODE;
        m_flags |= LF_DATAFOLLOWS;
        m_terminator = 0 ;
    }
    if ( type == LABEL_DATAFOLLOWS_FF) {
        type = LABEL_CODE;
        m_flags |= LF_DATAFOLLOWS;
        m_terminator = 0xff ;
    }
    
    
    m_data[0].dataperline=0;
    m_data[0].format=16;
    m_data[0].type = type ;
}



LPSTR CLabel::Name() const
{
    if (!m_name.empty()  )  {
        sprintf(line[4], "%s", m_name.c_str() );
    }else{
        if ( !(m_flags&LF_ANALYSED) ) {
            
            if ( m_data[0].type == LABEL_CODE )
                sprintf(line[4], "L%X", m_address );
            else
                sprintf(line[4], "_%X", m_address );
            
            return line[4];
        }
        
        if ( m_data[0].type == LABEL_CODE ) {
            sprintf(line[4], "l%X", m_address );
            return line[4];
        }
        
        sprintf(line[4], "__%X", m_address );
    }
    return ( line[4] );
}

LPCSTR CLabel::Comment() const
{
    if (m_comment.empty())
        return NULL ;
    return m_comment.c_str() ;
}

LPCSTR CLabel::Header() const
{
    if (m_header.empty())
        return NULL ;
    return m_header.c_str() ;
}

CLabels::CLabels()
{
    //	lpStrSeg=NULL;
    //	lpCurStrSeg=NULL;
    szDefLabelFile=NULL;
    szLabelFile=NULL;
    sortedLabels=0;
    unsortedLabels=0;
    nearestLabel=0;
    labels=NULL;
    labelprinted=FALSE;
    labelsadded=FALSE;
    defaultflags=0;
    
    /* allocate the string segment for the labels */
    //if ( (lpStrSeg = (char*)malloc( MAX_STRING_SEGMENT )) == NULL ) {
    //	printf("Cannot allocate string memory\n");
    //	exit(-1);
    //}
    //lpCurStrSeg = lpStrSeg ;
    //memset ( lpStrSeg,0x00,MAX_STRING_SEGMENT );
    
    if ( (labels = (CLabel**)malloc( MAX_LABELS * sizeof(CLabel*) )) == NULL ) {
        printf("Cannot allocate symbol memory\n");
        exit(-1);
    }
    memset ( labels,0x00,MAX_LABELS * sizeof(CLabel*) );
    
}

CLabels::~CLabels()
{
    
    for ( int ii=0; ii<	sortedLabels+unsortedLabels; ii++ ) {
        SAFEDELETE( labels[ii] );
    }
    
    
    SAFEDELETE ( labels );
    //SAFEDELETE ( lpStrSeg );
    
}




/*
 * void ReadLabels ( void )
 *
 * Reads the default.lbl file and the user label file if found
 *
 */
void CLabels::Read ( void )
{
    sortedLabels=unsortedLabels=1;
    
    /* the system labels won't be written out */
    defaultflags=0;
    
//    ///* try and read a default system labels file */
//    if ( szDefLabelFile == NULL )
//        SAFESTRCPY( path_buffer, "default.xml" );
//    else
//        SAFESTRCPY( path_buffer, szDefLabelFile );
//
//    Read( path_buffer );
//
    /* the user labels will be written out */
    defaultflags|=LF_KEEP;
    
    chilli::lib::splitpath( doc->GetPathName(), drive, dir, fname, ext );
    
    /* try and read a labels file for the SNA file */
    if ( szLabelFile == NULL )
        chilli::lib::makepath( path_buffer, drive, dir, fname, "xml" );
    else
        SAFESTRCPY( path_buffer, szLabelFile );
    
    Read( path_buffer );
    
    /* sort labels */
    sort();
    
}


/*
 * void ReadLabelFile ( LPSTR szFilename )
 *
 * Reads and parses a label file, this is done as pass 0
 * and thus all all labels will be added
 *
 */

token_t tokens_type[] = {
    { "ignore",			LABEL_IGNORE },
    { "byte",			LABEL_BYTEDATA },
    { "word",			LABEL_WORDDATA },
    { "char",			LABEL_CHARDATA },
    { "ref",			LABEL_REFDATA },
    { "code",			LABEL_CODE },
    { "acgchar",		LABEL_ULTIMATECHARDATA },
    { "datafn_00",		LABEL_DATAFOLLOWS_00 },
    { "datafn_ff",		LABEL_DATAFOLLOWS_FF },
    { "romchar",		LABEL_ROMCHARDATA },
    { "char_hi", LABEL_CHARFOLLOWS_HI },
};

token_t tokens_format[] = {
    { "hex",			16 },
    { "h",			    16 },
    { "binary",			1 },
    { "bin",			1 },
    { "b",			    1 },
    { "decimal",		10 },
    { "dec",			10 },
    { "d",			    10 },
} ;

void CLabels::Read ( LPCSTR szFilename )
{
    
    xml* config = new xml();
    
    if ( !config->Load ( szFilename ) ) {
        SAFEDELETE(config);
        return ;
    }
    
    xml::node* base = config->Find("main");
    if ( base == NULL ) {
        SAFEDELETE(config);
        return ;
    }
    
    Read(base);
    
    SAFEDELETE(config);
}



void CLabels::Read ( xml::node* node )
{
    xml::node* base = node->Find("labels");
    ReadLabels(base,NULL);
    sort();
}

void CLabels::ReadLabels ( xml::node* base, CLabel* parent )
{
    LPCSTR temp = NULL ;
    
    if ( base == NULL ) {
        return ;
    }
    
    mem_t baseaddress = parent ? parent->m_address : 0 ;
    
    
    FOREACHELEMENT(base,l) {
        
        labeltype_t		type = LABEL_IGNORE ;
        mem_t			address = baseaddress;
        mem_t			remap = 0 ;
        LPCSTR			name = NULL ;
        CLabel*			label = NULL;
        
        if ( l->IsType("header") )
            continue;
        if ( l->IsType("ref") )
            continue;
        
        
        address = l->ReadInt ( "address", address, baseaddress );
        
        //		if ( address == 0xb18b )
        //			int a =1;
        
        remap = l->ReadInt ( "remap", 0 );
        if ( remap ) {
            mem_t temp = address;
            address = remap ;
            remap = temp;
        }
        
        if ( parent ) {
            
            // search for a base remap
            mem_t remap1 = parent->m_remap;
            CLabel* p=parent;
            while ( remap1 == 0 && p->m_parent ) {
                p = p->m_parent;
                remap1=p->m_remap;
            }
            
            if ( remap1 ) {
                address = p->m_address + (address - remap1);
            }
        }
        
        
#ifndef _CLEAN_
        if ( l->IsType("comment") ) {
            
            
            
            if ( !HasLabel(address) ) {
                type = (labeltype_t) l->ReadToken("type", tokens_type, NUMELE(tokens_type), LABEL_CODE) ;
                
                if ( (name = l->ReadStr("id")) )
                    label = add ( address, name, type, defaultflags );
                else
                    label = add ( address, NULL, type, defaultflags );
                
                label->m_remap = remap ;
                label->m_parent = parent;
                if ( !l->ReadBool("label",FALSE) )
                    label->m_flags |= LF_NOLABEL ;
                
                
            }else{
                label = find( address );
            }
            
            if ( label ) {
                temp = l->ReadStr("value");
                if ( temp )	label->m_comment = temp;
                
                if ( label->Comment() == NULL ) {
                    temp = l->ReadValue("comment");
                    if ( temp )	label->m_comment = temp;
                }
                
                // line comment
                temp = l->ReadStr("pre") ;
                if ( temp )	label->m_beforefile = temp;
                temp = l->ReadStr("post");
                if ( temp )	label->m_afterfile = temp;
            }
        }
#endif
        
        if ( l->IsType("label") || l->IsType("byte") || l->IsType("word") || l->IsType("code") || l->IsType("char") ) {
            
            
            if ( !HasLabel(address) ) {
                
                BOOL quiet = l->ReadBool("quiet",FALSE);
                
                if ( l->IsType("label") ) {
                    type = (labeltype_t) l->ReadToken("type", tokens_type, NUMELE(tokens_type), LABEL_IGNORE) ;
                }else{
                    if ( l->IsType("byte") ) type =LABEL_BYTEDATA ;
                    if ( l->IsType("word") ) type =LABEL_WORDDATA ;
                    if ( l->IsType("char") ) type =LABEL_CHARDATA ;
                    if (l->IsType("char_hi")) type = LABEL_CHARFOLLOWS_HI;
                    if (l->IsType("code")) type = LABEL_CODE;
                }
#ifndef _CLEAN_
                if ( (name = l->ReadStr("id")) )
                    label = add ( address, name, type, defaultflags );
                else
#endif
                    label = add ( address, NULL, type, defaultflags );
                
                if ( !l->ReadBool("label",TRUE) ||quiet )
                    label->m_flags |= LF_NOLABEL ;
                
                label->m_remap = remap ;
                label->m_parent = parent;
                label->m_data[0].dataperline = l->ReadInt("dataperline",0);
                label->m_data[0].format = l->ReadToken("format", tokens_format, NUMELE(tokens_format), 16) ;
                label->m_endaddress = l->ReadInt("end", 0, baseaddress);
                
                
                //
                label->m_beforefile = l->ReadStr("pre");
                label->m_afterfile = l->ReadStr("post");
                
#ifndef _CLEAN_
                label->m_comment = l->ReadStr("comment");
                // big comment
                if ( l->Find("header") ) {
                    label->m_flags |= LF_HEADER ;
                    label->m_header = l->ReadValue("header");
                }
#endif
                
                
                xml::node* r = l;
                for ( int ii=1;ii<10;ii++ ) {
                    if ( label->m_data[ii-1].type == LABEL_REFDATA || label->m_flags&LF_DATAFOLLOWS ) {
                        if ( (r = r->Find("ref")) ) {
                            label->m_data[ii].type = (labeltype_t) r->ReadToken("type", tokens_type, NUMELE(tokens_type), LABEL_IGNORE) ;
                            label->m_data[ii].dataperline = r->ReadInt("dataperline",0);
                            label->m_data[ii].format = r->ReadToken("format", tokens_format, NUMELE(tokens_format), 16);
                            LPCSTR name = r->ReadStr("name", nullptr);
                            if ( name != nullptr ) {
                                label->m_data[ii].name = strdup(name);
                            }
                            
                        }else{
                            break;
                        }
                    }else{
                        break;
                    }
                }
                
                if ( l->ReadBool("break",TRUE) && !quiet )
                    doc->SetMemFlags(address,MEMF_LINEBREAK);
                
                //if ( l->IsType("label") ) {
                ReadLabels(l,label);
                //}
            }
            
        }
    }
    
}


/*
 * int addLabel ( mem_t address, LPSTR szLabel, mem_t type, BOOL keep )
 *
 * adds an address to the label table.
 * on pass 1 & 2 if the nearest previous label is of type LABEL_IGNORE then
 * the label isn't added.
 * check the return value to see where or if the label was added.
 */
CLabel* CLabels::add ( mem_t address, LPCSTR szLabel, labeltype_t type, flags32_t flags )
{
    CLabel* label;

    
    if ( doc->disz80->pass != 0 && nearestLabel ) {
        
        /* if the nearest label is bigger then step back */
        while ( labels[nearestLabel]->m_address >address )
            nearestLabel--;
        
        if ( nearestLabel && labels[nearestLabel]->m_data[0].type == LABEL_IGNORE )
            return NULL;
    }
    
    labelsadded=TRUE;
    
    /* add the label to the string segment */
    //name=NULL;
    //if ( szLabel ) {
    //	SAFESTRCPY(lpCurStrSeg, szLabel );
    //	name=lpCurStrSeg;
    //	lpCurStrSeg+=strlen(lpCurStrSeg)+1;
    //}
    
    label = new CLabel ( address, szLabel, type );
    
    labels[unsortedLabels] = label ;
    label->m_flags |= flags ; //LF_KEEP;
    label->m_data[0].dataperline = 0 ;
    label->m_data[0].format = 16;
    
    // this address must be
    // the start of a line
    // and a label
    if ( !(doc->GetMemFlags(address)&MEMF_START) ) {
        doc->SetMemFlags(address,MEMF_START) ;
        testcount++;
    }
    doc->SetMemFlags(address,MEMF_LABEL) ;
    
    unsortedLabels++;
    
    return label ;
}


/*
 * void qsortLabels ( label_t Item[], int Low, int High)
 *
 * standard quicksort on the label table
 *
 */
void CLabels::qsort ( CLabel* Item[], int Low, int High)
{
    CLabel*	CheckItem;
    CLabel*	temp;
    int		i,j,pivot;
		  
    i = Low;
    j = High;
    pivot = (High + Low) / 2;
    
    CheckItem = Item[pivot];
    
    do	{
        while ( Item[i]->m_address < CheckItem->m_address && i < High) i++;
        while ( CheckItem->m_address < Item[j]->m_address && j > Low)  j--;
        
        if ( i <= j ) {
            temp = Item[i];
            Item[i] = Item[j];
            Item[j] = temp;
            i++;
            j--;
        }
    } while ( i <= j );
    
    if ( Low < j )
        qsort( Item, Low, j);
    
    if ( i < High )
        qsort( Item, i, High);
    
}

/*
 * void SortLabels ( void )
 *
 * sorts the label table if required
 *
 */
void CLabels::sort ( void )
{
    if ( sortedLabels==unsortedLabels )
        return;
    
    qsort( labels, 1, unsortedLabels-1 );
    sortedLabels=unsortedLabels;
    
    for ( int ii=1; ii<sortedLabels-1; ii++ ) {
        if ( labels[ii]->m_endaddress == 0 ) {
            labels[ii]->m_size = labels[ii+1]->m_address - labels[ii]->m_address;
        }else{
            labels[ii]->m_size = labels[ii]->m_endaddress - labels[ii]->m_address;
        }
    }
    
}


/*
 * LPSTR getLabel( int index )
 *
 * returns a LPSTR to the actualy label if it exists, otherwise
 * it creates one in a linebuffer and returns a LPSTR to that.
 * NOTE: this linebuffer is temporary storage
 */
LPSTR CLabels::get( int index )
{
    return labels[index]->Name();
}

/*
 * int findLabel( mem_t address )
 *
 * searches for a given address in the labels table,
 * binary searches the sorted portion followed by a sequential search
 * through the unsorted portion. If a label isn't exactly found then it
 * sets up the interal variable holding the nearest match
 */
CLabel* CLabels::find( mem_t address )
{
    CLabel		key;
    CLabel*		found;
    int			ii;
    
    nearestLabel=0;
    key.m_address=address;
    
    /* search the sorted list */
    int pos = bsearch ( &key, labels, 1, sortedLabels-1 );
    
    found = pos ? labels[pos] : NULL;
    
    if ( found && found->m_address != address )
        nearestLabel=pos;
    
    /* search the unsorted list */
    for (ii=sortedLabels;ii<unsortedLabels;ii++ ) {
        if ( labels[ii]->m_address == address ) {
            found = labels[ii] ;
            break;
        }
    }
    
    return found;
    
}

CLabel* CLabels::findSymbol( LPCSTR text )
{
    for ( int ii=1; ii<unsortedLabels; ii++ ) {
        if ( strcmp( labels[ii]->Name(), text ) == 0 )
            return labels[ii];
    }
    return NULL;
}


LPSTR CLabels::ConvertToLabel ( mem_t address, BOOL ldinstruc )
{
    CLabel*	label;
    mem_t newaddress=0;
    char sign;
    
    if ( address == 0 )
        return wHex(address);
    
    
    label = HasLabel(address) ? find( address ) : NULL ;
    
    // if we are referencing a code address
    // then we are doing self mod, so create a label
    // for the start of the line
    if ( doc->disz80->pass == 1 && label == NULL && ldinstruc && doc->GetMemFlags(address)&MEMF_CODE) {
        for ( newaddress=address-1; newaddress>0x4000; newaddress-- ) {
            if ( doc->GetMemFlags(newaddress)&MEMF_START ) {
                label = HasLabel(newaddress) ? find( newaddress ) : NULL ;
                sprintf(line[3],"_m%04x", newaddress );
                if ( label == NULL ) {
                    add(newaddress,line[3],LABEL_CODE, LF_ANALYSED);
                }else{
                    if (label->m_name.empty() )
                        label->m_name = line[3];
                }
                break;
            }
        }
    }
    
    //
    // data lookup
    //
    
    if ( label == NULL && ldinstruc && !(doc->GetMemFlags(address)&MEMF_CODE)) {
        for (newaddress = address - 1; newaddress>=0x4000; newaddress--) {
            label = HasLabel(newaddress) ? find(newaddress) : NULL;
            if (label != NULL) {
                if (label->m_endaddress == 0)
                    break;
                if (address > label->m_endaddress)
                    break;
                
                int diff = address - newaddress;
                if (diff == 0)
                    return label->Name();
                
                sign = (diff < 0) ? '-' : '+';
                diff = abs(diff);
                if ( diff < 256 )
                    sprintf(line[3], "%s%c#%02x", label->Name(), sign, diff);
                else
                    sprintf(line[3], "%s%c#%04x", label->Name(), sign, diff);
                
                doc->SetMemFlags(address, MEMF_NOLABEL);
                
                return line[3];
            }
        }
    }
    
    
    if ( doc->disz80->pass == 0 )
        return wHex(address);
    
#ifdef _SELFMOD_
    if ( newaddress && ldinstruc && doc->GetMemFlags(address)&MEMF_CODE ) {
        label = find(newaddress);
        int diff = address-newaddress;
        if ( diff==0 )
            return label->Name();
        sign = ( diff<0 ) ? '-' : '+';
        diff = abs(diff);
        sprintf ( line[3], "%s%c%d", label->Name(),sign,diff );
        return line[3];
    }
#endif
    
    
    if ( label && label->m_address == address ) {
        
        //sprintf( line[3], "\1%s\2", label->Name() ) ;
        sprintf( line[3], "%s", label->Name() ) ;
        return line[3];
    }
    
    else {
#if 0
        if ( !ldinstruc || address<0x4000 )
            return wHex(address);
        
        //			if ( !analysenearestdata )
        //				return wHex(address);
        
        //			if ( doc->GetMemFlags(address)&MEMF_CODE )
        //				return wHex(address);
        
        for ( newaddress=address-1; newaddress>0x4000; newaddress-- ) {
            if ( HasLabel(newaddress) ) {
                label = find(newaddress);
                break;
            }
        }
        
        if ( label ) {
            int diff = address-label->m_address;
            
            if ( diff==0 )
                return label->Name();
            
            if ( diff<0 )
                diff=diff;
            
            sign = ( diff<0 ) ? '-' : '+';
            diff = abs(diff);
            
            int dataperline = label->m_data[0].dataperline ;
            if ( dataperline ) {
                int element=diff/dataperline;
                int rest = diff % dataperline;
                //sprintf ( line[3], "\1%s%c(%d*%d+%d)\2", label->Name(),sign,element,dataperline,rest );
                sprintf ( line[3], "%s%c(%d*%d+%d)", label->Name(),sign,element,dataperline,rest );
            }else{
                if ( diff > 255 )
                    //sprintf ( line[3], "\1%s%c#%04x\2", label->Name(),sign,diff );
                    sprintf ( line[3], "%s%c#%04x", label->Name(),sign,diff );
                else
                    //sprintf ( line[3], "\1%s%c#%02x\2", label->Name(),sign,diff );
                    sprintf ( line[3], "%s%c#%02x", label->Name(),sign,diff );
            }
            return line[3];
        }
#endif
        
        return wHex(address);
    }
    
    
}



/*
 * int bsearchLabels ( label_t* lpKey, label_t Item[], int Low, int High )
 *
 * standard binary search for a label address
 * returns the index to the exact match or nearest match
 */
int CLabels::bsearch ( CLabel* lpKey, CLabel* Item[], int Low, int High )
{
    int	mid;
    int num = High;
    unsigned int half;
    
    while (Low <= High)
        if ((half = num / 2))	{
            mid = Low + (num & 1 ? half : (half - 1));
            if ( lpKey->m_address==Item[mid]->m_address)
                return(mid);
            else if (lpKey->m_address<Item[mid]->m_address) {
                High = mid - 1;
                num = num & 1 ? half : half-1;
            }else{
                Low = mid+1;
                num = half;
            }
        }
        else if (num)
            //return (lpKey->address==Item[Low].address ? Low : 0);
            return (Low);
        else
            break;
    
    return NULL ;
}


CLabel* CLabels::nearest ( mem_t address )
{
    for (int ii=sortedLabels-1;ii>=0;ii-- ) {
        if ( labels[ii]->m_address <= address )
            return labels[ii] ;
    }
    return NULL ;
}

BOOL CLabels::HasLabel ( mem_t address ) const
{
    return doc->GetMemFlags(address)&MEMF_LABEL ;
}
