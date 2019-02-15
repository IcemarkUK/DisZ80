// main.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include "types.h"
#include "disz80.h"
#include "tinyxml.h"
#include "CLabels.h"

#include <wordexp.h>

void ParseCommand ( int argc, const char* argv[] );
void Init();
void DeInit();
void ReadConfig();

void Output ( void );
int DisplayIncludeFile( CLabel* label, int address, int type );
int DisplayCodeLine( CLabel* label, int address );
int DisplayDataLine ( mem_t address, CLabel* label );
void DisplayCodeOpcodesLine( mem_t address, int nElements );
void DisplayWordLine( mem_t address, int nElements );
void DisplayWordLine( mem_t address, int nElements );
void DisplayByteLine( mem_t address, int nElements, int format );
void DisplayRefLine( mem_t address, int nElements );
void DisplayCharLine( mem_t address, int nElements, int add );
int DisplayLine ( int address ) ;
void textout_lf ( LPCSTR text );
void textout ( LPCSTR format, ... );
void newline();




TIXML_STRING	filebase;
CDisZ80*		disz80=NULL;
FILE*			g_file=NULL;
int				g_line=0;
LPSTR			szComment = NULL ;
LPSTR			szNextComment = NULL ;
CLabel*			lastlabel=NULL;
mem_t			lastaddress=0;

int main(int argc, const char* argv[])
{
    ParseCommand ( argc, argv );
    
    Init();
    
    ReadConfig();
    
    disz80->Analyse();
    
    Output();
    
    DeInit();
    
    return 0;
}

void Init()
{
    disz80 = new CDisZ80 ;
    
}

void DeInit()
{
    SAFEDELETE(disz80);
}


void ReadConfig()
{
    disz80->LoadXml( "default.xml" );

    disz80->LoadXml( filebase.c_str() );
}

void Output ( void )
{
    //g_file = fopen("c:/spectrum/firelord/firelord.s","wt");
    
    g_file = fopen(disz80->doc->m_output.c_str(),"wt");
    if ( g_file ) {
        int address = disz80->labels->GetAt(0)->m_address;
        int size=0;
        for ( ;; ) {
            size = DisplayLine(address);
            if ( size==0 ) break;
            address+=size;
        }
        fclose(g_file);
    }
}

u8 getByte(mem_t a)
{
    return disz80->getByte(a);
}
u16 getWord(mem_t a)
{
    return disz80->getWord(a);
}


void DisplayLabel ( CLabel* label, int address )
{
    LPCSTR output = "" ;
    LPCSTR format = "%s\t\t\t";
    // print label
    if ( label->m_address == address ) {
        // display address label
        if ( !(disz80->GetMemFlags(address)&MEMF_LABELBREAK) ) {
            if ( !(label->m_flags&LF_NOLABEL) ) {
                output = label->Name();
                format = "%-16s\t";
            }
        }
    }
    textout(format,output);
}

int DisplayIncludeFile( CLabel* label, int address, int type )
{
    char linebuffer[1024];
    LPCSTR input = NULL ;
    
    if ( type == 0 ) {
        input = label->m_beforefile.c_str();
    }else{
        input = label->m_afterfile.c_str();
    }
    
    
    if ( strlen(input)==0 )
        return 0;
    
    FILE* file = fopen(input,"rt");
    while ( !feof(file) ) {
        fgets(linebuffer,1000,file);
        textout(linebuffer);
    }
    fclose(file);
    return 0;
}


int DisplayCodeLine( CLabel* label, int address )
{
    instruc_info_t	info;
    
    disz80->DissasembleInstruction( address, line[0], &info );
    
    // display the opcodes
    if ( disz80->printopcodes ) {
        for ( int ii=0; ii<info.size; ii++ ) {
            if ( ii ) textout(" ");
            textout("%02X", getByte(address+ii) );
        }
        if ( info.size<3 ) textout("\t");
        textout("\t");
    }else{
        textout("\t\t");
    }
    
    DisplayLabel(label,address);
    
    // print line
    
    if ( szNextComment )
        textout( "%-32s", line[0] );
    else
        textout( line[0] );
    
    return info.size ;
    
}

int DisplayDataLine ( mem_t address, CLabel* label )
{
    u8* datab;
    
    textout("\t\t");
    DisplayLabel(label,address);
    
    // do some casts
    datab = (u8*) getByte(address);
    labeltype_t type = label->m_data[0].type ;
    int dataperline = label->m_data[0].dataperline ;
    int format = label->m_data[0].format;
    
    mem_t endAddress = disz80->findNextLineStart ( address+1 );
    
    int size = endAddress-address;
    
    // we always enter here at the start of a line
    int nElements = size / datasizes[type] ;
    
    int perline = dataperline ? dataperline : disz80->maxdataperline ;
    if ( type != LABEL_IGNORE && nElements > perline ){
        nElements=perline;
        size=nElements*datasizes[type];
    }
    
    switch ( type ) {
        case LABEL_IGNORE:
            textout( "IGNORE DATA CHUNK %d bytes", size );
            break;
        case LABEL_BYTEDATA:
            DisplayByteLine(address,nElements,format);
            break;
        case LABEL_WORDDATA:
            DisplayWordLine(address,nElements);
            break;
        case LABEL_CHARDATA:
        case LABEL_ULTIMATECHARDATA:
        case LABEL_CHARFOLLOWS_HI:
            DisplayCharLine(address,nElements,0);
            break;
            
        case LABEL_ROMCHARDATA:
            DisplayCharLine(address,nElements,14);
            break;
        case LABEL_REFDATA:
            DisplayRefLine(address,nElements);
            break;
        case LABEL_CODE:
            DisplayCodeOpcodesLine( address, nElements );
            break;
            
    }
    return size;
}

void DisplayCodeOpcodesLine( mem_t address, int nElements )
{
    textout("DB ");
    for ( int ii=0; ii<nElements; ii++ ) {
        textout( "%s ",bHex(getByte(address+ii)));
    }
}

void DisplayWordLine( mem_t address, int nElements )
{
    if  ( nElements == 0 ) {
        DisplayByteLine(address,1,16);
        return;
    }
    
    textout("DW ");
    for ( int ii=0; ii<nElements*2; ii+=2 ) {
        if ( ii ) textout(",");
        textout(wHex(getWord(address+ii)));
    }
}

void DisplayByteLine( mem_t address, int nElements, int format )
{
    textout("DB ");
    for ( int ii=0; ii<nElements; ii++ ) {
        if ( ii ) textout(",");
        if ( format == 16 ) {
            textout(bHex(getByte(address+ii)));
        } else if (format == 10 ) {
            textout(bDec(getByte(address+ii)));
        } else if ( format == 1 ) {
            textout(bBin(getByte(address+ii)));
        }
    }
}

void DisplayRefLine( mem_t address, int nElements )
{
    textout("DW ");
    for ( int ii=0; ii<nElements*2; ii+=2 ) {
        if ( ii ) textout(",");
        textout(disz80->labels->ConvertToLabel(getWord(address+ii),FALSE));
    }
}


void DisplayCharLine( mem_t address, int nElements, int add )
{
    BOOL c = FALSE;
    textout("DB ");
    for ( int ii=0; ii<nElements; ii++ ) {
        
        u8 ch = getByte(address+ii);
        u8 ch2 = ch+add;
        if ( ch2>=32 && ch2<128 ){
            if ( !c ) {
                if ( ii ) textout(",");
                textout("\"");
            }
            textout("%c",ch2);
            c=TRUE;
        }else {
            //textout(".");
            if ( ii ) {
                if ( c ) textout("\"");
                textout(",");
            }
            textout("#%02x",ch);
            c=FALSE;
        }
    }
    if ( c ) textout("\"");
}




int DisplayLine ( int address )
{
    int		size;
    BOOL	needaddress;
    
    if ( address>=0xffff ) return 0;
    
    //	address = findNextLineStart ( address );
    
    CLabel* label = disz80->labels->nearest(address) ;
    
    DisplayIncludeFile( label, address, 0 );
    
    if ( disz80->GetMemFlags(address)&MEMF_CODE && !(disz80->GetMemFlags(address-1)&MEMF_CODE) )
          newline();
    
    if ( disz80->GetMemFlags(address)&MEMF_LINEBREAK ) {
        if ( disz80->GetMemFlags(address)&MEMF_ONLYBREAK ) {
            
            if (disz80->GetMemFlags(address)&MEMF_ROUTINE) {
                
                newline();
                textout("; %04X", address);
                newline();
                //m_forcedbreaks++;
            }
        }
    }
    
    needaddress=TRUE;
    
    if ( label->m_address == address ) {
        
        // display comment
        if ( (label->m_flags&LF_HEADER) && label->Header() ) {
            textout_lf(label->Header());
            newline();
        }
        
        // display address label
        if ( !(disz80->GetMemFlags(address)&MEMF_LABELBREAK) ) {
        } else {
            if ( disz80->GetMemFlags(address)&MEMF_ONLYLABEL ) {
                //textout("%s: ; %s",label->Name(),wHex(address));
                
                if ( disz80->printreladdress  ) {
                    textout("%04x \t",address-lastaddress);
                    //}else{
                    //textout("\t");
                }
                
                textout("%04X \t\t\t%s: ",address,label->Name());
                newline();
            }
        }
    }
    
    // output address ?
    if ( needaddress ) {
        
        if ( disz80->printreladdress ) {
            textout("%04x \t",address-lastaddress);
            //}else{
            //	textout("\t");
        }
        
        
        if ( disz80->printaddress ) {
            textout("%04X \t",address);
        } else
            textout("\t");
    }
    
    
    // prep the end comment
    if ( label->m_address == address ) {
        if ( /*!(label->m_flags&LF_HEADER) && */ label->Comment() ) {
            SAFEFREE ( szComment );
            szComment = strdup(label->Comment());
            szNextComment = strtok(szComment,"|");
        }
    }
    
    
    
    // we now have the start of a line
    // and probably a symbol
    if ( disz80->GetMemFlags(address)&MEMF_CODE ) {
        
        size=DisplayCodeLine( label,address );
        
    }else{
        
        size=DisplayDataLine( address, label );
        
    }
    
    
    if ( szNextComment ) {
        textout(" ; %s",szNextComment);
        szNextComment = strtok(NULL,"|");
    }else{
        SAFEFREE ( szComment );
    }
    
    DisplayIncludeFile( label, address, 1 );
    
    newline();
    
    lastlabel = label ;
    if ( label->m_address == address && label->m_parent == NULL && !(label->m_flags&LF_ANALYSED))
        lastaddress = address;
    //else
    //	lastaddress = address;
    
    return size;
}

void textout_lf ( LPCSTR text )
{
    static char o[1024];
    LPSTR output = o ;
    
    strcpy(o,text) ;
    
    for ( ;; ) {
        LPSTR out = strstr ( output,"\\n" );
        if ( out ) {
            *out = '\0';
            fprintf(g_file,"; ");
            fprintf(g_file,"%s", output);
            output += (out-output)+2;
            fprintf(g_file,"\n");
        }else{
            fprintf(g_file,"; ");
            fprintf(g_file,"%s", output);
            break;
        }
        
    }
    
}

void textout ( LPCSTR format, ... )
{
    static char buffer[256];
    va_list arglist ;
    
    va_start( arglist, format ) ;
    vsprintf( buffer, format, arglist );
    va_end( arglist ) ;
    
    fprintf(g_file,"%s", buffer);
    
}


void newline()
{
    fprintf(g_file,"\n");
    g_line++;
}


void ParseCommand ( int argc, const char* argv[] )
{
    if ( argc < 2 )	{
        printf("USAGE: DISZ80 [input] <OPTIONS>\n");
        exit(1);
    }
    
    /* filename is always option 1 */
    wordexp_t exp_result;
    wordexp(argv[1], &exp_result, 0);
    
    filebase = *exp_result.we_wordv;
    wordfree(&exp_result);
    
    
    
}
