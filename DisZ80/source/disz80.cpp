/* DISZ80.c
 * z80-SNA Dissasembler
 * Author: Chris Wild
 * chris@anam.demon.co.uk
 *
 * The code dissasembler is based on the code from the book;
 * Mastering Machine Code On Your Spectrum
 * by Toni Baker (c) Copyright 1983 Toni Baker
 * Published by Interface Publications
 * ISBN: 0 907563 23 6
 *
 * I aplogise for the probably untidy iplementation, but I was in a rush
 * to implement it.
 *
 */

#include "disz80.h"
#include "CLabels.h"


int testcount=0;

int datasizes[16] = {
    1, //LABEL_IGNORE=0,
    1, //LABEL_BYTEDATA,
    2, //LABEL_WORDDATA,
    1, //LABEL_CHARDATA,
    2, //LABEL_REFDATA,
    1, //LABEL_CODE,
    1, //LABEL_ULTIMATECHARDATA,
    1, //
    1, //
    1, //LABEL_ROMCHARDATA,
    1, //LABEL_CHARFOLLOWS_HI
} ;


/*
 * default.lbl format
 * xxxxx,"",type
 * 0 = ignore, 1 = bytes, 2 = words, 3 = chars, 4 = ref, 5 = code
 *
 */
 
 
/* replacement codes */
// *n = index byte
// *r = relative address (from nextbyte)
// *s = IXH, IYH
// *t = IXL, IYL
// *v = nextbyte
// *w = nextword
// *x = (HL), (IX+*n), (IY+*n)
// *y = HL, IX, IY

LPCSTR r[16]={"B","C","D","E","*s","*t","*x","A"};
LPCSTR s[16]={"BC","DE","*y","SP"};
LPCSTR q[16]={"BC","DE","*y","AF"};
LPCSTR n[16]={"0","1","2","3","4","5","6","7"};
LPCSTR c[16]={"NZ","Z","NC","C","PO","PE","P","M"};
LPCSTR x[16]={"ADD  A,","ADC  A,","SUB  ","SBC   A,","AND  ","XOR  ","OR   ","CP   "};
LPCSTR l1[16]={NULL,"NOP","EX   AF,AF'","DJNZ *r","JR   *r"};
LPCSTR l2[16]={NULL,"(BC),A", "A,(BC)","(DE),A","A,(DE)","(*w),*y","*y,(*w)","(*w),A","A,(*w)"};
LPCSTR l3[16]={NULL,"RLCA","RRCA","RLA","RRA","DAA","CPL","SCF","CCF"};
LPCSTR l4[16]={NULL,"RET","EXX","JP   (*y)","LD    SP,*y"};
LPCSTR l5[16]={NULL,"JP   *w",NULL,"OUT  (*v),A","IN   A,(*v)","EX   (SP),*y","EX   DE,HL","DI","EI"};
LPCSTR l6[16]={NULL,"00 ; START","08 *v ; ERROR-1","10 ; PRINT-A-1","18 ; GET-CHAR","20 ; NEXT-CHAR","28 ; FP-CALC","30 ; BC-SPACES","38 ; MASK-INT"};
LPCSTR l7[16]={NULL,"RLC  ","RRC  ","RL   ","RR   ","SLA  ","SRA  ","SLL  ","SRL  "};
LPCSTR l8[16]={NULL,"IM 0","*","IM 1","IM 2"};
LPCSTR l9[16]={NULL,"LD   I,A","LD   R,A","LD   A,I","LD   A,R","RRD","RLD",NULL,NULL};
LPCSTR l10[16]={NULL,"LD","CP","IN","OUT"};
LPCSTR l11[16]={"ERROR","I","D","IR","DR"};

/* temporary line buffers */
char line[10][MAX_LINELENGTH];


CDisZ80::CDisZ80()
{
    
    pass=0;
    repass=0;
    jrindex=1;
    printaddress=FALSE;
    printreladdress=FALSE;
    printopcodes=FALSE;
    labelbranches=TRUE;
    analyse=FALSE;
    analysebranches=FALSE;
    analysejumps=FALSE;
    analysecalls=FALSE;
    analysedata=TRUE;
    analysewordvar=FALSE;
    analysebytevar=FALSE;
    analysenearestdata=FALSE;
    needlabeltofollow = FALSE ;
    maxdataperline=16;
    dataperline=0;
    maxsizedata=1024*5;
    curaddress=0;
    thisaddress=0;
    lines=0;
    
    labels = new CLabels ;
    
    doc = new CSna ;
    doc->disz80 = this;
    doc->labels = labels;
    
    labels->doc = doc;
    
    
}

CDisZ80::~CDisZ80()
{
    SAFEDELETE(doc);
    SAFEDELETE(labels);
}


void CDisZ80::LoadXml( LPCSTR filename )
{
    xml* config = new xml();
    
    if ( !config->Load ( filename ) ) {
        SAFEDELETE(config);
        return ;
    }
    
    xml::node* base = config->Find("main");
    if ( base == NULL ) {
        SAFEDELETE(config);
        return ;
    }
    
    xml::node* prop = base->Find("properties");
    if ( prop == NULL ) {
        SAFEDELETE(config);
        return ;
    }
    
    
    doc->m_filename = prop->ReadStrProperty("sna",NULL);
    doc->m_output = prop->ReadStrProperty("output","deafult.s");
    
    printaddress = prop->ReadBoolProperty("print_address",printaddress);
    printreladdress = prop->ReadBoolProperty("print_reladdress",printreladdress);
    printopcodes = prop->ReadBoolProperty("print_opcodes",printopcodes);
    labelbranches = prop->ReadBoolProperty("label_branches",labelbranches);;
    
    analysebranches = prop->ReadBoolProperty("analyse_branches",analysebranches);
    analysejumps = prop->ReadBoolProperty("analyse_jumps",analysejumps);
    analysecalls = prop->ReadBoolProperty("analyse_calls",analysecalls);
    analysedata = prop->ReadBoolProperty("analyse_data",analysedata);
    analysewordvar = prop->ReadBoolProperty("analyse_wordvar",analysewordvar);
    analysebytevar = prop->ReadBoolProperty("analyse_bytevar",analysebytevar);
    analysenearestdata = prop->ReadBoolProperty("analyse_nearestdata",analysenearestdata);
    
    needlabeltofollow = prop->ReadBoolProperty("needlabeltofollow",needlabeltofollow);
    maxdataperline = prop->ReadIntProperty("max_data_perline",maxdataperline);
    maxsizedata = prop->ReadIntProperty("max_size_data",maxsizedata);
    
    
    doc->Open(doc->GetPathName());
    labels->Read(base);
    
    SAFEDELETE(config);
}

/*
 * void main ( int argc, LPSTR argv[] )
 *
 *
 *
 */
void CDisZ80::Analyse ( void )
{
    // cache these
    memflags = doc->memoryflags;
    mem = doc->mem;
    labels = doc->labels;
    
    
    /* should we do any analysis? */
    if ( analysebranches || analysejumps || analysecalls || analysedata || analysewordvar || analysebytevar )
        analyse=TRUE;
    
    /* read the label files */
    pass=0;
    
    /* analyse all the chunks */
    if ( analyse ) {
        pass=1;
        repass=0;
        printf("Pass1: ");
        do {
            printf("%d ",repass);
            labels->labelsadded=FALSE;
            AnalyseChunks();
            repass++;
        } while ( labels->labelsadded == TRUE );
        printf("\n");
        AnalyseDataChunkLineSplits();
    }
    
    
    RefreshLineCount();
    
    
    //	DissasembleMemory(labels->GetAt(1)->m_address);
    
}

void CDisZ80::RefreshLineCount( void )
{
    lines=0;
    for ( int ii=labels->GetAt(1)->m_address; ii<0x10000; ii++ ) {
        
        if ( GetMemFlags(ii)&MEMF_LINEBREAK ) {
            SetMemFlags(ii,MEMF_ONLYBREAK);
            lines++;
        }
        
        if ( GetMemFlags(ii)&MEMF_LABELBREAK ) {
            SetMemFlags(ii,MEMF_ONLYLABEL);
            lines++;
        }
        
        if ( GetMemFlags(ii)&MEMF_START )
            lines++;
    }
}

mem_t CDisZ80::GetLineAddress ( int line )
{
    //	if ( line==2 )
    //		int a=100;
    
    mem_t address = labels->GetAt(1)->m_address-1;
    do {
        address++;
        if ( GetMemFlags(address)&MEMF_LINEBREAK )
            line--;
        if ( GetMemFlags(address)&MEMF_LABELBREAK)
            line--;
        if ( GetMemFlags(address)&MEMF_START )
            line--;
    } while ( line>=0 && address<0x10000 ) ;
    
    return address;
}





/*
 *
 * outputs a chunk as code,
 * also used for the analyses pass
 *
 */
void CDisZ80::DissasembleInstruction( mem_t address, LPSTR output, instruc_info_t* info )
{
    int		opcodeclass;
    int		index;
    int		f,g,h,j,k;
    int		opcode;
    u8		nn;
    //int		ii;
    
    curaddress=address;
    
    
    /* check for finished */
    //if ( curaddress >= end || curaddress< 0x4000 )
    //	return 0;
    
    info->calladdress=FALSE;
    info->jumpaddress=FALSE;
    info->jraddress=FALSE;
    info->loadaddress=FALSE;
    info->wordvar=FALSE;
    info->bytevar=FALSE;
    info->size=0;
    
    opcodeclass = 0;
    index=0;
    
    thisaddress=curaddress;
    
    
top:
    
    opcode = nextByte();
    f = (opcode>>6) & 0x03;
    g = (opcode>>3) & 0x07;
    h = opcode&0x07;
    j = (g>>1) & 0x03;
    k = g & 0x01;
    info->wordaddress=0;
    
    
    switch ( opcode ) {
        case 0xcb:
            opcodeclass = 1;
            if ( index ) nn = nextByte();
            goto top;
            
        case 0xed:
            opcodeclass = 2;
            goto top;
            
        case 0xdd:
            index = 1;
            goto top;
            
        case 0xfd:
            index = 2;
            goto top;
    }
    
    
    
    switch ( opcodeclass ) {
            
        case 0 :
            
            if ( opcode == 0x76 ) {
                SAFESTRCPY( output, "HALT" );
                //goto DisplayLine;
                break;
            }
            
            
            switch ( f ) {
                case 0:
                    switch ( h ) {
                        case 0:
                            if ( g > 3 ){
                                sprintf( output, "JR   %s,*r", c[g-4] );
                                info->jraddress=TRUE;
                                
                            } else {
                                SAFESTRCPY(output,l1[g+1]);
                                if (g>=2) info->jraddress=TRUE;
                                if (g == 2) {
                                    int a = 100;
                                }
                            }
                            
                            break;
                        case 1:
                            if ( k == 0 ) {
                                sprintf( output, "LD   %s,*w", s[j] );
                                info->loadaddress=TRUE;
                            } else
                                sprintf( output, "ADD  *y,%s", s[j] );
                            break;
                        case 2:
                            if ( g>=4 && g<=8 ) info->loadaddress=TRUE;
                            sprintf( output, "LD   %s", l2[g+1] );
                            if ( g == 4 || g==5 )
                                info->wordvar=TRUE;
                            if ( g==6 || g == 7 )
                                info->bytevar=TRUE;
                            break;
                        case 3:
                            if ( k == 0 )
                                sprintf( output, "INC  %s", s[j] );
                            else
                                sprintf( output, "DEC  %s", s[j] );
                            break;
                        case 4:
                            sprintf( output, "INC  %s", r[g] );
                            break;
                        case 5:
                            sprintf( output, "DEC  %s", r[g] );
                            break;
                        case 6:
                            sprintf( output, "LD   %s,*v", r[g] );
                            //if ( g==6 && index)
                            //	info->loadaddress=TRUE; // ????
                            break;
                        case 7:
                            SAFESTRCPY( output, l3[g+1] );
                            break;
                    }
                    break;
                case 1:
                    sprintf( output, "LD   %s,%s", r[g],r[h] );
                    break;
                case 2:
                    SAFESTRCPY( output, x[g] );
                    SAFESTRCAT( output, r[h] );
                    break;
                case 3:
                    switch ( h ) {
                        case 0:
                            sprintf( output, "RET  %s", c[g] );
                            break;
                        case 1:
                            if ( k == 0 )
                                sprintf( output, "POP  %s", q[j] );
                            else
                                SAFESTRCPY( output, l4[j+1] );
                            break;
                        case 2:
                            sprintf( output, "JP   %s,*w", c[g] );
                            info->jumpaddress=TRUE;
                            break;
                        case 3:
                            SAFESTRCPY( output, l5[g+1] );
                            if (g==0) info->jumpaddress=TRUE;
                            break;
                        case 4:
                            sprintf( output, "CALL %s,*w", c[g] );
                            info->calladdress=TRUE;
                            break;
                        case 5:
                            if ( k == 0 )
                                sprintf( output, "PUSH %s", q[j] );
                            else {
                                SAFESTRCPY( output, "CALL *w" );
                                info->calladdress=TRUE;
                            }
                            break;
                        case 6:
                            sprintf( output, "%s*v", x[g] );
                            break;
                        case 7:
                            sprintf( output, "RST  %s",l6[g+1] );
                            break;
                    }
                    break;
            }
            break;
            
        case 1:
            
            switch ( f ) {
                case 0:
                    SAFESTRCPY( output, l7[g+1] );
                    SAFESTRCAT( output, r[h] );
                    break;
                case 1:
                    sprintf( output,"BIT  %s,%s", n[g],r[h] );
                    break;
                case 2:
                    sprintf( output,"RES  %s,%s", n[g],r[h] );
                    break;
                case 3:
                    sprintf( output,"SET  %s,%s", n[g],r[h] );
                    break;
            }
            break;
            
        case 2:
            switch ( f ) {
                case 0:
                    SAFESTRCPY( output, "ERROR" );
                    break;
                case 1:
                    switch ( h ) {
                        case 0:
                            sprintf( output, "IN   %s,(C)", r[g] );
                            break;
                        case 1:
                            sprintf( output, "OUT  (C),%s", r[g] );
                            break;
                        case 2:
                            if ( k == 0 )
                                sprintf( output, "SBC  HL,%s", s[j] );
                            else
                                sprintf( output, "ADC  HL,%s", s[j] );
                            break;
                        case 3:
                            if ( k == 0 )
                                sprintf( output, "LD   (*w),%s",s[j] );
                            else
                                sprintf( output, "LD   %s,(*w)",s[j] );
                            info->loadaddress=TRUE;
                            info->wordvar=TRUE;
                            break;
                        case 4:
                            SAFESTRCPY( output, "NEG" );
                            break;
                        case 5:
                            if ( k == 0 )
                                SAFESTRCPY( output, "RETN" );
                            else
                                SAFESTRCPY( output, "RETI" );
                            break;
                        case 6:
                            SAFESTRCPY( output, l8[g+1] );
                            break;
                        case 7:
                            SAFESTRCPY( output, l9[g+1] );
                            break;
                    }
                    break;
                case 2:
                    SAFESTRCPY( output, l10[h+1] );
                    SAFESTRCAT( output, l11[g-3] );
                    break;
                case 3:
                    SAFESTRCPY( output, "ERROR" );
                    break;
            }
            
    }
    
    /* replace codes */
    while ( findCode(output,line[1],"*s") ) {
        if ( index == 0 || h == 6 || h == 4 || h == 5 ) // don't change the H in IXH/IYH if (IX/IY+??)
            sprintf(output, line[1], "H" );
        else if ( index == 1 )
            sprintf(output, line[1], "IXH" );
        else if ( index == 2 )
            sprintf(output, line[1], "IYH" );
    };
    
    while ( findCode(output,line[1],"*t") ) {
        if ( index == 0 || h == 6 || h == 4 || h == 5 ) // don't change the L in IXL/IYL if (IX/IY+??)
            sprintf(output, line[1], "L" );
        else if ( index == 1 )
            sprintf(output, line[1], "IXL" );
        else if ( index == 2 )
            sprintf(output, line[1], "IYL" );
    };
    
    while ( findCode(output,line[1],"*x") ) {
        if ( index == 0 )
            sprintf(output, line[1], "(HL)" );
        else if ( index == 1)
            sprintf(output, line[1], "(IX+*n)" );
        else if ( index == 2)
            sprintf(output,line[1], "(IY+*n)" );
    };
    
    while ( findCode(output,line[1],"*y") ) {
        if ( index == 0 )
            sprintf(output, line[1], "HL" );
        else if ( index == 1)
            sprintf(output, line[1], "IX" );
        else if ( index == 2)
            sprintf(output, line[1], "IY" );
    };
    
    while ( findCode(output,line[1],"*n") ) {
        if ( opcodeclass == 1 )
            sprintf(output, line[1], bHex(nn) );
        else
            sprintf(output, line[1], bHex(nextByte()) );
    };
    
    while ( findCode(output,line[1],"*v") ) {
        sprintf(output, line[1], bHex(nextByte()) );
    };
    
    while ( findCode(output,line[1],"*r") ) {
        info->wordaddress=thisaddress+(s8)(nextByte()+2);
        
        //info.wordaddress
        
        // NOTE: mark these somehow
        sprintf(output, line[1], labels->ConvertToLabel(info->wordaddress,info->loadaddress) );
    };
    
    while ( findCode(output,line[1],"*w") ) {
        info->wordaddress = nextWord();
        sprintf(output, line[1], labels->ConvertToLabel(info->wordaddress,info->loadaddress) );
    };
    
    info->size =  curaddress-thisaddress ;
    
}



/*
 * BOOL findCode( LPSTR string1, LPSTR string2, LPSTR code )
 *
 * copies string2 to string one but replaces the code reference
 * with %s
 */
BOOL CDisZ80::findCode( LPSTR string1, LPSTR string2, LPSTR code )
{
    LPSTR temp;
    
    SAFESTRCPY( string2, string1 );
    temp = strstr( string2, code );
    if ( temp ) {
        temp[0]=(char)'%%';
        temp[1]=(char)'s';
        return(TRUE);
    }
    
    return(FALSE);
}





/*
 * u8 nextByte(void)
 *
 * returns the next byte from the current dissasembly address
 * and increments the dissasembly address by one
 *
 */

u8 CDisZ80::getByte(mem_t address)
{
    return( mem[address] );
}
mem_t CDisZ80::getWord(mem_t address)
{
    u8	h,l;
    l=getByte(address);
    h=getByte(address+1);
    return ( (h<<8)|l );
}

u8 CDisZ80::nextByte(void)
{
    return( mem[curaddress++] );
}

/*
 * mem_t nextWord(void)
 *
 * returns the next word from the current dissasembly address
 * and increments the dissasembly address byt two
 *
 */
mem_t CDisZ80::nextWord(void)
{
    u8	h,l;
    l=nextByte();
    h=nextByte();
    return ( (h<<8)|l );
}


/*
 * void AnalyseChunks ( void )
 *
 *
 *
 */

void CDisZ80::AnalyseChunks ( void )
{
    int	ii;
    
    for ( ii=1; ii<labels->sortedLabels-1; ii++ )	{
        CLabel* label = labels->GetAt(ii);
        
        if (label->m_address == 29186 ) {
            int a = 100;
        }
        
        switch ( label->m_data[0].type ) {
                
            case LABEL_CODE:
                AnalyseCodeChunk( label );
                break;
                
            case LABEL_BYTEDATA:
            case LABEL_WORDDATA:
            case LABEL_CHARDATA:
            case LABEL_CHARFOLLOWS_HI:
            case LABEL_REFDATA:
                AnalyseDataChunk( label, FALSE );
                break;
        }
    }
    
    labels->sort();
}

void CDisZ80::AnalyseDataChunkLineSplits ( void )
{
    int	ii;
    
    for ( ii=1; ii<labels->sortedLabels-1; ii++ )	{
        CLabel* label = labels->GetAt(ii);
        
        //if ( !label->m_name.empty() && label->m_name.length()>MAX_LABEL_SIZE )
        //	SetMemFlags(label->m_address,MEMF_LABELBREAK);
        
        switch ( label->m_data[0].type ) {
            case LABEL_BYTEDATA:
            case LABEL_WORDDATA:
            case LABEL_CHARDATA:
            case LABEL_CHARFOLLOWS_HI:
            case LABEL_REFDATA:
                AnalyseDataChunk( label, TRUE );
                break;
        }
    }
    
}

void CDisZ80::AnalyseDataChunk ( CLabel* label, BOOL flag )
{
    mem_t address = label->m_address ;
    mem_t endaddress = label->m_address+label->m_size ;
    
    int index=0;
    while ( address< endaddress ) {
        
        if ( flag )	{
            if ( !(GetMemFlags(address)&MEMF_START) ) {
                SetMemFlags(address,MEMF_START) ;
                testcount++;
            }
        }
        
        labeltype_t type = label->m_data[0].type ;
        int dataperline = label->m_data[0].dataperline ;
        int elements = dataperline ? dataperline : maxdataperline / datasizes[type] ;
        int size = endaddress-label->m_address;
        int bytesize = elements*datasizes[type];
        
        if ( type == LABEL_REFDATA ) {
            //u16* data = (u16*)&mem[address];
            for ( int ii=0; ii<elements; ii++ ) {
                mem_t element = address+(ii*2) ;
                if ( element >= endaddress )
                    break;
                mem_t refadd = getWord(element);
                if ( refadd >= labels->GetAt(1)->m_address ) {
                    if ( !labels->HasLabel(refadd) ) {
                        
                        char buffer[128];
                        sprintf(buffer,"%s_%02x", label->Name(), index );
                        
                        CLabel* l = labels->add(refadd,buffer,(labeltype_t)label->m_data[1].type, LF_KEEP|LF_ANALYSED );
                        for ( int j=1; j<10; j++ )
                            l->m_data[j-1] = label->m_data[j];
                        
                        SetMemFlags(refadd,MEMF_LINEBREAK);
                        
                        
                        if ( refadd < endaddress ) {
                            endaddress = refadd ;
                            label->m_size = endaddress - label->m_address ;
                        }
                        
                    }
                    
                }
                index++;
            }
        }
        else if ( type == LABEL_CHARFOLLOWS_HI )
        {
            bool addNewLine = false;
            bool searchEndOfLine = false;
            for (int ii = 0; ii<size; ii++) {
                
                if (searchEndOfLine && getByte(address+1) >= 32 ) {
                    searchEndOfLine = false;
                    addNewLine = true;
                }
                
                else if (getByte(address) < 32) {
                    searchEndOfLine = true;
                }
                
                if ( getByte(address) > 127 ) {
                    addNewLine = true;
                }
                
                if (addNewLine && (address+1) < endaddress ) {
                    CLabel* l = labels->add(address + 1, "", type, LF_KEEP | LF_ANALYSED);
                    //SetMemFlags(address + 1, MEMF_LINEBREAK);
                    l->m_size = endaddress - (address + 1);
                    l->m_data[0].dataperline = 128;
                    
                    label->m_size = (address + 1) - label->m_address;
                    label->m_data[0].dataperline = 128;
                    index++;
                    addNewLine = false;
                }
                
                address++;
            }
        }
        
        address+=bytesize;
    }
    
    if (index > 0) {
        //labels->sort();
    }
}

void CDisZ80::AnalyseCodeChunk( CLabel* label )
{
    instruc_info_t info;
    
    mem_t address = label->m_address ;
    mem_t endaddress = label->m_address+label->m_size ;
    
    while ( address < endaddress ) {
        
        DissasembleInstruction( address, line[0], &info );
        
        // mark code bytes
        if ( !(GetMemFlags(address)&MEMF_START) ) {
            SetMemFlags(address,MEMF_START) ;
            testcount++;
        }
        
        
        for ( int ii=0; ii<info.size; ii++ )
            SetMemFlags(address+ii,MEMF_CODE) ;
        
        if ( info.calladdress && info.wordaddress>=0x5b00 ) {
            CLabel* l = labels->find(info.wordaddress);
            if ( l && l->m_flags&LF_DATAFOLLOWS ) {
                address+=info.size;
                if (!labels->HasLabel(address)  ) {
                    CLabel* l2 = labels->add(address,NULL,l->m_data[1].type, LF_ANALYSED|LF_NOLABEL);
                    l2->m_data[0].dataperline = l->m_data[1].dataperline ;
                }
                while ( getByte(address) != l->m_terminator) {
                    doc->SetMemFlags(address++,doc->GetMemFlags(address)&~(MEMF_CODE|MEMF_START)) ;
                }
                doc->SetMemFlags(address++,doc->GetMemFlags(address)&~(MEMF_CODE|MEMF_START)) ;
                
                if (!labels->HasLabel(address) )
                    labels->add(address,NULL,LABEL_CODE, LF_ANALYSED|LF_NOLABEL);
                return;
            }
        }
        
        
        if ( info.calladdress && analysecalls && info.wordaddress>=0x5b00 ) {
            if (!labels->HasLabel(info.wordaddress) ){
                labels->add(info.wordaddress,NULL,LABEL_CODE, LF_KEEP|LF_ANALYSED);
            }
        }
        
        if ( info.jumpaddress && analysejumps && info.wordaddress>=0x5b00 )
            if (!labels->HasLabel(info.wordaddress) )
                labels->add(info.wordaddress,NULL,LABEL_CODE, LF_KEEP|LF_ANALYSED);
        
        if ( info.jraddress && analysebranches && info.wordaddress>=0x5b00 ){
            
            
            if (!labels->HasLabel(info.wordaddress) ) {
                if ( labelbranches ) {
                    sprintf(line[1],"_b%d", jrindex++ );
                    labels->add(info.wordaddress,line[1],LABEL_CODE, LF_ANALYSED);
                }else
                    labels->add(info.wordaddress,NULL,LABEL_CODE, LF_ANALYSED);
            }else{
                CLabel* label = labels->find(info.wordaddress);
                if ( labelbranches ) {
                    if ( label->m_name.empty() )  {
                        sprintf(line[1],"_b%d", jrindex++ );
                        label->m_name = line[1] ;
                        //labels->add(info.wordaddress,line[1],LABEL_CODE, LF_ANALYSED);
                    }
                }
                label->m_flags &= ~LF_NOLABEL ;
            }
        }
        
        
        if (repass > 1) {
            
            if (info.wordvar && analysewordvar && info.wordaddress >= 0x5b00) {
                
                if (doc->GetMemFlags(info.wordaddress)&MEMF_CODE && !(doc->GetMemFlags(info.wordaddress)&MEMF_START)) {
                    doc->SetMemFlags(info.wordaddress, MEMF_NOLABEL);
                }
                
                if (!(doc->GetMemFlags(info.wordaddress)&MEMF_NOLABEL)) {
                    if (!labels->HasLabel(info.wordaddress)) {
                        labels->find(info.wordaddress);
                        labels->add(info.wordaddress, NULL, LABEL_WORDDATA, LF_KEEP | LF_ANALYSED);
                    }
                }
            }
            
            if (info.bytevar && analysebytevar && info.wordaddress >= 0x5b00) {
                
                if (doc->GetMemFlags(info.wordaddress)&MEMF_CODE && !(doc->GetMemFlags(info.wordaddress)&MEMF_START)) {
                    doc->SetMemFlags(info.wordaddress, MEMF_NOLABEL);
                }
                
                if (!(doc->GetMemFlags(info.wordaddress)&MEMF_NOLABEL)) {
                    if (!labels->HasLabel(info.wordaddress)) {
                        labels->find(info.wordaddress);
                        labels->add(info.wordaddress, NULL, LABEL_BYTEDATA, LF_KEEP | LF_ANALYSED);
                    }
                }
            }
        }
        // add a line break after JP, RET, JR
        if ( mem[address] == 0xc9  ||
            mem[address] == 0x18  ||
            mem[address] == 0xc3 ) {
            
            SetMemFlags(address+info.size,MEMF_LINEBREAK|MEMF_ONLYBREAK|MEMF_ROUTINE);
            //if ( GetMemFlags(address+info.size)&MEMF_CODE ) {
            if (!labels->HasLabel(address+info.size) ) {
                labels->add(address+info.size,NULL,LABEL_CODE, LF_KEEP|LF_ANALYSED);
            }
            
            //}else{
            //	if (!labels->HasLabel(address+info.size) )
            //		labels->add(address+info.size,NULL,LABEL_BYTEDATA, LF_KEEP|LF_ANALYSED);
            //}
            
        }
        
        
        address+=info.size;
        
        /*	if ( info.calladdress && labels->HasLabel(info.wordaddress) ) {
         CLabel* label = labels->find( info.wordaddress );
         if ( label->m_flags&LF_DATAFOLLOWS ) {
         int dataperline = label->m_data[0].dataperline;
         BOOL temp = labels->labelsadded;
         label = labels->add(address,NULL,(labeltype_t)label->m_data[0].type, LF_KEEP|LF_ANALYSED);
         while ( getByte(address++) != 0xff );	
         label->m_size = address-label->m_address ;
         label->m_data[0].dataperline = dataperline;
         labels->add(address,NULL,LABEL_CODE, LF_KEEP|LF_ANALYSED);
         AnalyseDataChunk( label, FALSE );
         labels->labelsadded=temp;
         }
         
         }*/
        
    }
}


mem_t CDisZ80::findPreviousLineStart ( mem_t address )
{
    // find the start address of a line
    while ( !(GetMemFlags(address)&MEMF_START) ) {
        address--;
    }
    return address;
}

mem_t CDisZ80::findNextLineStart ( mem_t address )
{
    // find the start address of a line
    while ( !(GetMemFlags(address)&MEMF_START) ){
        address++;
    }
    return address;
}





/*
 * SAFESTRCPY( LPSTR x, LPSTR y )
 *
 * performs a standard strcpy if the second argument isn't NULL
 * otherwise it uses an error string for the strcpy
 *
 */
void SAFESTRCPY( LPSTR x, LPCSTR y )
{
    if ( y==NULL )
        strcpy(x,l11[0]);
    else
        strcpy(x,y);
}

/*
 * SAFESTRCAT( LPSTR x, LPSTR y )
 *
 * performs a standard strcat if the second argument isn't NULL
 * otherwise it uses an error string for the strcat
 *
 */
void SAFESTRCAT( LPSTR x, LPCSTR y )
{
    if ( y==NULL )
        strcat(x,l11[0]);
    else
        strcat(x,y);
}


/*
 * LPSTR wHex( mem_t number )
 *
 * it creates a word hex number in a linebuffer and returns a LPSTR to that.
 * NOTE: this linebuffer is temporary storage
 *
 */
LPSTR wHex( mem_t number )
{
    sprintf( line[3], "#%04X", number );
    return ( line[3]);
}

/*
 * LPSTR bHex( u8 number )
 *
 * it creates a byte hex number in a linebuffer and returns a LPSTR to that.
 * NOTE: this linebuffer is temporary storage
 *
 */
LPSTR bHex( u8 number )
{
    sprintf( line[3], "#%02X", number );
    return ( line[3] );
}

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
(byte & 0x80 ? 1 : 0), \
(byte & 0x40 ? 1 : 0), \
(byte & 0x20 ? 1 : 0), \
(byte & 0x10 ? 1 : 0), \
(byte & 0x08 ? 1 : 0), \
(byte & 0x04 ? 1 : 0), \
(byte & 0x02 ? 1 : 0), \
(byte & 0x01 ? 1 : 0) 

LPSTR bBin( u8 number )
{
    sprintf( line[3], "%d%d%d%d%d%d%d%d", BYTETOBINARY(number) );
    return ( line[3] );
}

LPSTR bDec( u8 number )
{
    sprintf( line[3], "#%d", number );
    return ( line[3] );
}


CSna::CSna ()
{
    mem = NULL ;
    
    if ( (memoryflags = (u16*)malloc ( (MEMORY_SIZE+1)*2 )) == NULL ) {
        printf("Cannot allocate memory map\n");
        exit(-1);
    }
    
    /* allocate an memory image */
    mem =(u8*)malloc(MEMORY_SIZE);
    memset( mem, 0x00, MEMORY_SIZE );
    
    ClearMemoryFlags();
    
    m_defstartaddress=0x5b00;
    m_defendaddress=0x10000;
}

CSna::~CSna ()
{
    Close();
    SAFEFREE(memoryflags);
    SAFEFREE(mem);
}

void CSna::Close ( void )
{
}

u8*	CSna::Open ( LPCSTR szFilename )
{
    sna_t*	pSna;
    
    /* load the snapshot file */
    if ( (pSna = (sna_t*) FILE_Load( szFilename )) == NULL )
        return (NULL);
    
    memset( mem, 0x00, MEMORY_SIZE );
    
    /* stick the SNA file into the image */
    memcpy( &mem[0x4000], pSna->image, SNA_IMAGE_SIZE );
    
    
    reg.i = pSna->i;
    reg.xhl = pSna->xhl;
    reg.xde = pSna->xde;
    reg.xbc = pSna->xbc;
    reg.xaf = pSna->xaf;
    reg.hl = pSna->hl;
    reg.de = pSna->de;
    reg.bc = pSna->bc;
    reg.iy = pSna->iy;
    reg.ix = pSna->ix;
    reg.iff2 = pSna->iff2;
    reg.r = pSna->r;
    reg.af = pSna->af;
    reg.sp = pSna->sp;
    reg.im = pSna->im;
    
    /* dump the SNA file */
    FILE_Unload( pSna );
    
    return ( mem );
}

/*
 * void *FILE_Load( LPSTR szFilename)
 * Allocates memory for the file and reads the entire contents into
 * memory.
 *
 * ENTRY:       LPSTR szFilename - File to get!
 * EXIT:        void *          - pointer to file in memory
 *
 * NOTE:        Remember to free the memory!
 */

void* CSna::FILE_Load( LPCSTR szFilename)
{
    FILE *infile;
    size_t length;
    void *buffer;
    
    if ( (infile = fopen( szFilename, "rb" )) == NULL )
        return NULL;
    
    length = FILE_Length ( infile );
    
    if( (buffer = malloc( length )) == NULL ) {
        fclose( infile );
        return NULL;
    }
    
    if ( (fread( buffer, sizeof(u8), length, infile )) != length )	{
        fclose( infile );
        free( buffer );
        return NULL;
    }
    
    fclose( infile );
    
    return ( buffer );
}

void CSna::FILE_Unload ( void *mem )
{
    free ( mem );
}

/*
 * size_t FILE_Length ( FILE *infile )
 * Returns the length of the file pointed to by the stream.
 *
 * ENTRY:       FILE * infile   - File stream handle
 * EXIT:        size_t          - length of file
 *
 * NOTE:        This function remembers the current file position, and thus
 *              can be called at any point during file access.
 */

size_t CSna::FILE_Length ( FILE *infile )
{
    size_t length;
    s32 curpos;
    
    /* remember current file position */
    curpos = ftell ( infile );
    
    /* then, move to the end of the file */
    fseek ( infile, 0, SEEK_END );
    
    /* giving us its length therefore! */
    length = ftell ( infile );
    
    /* return the file seek position to whence we came */
    fseek ( infile, curpos, SEEK_SET );
    
    return length;
}
