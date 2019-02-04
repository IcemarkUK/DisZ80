#include "types.h"
#include "xml.h"

token_t token_Bool[] = {
	{ "yes",	2 },
	{ "true",	2 },
	{ "no",		1 },
	{ "false",	1 },
	};

		int GetToken ( LPCSTR token, token_t array[], int max )
		{
			if ( token ) {
				for ( int ii=0; ii<max; ii++ ) {
					if ( strcasecmp( array[ii].token, token ) == 0 )
						return array[ii].value;
				}
			}
			return 0 ;
		}


xml::xml()
{
}

xml::~xml()
{
}

BOOL xml::Load ( LPCSTR filename )
{
//u8* data = NULL ;
//u32 size=0;
//
//	if ( pak ) {
//		data = (u8*)pak->Load ( filename, &size );
//	}else{
//		data = (u8*)os::filemanager::Load ( filename, &size );
//	}
//	if ( data == NULL )
//		return FALSE;
//
//	m_doc = new TiXmlDocument();
	return LoadFile(filename);
//	Parse((const char*)data);
//
//	SAFEDELETE ( data );
	return FALSE ;
}

xml::node* xml::Find ( LPCSTR section )
{
	if ( this == nullptr )
		return NULL;
	return (xml::node*)FirstChild(section)->ToElement();
}

xml::node* xml::node::Find ( LPCSTR section )
{
	if ( this == nullptr )
		return NULL;
	return (xml::node*)FirstChild(section)->ToElement();
}

BOOL xml::node::IsType(LPCSTR name) const
{
	return strcasecmp(Value(),name) == 0;
}

int xml::node::Count ()
{
	if ( this == nullptr )
		return 0;

	int count=0;
	FOREACHELEMENT(this,e) {
		count++;
	}
	return count;
}

xml::node* xml::node::Find( LPCSTR element, LPCSTR id )
{
	FOREACHELEMENT(this,e){
		if ( e->IsType(element) ) {
			if ( strcasecmp(e->ReadStr("id"), id) == 0 ) {
				return e;
			}
		}
	}
	return NULL ;
}

LPCSTR xml::node::ReadElement( LPCSTR element, LPCSTR id, LPCSTR tag )
{
	FOREACHELEMENT(this,e){
		if ( e->IsType(element) ) {
			if ( strcasecmp(e->ReadStr("id"), id) == 0 ) {
				return e->ReadStr(tag);
			}
		}
	}
	return NULL ;
}

LPCSTR xml::node::ReadItem ( LPCSTR name, LPCSTR defaultvalue )
{
	if ( this == NULL )
		return defaultvalue;
	LPCSTR value = this->Attribute(name);
	if ( value == NULL )
		return defaultvalue;
	return value;
}

f32 xml::node::ReadItem ( LPCSTR name, f32 defaultvalue )
{
	if ( this == NULL )
		return defaultvalue;
	LPCSTR value = this->Attribute(name);
	if ( value == NULL )
		return defaultvalue;
	return atof ( value );
}

int xml::node::ReadItem ( LPCSTR name, int base, int defaultvalue )
{
int number=0;
int v=0;
int rel=0;

	if ( this == nullptr )
		return defaultvalue;
    
	LPCSTR value = this->Attribute(name);
	if ( value == NULL )
		return defaultvalue;

	if ( value[0] == '+' )  {
		rel=1;
		number=base;
		value++;
	}
	if ( value[0] == '+' ) {
		rel=2;
		number=base;
		value++;
	}

	if ( value[0] == '0' && (value[1] == 'x' || value[1] == 'X' )) {
		sscanf(&value[2],"%x",&v);
		//return number+v;
	}else{
		v = atoi ( value );
	}

	if ( rel == 1 ) 
		return number + v ;
	else if ( rel == 1 ) 
		return number - v ;
	return v;
}


BOOL xml::node::Exists( LPCSTR name )
{
	if ( this == nullptr )
		return FALSE;

	if ( this->Attribute(name) == NULL )
		return FALSE;

	return TRUE;
}

//int xml::node::ReadValue( LPCSTR name, int defaultvalue )
//{
//	xml::node* newelement = (xml::node*)this->FirstChildElement(name);
//	return newelement->ReadItem("value",defaultvalue);
//}


LPCSTR  xml::node::ReadStr( LPCSTR name, LPCSTR defaultvalue )
{
	return ReadItem ( name, defaultvalue );
}

/*
int xml::node::ReadColour( LPCSTR name, int defaultvalue )
{
int value=0;

LPCSTR	val = ReadStr(name);
	if ( val == NULL )
		return defaultvalue;

	if ( val[0] == '#' ) {
		sscanf(&val[1],"%x",&value);
		return value;
	}

	// find a global colour variable
	//if ( val[0] == '@' ) {
	//	var_t* var = FindGlobalVariable ( &val[1] );
	//	if ( var ) {
	//		if ( var->type == V_RGB ) {
	//			CRgb* rgb = (CRgb*)var->ptr ;
	//			return rgb->rgba;
	//		}
	//		if ( var->type == V_INT ) {
	//			int* integer = (int*)var->ptr ;
	//			return *integer;
	//		}
	//	}
	//	return 0;
	//}

	return atoi(val) ;
}
*/

int xml::node::ReadToken( LPCSTR token, token_t array[], int max, int defaultvalue )
{
	if ( !Exists(token) )
		return defaultvalue;

	LPCSTR 	val = ReadStr(token,"");
	int temp =  GetToken ( val, array, max );
	if ( temp ) return temp ;
	return atoi(val);
}

f32 xml::node::ReadFloat ( LPCSTR name, f32 defaultvalue )
{
	return ReadItem(name,defaultvalue) ;
}

int xml::node::ReadInt( LPCSTR name, int defaultvalue, int basevalue )
{
	return ReadItem(name,defaultvalue,basevalue) ;
}

BOOL xml::node::ReadBool( LPCSTR name, BOOL defaultvalue )
{
	return (BOOL)(ReadToken( name, token_Bool, NUMELE(token_Bool), defaultvalue+1 )-1);
}

/*
int ConvertArray ( LPSTR value, collections::base& c, LPCSTR delim )
{
	if ( value == NULL ) return 0;
	LPCSTR token = strtok( value, delim );
	while( token != NULL )   {
		c.Add( atol(token) ) ;
		token = strtok( NULL, delim );
	}
	return c.Count();
}

int ConvertArray ( LPSTR value, collections::c_float& c, LPCSTR delim )
{
	if ( value == NULL ) return 0;
	LPCSTR token = strtok( value, delim );
	while( token != NULL )   {
		c.Add( atof(token) ) ;
		token = strtok( NULL, delim );
	}
	return c.Count();
}

int xml::node::ReadArray ( LPCSTR name, collections::base& c, LPCSTR delim )
{
	string value = ReadStr( name );
	if ( value.IsNull() )
		return 0;
	return ConvertArray(value,c,delim);

}

int xml::node::ReadArray ( LPCSTR  name, collections::base& c )
{
	return ReadArray(name,c,",");
}
*/

LPCSTR xml::node::ReadValue ( LPCSTR name )
{
	xml::node* m = Find(name);
	if ( m == NULL && IsType(name))
		m=this;
	if ( m ) {
		TiXmlText* m1 = m->FirstChild()->ToText();
		if ( m1 )
			return m1->Value();
	}
	return NULL ;

}
/*
int xml::node::ReadValueArray ( LPCSTR  name, collections::base& c )
{
	string s = ReadValue(name);
	return ConvertArray(s,c,",");
}

int xml::node::ReadValueArray ( LPCSTR  name, collections::c_float& c )
{
	string s = ReadValue(name);
	return ConvertArray(s,c,",");
}
*/
int xml::node::ReadIntProperty ( LPCSTR  name, int defaultvalue )
{
	xml::node* e = NULL ;
	if ( (e = Find( "int", name )) ) {
		if ( e->Exists("value") )
			return e->ReadInt("value",defaultvalue);
		TiXmlText* m1 = e->FirstChild()->ToText();
		if ( m1 ) return atol(m1->Value());
	}
	return defaultvalue ;
}

BOOL xml::node::ReadBoolProperty ( LPCSTR  name, BOOL defaultvalue )
{
	xml::node* e = NULL ;
	if ( (e = Find( "bool", name )) ) {
		if ( e->Exists("value") )
			return e->ReadBool("value",defaultvalue);

		TiXmlText* m1 = e->FirstChild()->ToText();
		if ( m1 ) {
			int temp =  GetToken ( m1->Value(), token_Bool, NUMELE(token_Bool) );
			if ( temp ) return temp-1 ;
		}

	}
	return defaultvalue ;
}

LPCSTR xml::node::ReadStrProperty ( LPCSTR  name, LPCSTR defaultvalue )
{
	xml::node* e = NULL;
	if ( (e = Find( "string", name )) ) {
		if ( e->Exists("value") )
			return e->ReadStr("value",defaultvalue);
		TiXmlText* m1 = e->FirstChild()->ToText();
		if ( m1 ) return m1->Value();
	}
	return defaultvalue ;
}

f32 xml::node::ReadFloatProperty ( LPCSTR  name, f32 defaultvalue )
{
	xml::node* e = NULL;
	if ( (e = Find( "float", name )) ) {
		if ( e->Exists("value") )
			return e->ReadFloat("value",defaultvalue);
		TiXmlText* m1 = e->FirstChild()->ToText();
		if ( m1 ) return atof(m1->Value());
	}
	return defaultvalue ;
}

