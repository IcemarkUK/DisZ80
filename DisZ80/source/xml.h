#ifndef _XML_H_INCLUDED_
#define _XML_H_INCLUDED_

#include "../tinyxml/tinyxml.h"

		class xml : public TiXmlDocument
		{
		public:

			xml();
			~xml();

			BOOL Load ( LPCSTR filename );

			class node : public TiXmlElement
			{
			public:
				node* Find ( LPCSTR section );
				node* Find( LPCSTR element, LPCSTR id );

				BOOL IsType(LPCSTR name) const ;

				LPCSTR ReadValue ( LPCSTR name );

				//int ReadValueArray ( LPCSTR name, collections::base& c );
				//int ReadValueArray ( LPCSTR  name, collections::c_float& c );

				int ReadIntProperty ( LPCSTR  name, int defaultvalue=0 );
				BOOL ReadBoolProperty ( LPCSTR  name, BOOL defaultvalue=FALSE );
				f32 ReadFloatProperty ( LPCSTR  name, f32 defaultvalue=0 );
				LPCSTR ReadStrProperty ( LPCSTR  name, LPCSTR defaultvalue=NULL );


				//int ReadArray ( LPCSTR name, collections::base& c );
				//int ReadArray ( LPCSTR name, collections::base& c, LPCSTR delim );

				int ReadToken( LPCSTR token, token_t array[], int max, int defaultvalue=0 );
				f32 ReadFloat ( LPCSTR name, f32 defaultvalue=0 );
				BOOL ReadBool( LPCSTR name, BOOL defaultvalue=FALSE );
				int ReadInt( LPCSTR name, int defaultvalue=0, int basevalue=0 );
				LPCSTR ReadStr( LPCSTR name, LPCSTR defaultvalue = NULL );
				int ReadItem ( LPCSTR name, int defaultvalue=0, int basevalue=0 );
				LPCSTR ReadItem ( LPCSTR name, LPCSTR defaultvalue= NULL  );
				f32 ReadItem ( LPCSTR name, f32 defaultvalue=0 );

				BOOL Exists( LPCSTR name );
				int Count ();

				LPCSTR ReadElement( LPCSTR element, LPCSTR id, LPCSTR tag );
				//types::point ReadPoint( LPCSTR name );
				//int ReadColour( LPCSTR name, int defaultvalue );
			};

			node* Find ( LPCSTR section );


			//CDate ReadXml_Date ( TiXmlElement* element, LPCSTR name );
			//TiXmlDocument* Doc() const { return m_doc; }


		};

#define FOREACHELEMENT( parent,element ) \
	for( xml::node* element=(xml::node*)parent->FirstChildElement(); element; element = (xml::node*)element->NextSiblingElement() )


#endif //_XML_H_INCLUDED_
