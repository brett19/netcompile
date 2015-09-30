#pragma warning(disable:4996)

#include <iostream>
#include <string>
#include <vector>
#include <stdarg.h>
#include <map>

std::string _ftok( FILE* fh )
{
	bool bStartRead = false;

	std::string sBuffer;
	while( true ) {
		int cChar = fgetc( fh );

		if( cChar == EOF ) {
			return sBuffer;
		}

		if( cChar == '{' || cChar == '}' || cChar == ';' ) {
			if( bStartRead ) {
				fseek( fh, -1, SEEK_CUR );
				return sBuffer;
			} else {
				sBuffer.push_back( cChar );
				return sBuffer;
			}
		}

		if( cChar == ' ' || cChar == '\t' || cChar == '\r' || cChar == '\n' ) {
			if( bStartRead ) {
				return sBuffer;
			}
		} else {
			if( !bStartRead ) {
				bStartRead = true;
			}
		}

		if( !bStartRead ) {
			continue;
		}

		sBuffer.push_back( cChar );
	}
}

std::string g_PeekTok = "";
std::string ftok( FILE* fh, bool no_term = false )
{
	std::string sToken = "";
	if( g_PeekTok != "" ) {
		sToken = g_PeekTok;
		g_PeekTok = "";
	} else {
		sToken = _ftok(fh);
	}

	if( no_term && ( sToken == ";" || sToken == "}" || sToken == "{" ) ) {
		g_PeekTok = sToken;
		return "";
	}
		
	return sToken;
}

struct STypeName
{
	std::string sName;
	std::string sArrayCount;
	std::string sBitfield;
	int iState;

	inline bool is_error( ) { return iState == -2; }
	inline bool is_array( ) { return iState == -1; }
	inline bool is_bitfield() { return iState == -3; }
	inline bool is_enum() { return iState == -4; }
};

int g_XIndents = 0;
void XIndent( int iNum ) {
	g_XIndents += iNum;
}
void XWrite( const char *format, ... ) {
	for( int i = 0; i < g_XIndents; ++i ) printf( "  " );

	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
}

int g_ZIndents = 0;
void ZIndent( int iNum ) {
	g_ZIndents += iNum;
}
void ZWrite( const char *format, ... ) {
	for( int i = 0; i < g_ZIndents; ++i ) printf( "  " );

	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
}

std::map<std::string,std::string> g_EnumMap;

std::string GetXTypeName( std::string sType )
{
	if( sType == "uint8" ) {
		return "BYTE";
	} else if( sType == "uint16" ) {
		return "WORD";
	} else if( sType == "uint32" ) {
		return "DWORD";
	} else if( sType == "uint64" ) {
		return "unsigned __int64";
	} else if( sType == "int8" ) {
		return "char";
	} else if( sType == "int16" ) {
		return "short";
	} else if( sType == "int32" ) {
		return "int";
	} else if( sType == "int64" ) {
		return "__int64";
	} else if( sType == "string" ) {
		return "char[]";
	}
	return sType;
}

std::string GetZTypeName( std::string sType )
{
	if( sType == "uint8" ) {
		return "::net::uint8";
	} else if( sType == "uint16" ) {
		return "::net::uint16";
	} else if( sType == "uint32" ) {
		return "::net::uint32";
	} else if( sType == "uint64" ) {
		return "::net::uint64";
	} else if( sType == "int8" ) {
		return "::net::int8";
	} else if( sType == "int16" ) {
		return "::net::int16";
	} else if( sType == "int32" ) {
		return "::net::int32";
	} else if( sType == "int64" ) {
		return "::net::int64";
	} else if( sType == "string" ) {
		return "::std::string";
	}
	return sType;
}

enum eZType
{
	eZType_Normal = 0,
	eZType_Enum
};

enum eStage
{
	eStage_Normal,
	eStage_ENormal,
	eStage_ECompat,
	eStage_Members,
	eStage_Ser,
	eStage_Unser,
	eStage_GetSet
};

enum eFlag {
	eFlag_None = 0,
	eFlag_NoComplex = 1
};

class SecBase
{
	friend class SecList;

protected:
	std::vector<SecBase*> m_Children;
	SecBase* m_Parent;

public:
	SecBase( ) : m_Parent( nullptr ) { };
	virtual ~SecBase( ) { };

	void SetParent( SecBase* pParent ) {
		m_Parent = pParent;
	}

	size_t child_count( ) {
		return m_Children.size( );
	}

	virtual unsigned int flags( ) {
		return m_Parent->flags( );
	}

	virtual std::string param_path( int iStage = 0 ) {
		return m_Parent->param_path(iStage);
	}

	virtual std::string friend_name( ) {
		return "";
	}
	
	virtual void x_output( int iStage = 0 )
	{
		for( auto i = m_Children.begin(); i != m_Children.end(); ++i ) {
			(*i)->x_output( iStage );
		}
	}

	virtual void z_output( int iStage = 0 )
	{
		for( auto i = m_Children.begin(); i != m_Children.end(); ++i ) {
			(*i)->z_output( iStage );
		}
	}

	virtual eZType GetZType( ) { return eZType_Normal; }

	virtual void AddChild( SecBase* pObj ) {
		m_Children.push_back( pObj );
	}
};

class SecRoot : public SecBase
{
public:
	virtual unsigned int flags( ) {
		return eFlag_None;
	}

	std::string param_path( int iStage = 0 ) {
		return "";
	}
};

class SecParam : public SecBase
{
protected:
	STypeName m_xType;
	std::string m_sName;
	std::string m_sXName;

public:
	SecParam( STypeName xType, std::string sName, std::string sXName )
		: m_xType(xType), m_sName(sName), m_sXName(sXName) { }

	void x_output( int iStage = 0 ) {
		std::string sType = m_xType.sName;
		if( m_xType.is_enum() ) {
			sType = g_EnumMap[ sType ];
		}

		std::string sXType = GetXTypeName(sType);

		if( m_sXName != "" ) {
			if( m_xType.is_array() ) {
				XWrite( "%s %s[%s];\n", sXType.c_str(), m_sXName.c_str(), m_xType.sArrayCount.c_str() );
			} else if( m_xType.is_bitfield() ) {
				XWrite( "%s %s : %s;\n", sXType.c_str(), m_sXName.c_str(), m_xType.sBitfield.c_str() );
			} else {
				XWrite( "%s %s;\n", sXType.c_str(), m_sXName.c_str() );
			}
		} else {
			XWrite( "// %s %s;\n", sXType.c_str(), m_sName.c_str() );
		}
	}

	void z_output( int iStage = 0 ) {
		if( flags() & eFlag_NoComplex ) {
			if( m_xType.sName == "string" ) {
				throw std::exception( "Cannot use a string inside a union" );
			}
		}

		std::string sType = m_xType.sName;
		std::string sZType = GetZTypeName(sType);

		if( m_xType.is_enum() ) { sZType = "e" + sType; }

		if( iStage == eStage_Members ) {
			if( m_sName != "" ) {
				if( m_xType.is_array() ) {
					ZWrite( "%s _%s[%s];\n", sZType.c_str(), m_sName.c_str(), m_xType.sArrayCount.c_str() );
				} else if( m_xType.is_bitfield() ) {
					ZWrite( "%s _%s : %s;\n", sZType.c_str(), m_sName.c_str(), m_xType.sBitfield.c_str() );
				} else {
					ZWrite( "%s _%s;\n", sZType.c_str(), m_sName.c_str() );
				}
			} else {
				static int iReserved = 0;
				iReserved++;

				if( m_xType.is_array() ) {
					ZWrite( "%s _reserved_%d[%s];\n", sZType.c_str(), iReserved, m_xType.sArrayCount.c_str() );
				} else if( m_xType.is_bitfield() ) {
					ZWrite( "%s _reserved_%d : %s;\n", sZType.c_str(), iReserved, m_xType.sBitfield.c_str() );
				} else {
					ZWrite( "%s _reserved_%d;\n", sZType.c_str(), iReserved );
				}
			}
		} else if( iStage == eStage_GetSet ) {
			if( m_sName != "" ) 
			{
				if( m_xType.is_array() ) {
					ZWrite( "inline void set_%s( size_t idx_, const %s& %s ) {\n", m_sName.c_str(), sZType.c_str(), m_sName.c_str() );
					ZIndent( +1 );
					ZWrite( "%s_%s[idx_] = %s;\n", param_path(iStage).c_str(), m_sName.c_str(), m_sName.c_str() );
					ZIndent( -1 );
					ZWrite( "}\n" );

					ZWrite( "inline const %s& get_%s( size_t idx_ ) {\n", sZType.c_str(), m_sName.c_str() );
					ZIndent( +1 );
					ZWrite( "return %s_%s[idx_];\n", param_path(iStage).c_str(), m_sName.c_str(), m_sName.c_str() );
					ZIndent( -1 );
					ZWrite( "}\n" );
				} else {
					ZWrite( "inline void set_%s( const %s& %s ) {\n", m_sName.c_str(), sZType.c_str(), m_sName.c_str() );
					ZIndent( +1 );
					ZWrite( "%s_%s = %s;\n", param_path(iStage).c_str(), m_sName.c_str(), m_sName.c_str() );
					ZIndent( -1 );
					ZWrite( "}\n" );

					if( m_xType.is_bitfield() ) {
						// Cannot return a reference for a bitfield!
						ZWrite( "inline %s get_%s( ) {\n", sZType.c_str(), m_sName.c_str() );
					} else {
						ZWrite( "inline const %s& get_%s( ) {\n", sZType.c_str(), m_sName.c_str() );
					}
					ZIndent( +1 );
					ZWrite( "return %s_%s;\n", param_path(iStage).c_str(), m_sName.c_str() );
					ZIndent( -1 );
					ZWrite( "}\n" );
				}
			}
		} else if( iStage == eStage_Ser ) {
			if( m_sName != "" ) {
				if( m_xType.sName == "string" ) {
					ZWrite( "memcpy( &data[len_out], %s_%s.c_str(), %s_%s.size()+1 );\n", param_path(iStage).c_str(), m_sName.c_str(), param_path(iStage).c_str(), m_sName.c_str() );
					ZWrite( "len_out += %s_%s.size()+1;\n", param_path(iStage).c_str(), m_sName.c_str() );
				} else {
					if( m_xType.is_array() ) {
						ZWrite( "memcpy( &data[len_out], %s_%s, sizeof(%s) * %s );\n", param_path(iStage).c_str(), m_sName.c_str(), sZType.c_str(), m_xType.sArrayCount.c_str() );
						ZWrite( "len_out += sizeof(%s) * %s;\n", sZType.c_str(), m_xType.sArrayCount.c_str() );
					} else {
						ZWrite( "*((%s*)&data[len_out]) = %s_%s;\n", sZType.c_str(), param_path(iStage).c_str(), m_sName.c_str() );
						ZWrite( "len_out += sizeof(%s);\n", sZType.c_str() );
					}
				}
			} else {
				ZWrite( "len_out += sizeof(%s);\n", sZType.c_str() );
			}
		} else if( iStage == eStage_Unser ) {
			if( m_sName != "" ) {
				if( m_xType.sName == "string" ) {
					ZWrite( "if(!::net::read_string(%s_%s,data,len,cur_pos)) return false;\n", param_path(iStage).c_str(), m_sName.c_str() );
				} else {
					if( m_xType.is_array() ) {
						ZWrite( "memcpy( %s_%s, &data[cur_pos], sizeof(%s) * %s );\n", param_path(iStage).c_str(), m_sName.c_str(), sZType.c_str(), m_xType.sArrayCount.c_str() );
						ZWrite( "cur_pos += sizeof(%s) * %s;\n", sZType.c_str(), m_xType.sArrayCount.c_str() );
					} else {
						ZWrite( "%s_%s = *((%s*)&data[cur_pos]);\n", param_path(iStage).c_str(), m_sName.c_str(), sZType.c_str() );
						ZWrite( "cur_pos += sizeof(%s);\n", sZType.c_str() );
					}
				}
			} else {
				ZWrite( "cur_pos += sizeof(%s);\n", sZType.c_str() );
			}
		}
	}
};

class SecMessage : public SecBase
{
protected:
	std::string m_sName;

public:
	SecMessage( std::string sName ) : m_sName(sName) { }

	std::string friend_name( ) {
		return std::string("pak_") + m_sName;
	}

	void x_output( int iStage = 0 ) {
		XWrite( "struct %s : public t_PacketHEADER {\n", m_sName.c_str() );
		XIndent( +1 );

		SecBase::x_output( );

		XIndent( -1 );
		XWrite( "};\n\n" );
	}

	void z_output( int iStage = 0 ) {
		ZWrite( "class pak_%s : public ::net::packet {\n", m_sName.c_str() );
		ZIndent( +1 );

		ZWrite( "protected:\n" );
		ZIndent( +1 );
		{
			SecBase::z_output( eStage_Members );
		}
		ZIndent( -1 );
		ZWrite( "\n" );

		ZWrite( "public:\n" );
		ZIndent( +1 );
		{
			SecBase::z_output( eStage_GetSet );

			ZWrite( "inline bool serialize( char *data, int max_len, int& len_out ) {\n" );
			ZIndent( +1 );
			ZWrite( "len_out = 0;\n" );
			SecBase::z_output( eStage_Ser );
			ZWrite( "return true;\n" );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline bool unserialize( char *data, int len ) {\n" );
			ZIndent( +1 );
			ZWrite( "int cur_pos = 0;\n" );
			SecBase::z_output( eStage_Unser );
			ZWrite( "return true;\n" );
			ZIndent( -1 );
			ZWrite( "}\n" );
		}
		ZIndent( -1 );
		ZWrite( "\n" );

		ZIndent( -1 );
		ZWrite( "};\n\n" );
	}
};

class SecAlign : public SecBase
{
protected:
	std::string m_sAlignBytes;

public:
	SecAlign( std::string sAlignBytes ) : m_sAlignBytes(sAlignBytes) { }

	void x_output( int iStage = 0 ) {
		XWrite( "#pragma pack(push:%s)\n", m_sAlignBytes.c_str() );
		XIndent( +1 );

		SecBase::x_output( );

		XIndent( -1 );
		XWrite( "#pragma pack(pop)\n\n" );
	}
};

class SecList : public SecBase
{
protected:
	std::string m_sType;
	std::string m_sName;
	std::string m_sXCntName;
	std::string m_sXVarName;

public:
	SecList( std::string sName, std::string sType, std::string sXCntName, std::string sXVarName )
		: m_sName(sName), m_sType(sType), m_sXCntName(sXCntName), m_sXVarName(sXVarName) { }

	std::string param_path( int iStage = 0 ) {
		if( iStage != eStage_GetSet ) {
			std::string sVarName = m_sName;
			if( sVarName.size() > 0 ) sVarName[0] = tolower(sVarName[0]);
			return sVarName + "_itr->";
		} else {
			return SecBase::param_path( iStage );
		}
	}

	void x_output( int iStage = 0 ) {
		if( m_sXCntName != "" ) {
			XWrite( "%s %s;\n", GetXTypeName(m_sType).c_str(), m_sXCntName.c_str() );
		}

		XWrite( "struct %s {\n", m_sName.c_str() );
		XIndent( +1 );

		SecBase::x_output( );

		XIndent( -1 );

		if( m_sXVarName != "" ) {
			XWrite( "} %s[];\n", m_sXVarName.c_str() );
		} else {
			XWrite( "};\n" );
		}
	}

	void z_output( int iStage = 0 ) {
		if( flags() & eFlag_NoComplex ) {
			throw std::exception( "Cannot use a list inside a union" );
		}

		std::string sVarName = m_sName;
		if( sVarName.size() > 0 ) sVarName[0] = tolower(sVarName[0]);

		if( iStage == eStage_Members ) {
			ZWrite( "class %s {\n", m_sName.c_str() );
			ZIndent( +1 );

			SecBase* pCurPtr = m_Parent;
			while( pCurPtr ) {
				std::string sFriendName = pCurPtr->friend_name();
				if( sFriendName != "" ) {
					ZWrite( "friend class %s;\n", sFriendName.c_str() );
				}
				pCurPtr = pCurPtr->m_Parent;
			}

			ZWrite( "protected:\n" );
			ZIndent( +1 );
			{
				SecBase::z_output( eStage_Members );
			}
			ZIndent( -1 );
			ZWrite( "\n" );

			ZWrite( "public:\n" );
			ZIndent( +1 );
			{
				SecBase::z_output( eStage_GetSet );
			}
			ZIndent( -1 );
			ZWrite( "\n" );

			ZIndent( -1 );
			ZWrite( "};\n" );

			ZWrite( "::std::vector<%s> _%ss;\n", m_sName.c_str(), sVarName.c_str() );

		} else if( iStage == eStage_GetSet ) {

			ZWrite( "inline const ::std::vector<%s>& %ss( ) {\n", m_sName.c_str(), sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "return _%ss;\n", sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline size_t %s_count( ) {\n", sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "return _%ss.size();\n", sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline void set_%s( size_t idx_, const %s& %s ) {\n", sVarName.c_str(), m_sName.c_str(), sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "_%ss[idx_] = %s;\n", sVarName.c_str(), sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline const %s& %s( size_t idx_ ) {\n", m_sName.c_str(), sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "return _%ss[idx_];\n", sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline void add_%s( const %s& %s ) {\n", sVarName.c_str(), m_sName.c_str(), sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "_%ss.push_back(%s);\n", sVarName.c_str(), sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );

			ZWrite( "inline void clear_%ss( ) {\n", sVarName.c_str() );
			ZIndent( +1 );
			ZWrite( "_%ss.clear();\n", sVarName.c_str() );
			ZIndent( -1 );
			ZWrite( "}\n" );
		} else if( iStage == eStage_Ser ) {
			ZWrite( "*((%s*)&data[len_out]) = (%s)%s_%ss.size( );\n", GetZTypeName(m_sType).c_str(), GetZTypeName(m_sType).c_str(), SecBase::param_path().c_str(), sVarName.c_str() );
			ZWrite( "len_out += sizeof(%s);\n", GetZTypeName(m_sType).c_str() );

			ZWrite( "for( auto %s_itr = %s_%ss.begin(); %s_itr != %s_%ss.end(); ++%s_itr ) {\n", 
				sVarName.c_str(), SecBase::param_path().c_str(), sVarName.c_str(), sVarName.c_str(), 
				SecBase::param_path().c_str(), sVarName.c_str(), sVarName.c_str() );
			ZIndent( +1 );

			SecBase::z_output( iStage );

			ZIndent( -1 );
			ZWrite( "}\n" );
		} else if( iStage == eStage_Unser ) {
			ZWrite( "%s_%ss.resize( (size_t)*((%s*)&data[cur_pos]) );\n", SecBase::param_path().c_str(), sVarName.c_str(), GetZTypeName(m_sType).c_str() );
			ZWrite( "for( auto %s_itr = %s_%ss.begin(); %s_itr != %s_%ss.end(); ++%s_itr ) {\n", 
				sVarName.c_str(), SecBase::param_path().c_str(), sVarName.c_str(), sVarName.c_str(), 
				SecBase::param_path().c_str(), sVarName.c_str(), sVarName.c_str() );
			ZIndent( +1 );

			SecBase::z_output( iStage );

			ZIndent( -1 );
			ZWrite( "}\n" );
		}
	}
};

class SecUnion : public SecBase
{
protected:
	std::string m_sName;

public:
	SecUnion( ) {
		static int iUnionInfo = 0;
		char tmp[32];
		sprintf( tmp, "__union_%d", ++iUnionInfo );
		m_sName = tmp;
	}

	unsigned int flags( ) {
		return SecBase::flags( ) | eFlag_NoComplex;
	}

	std::string param_path( int iStage = 0 ) {
		return SecBase::param_path() + m_sName + ".";
	}
	
	void x_output( int iStage = 0 ) {
		XWrite( "union {\n" );
		XIndent( +1 );

		SecBase::x_output();

		XIndent( -1 );
		XWrite( "};\n" );
	}

	void z_output( int iStage = 0 ) {
		if( iStage == eStage_Members ) {
			ZWrite( "union {\n" );
			ZIndent( +1 );
			
			SecBase::z_output( iStage );

			ZIndent( -1 );
			ZWrite( "} %s;\n", m_sName.c_str() );
		} else if( iStage == eStage_GetSet ) {
			SecBase::z_output( iStage );
		} else if( iStage == eStage_Ser ) {
			
			ZWrite( "memcpy( &data[len_out], &%s%s, sizeof(%s%s) );\n", SecBase::param_path().c_str(), m_sName.c_str(), SecBase::param_path().c_str(), m_sName.c_str() );
			ZWrite( "len_out += sizeof(%s%s);\n", SecBase::param_path().c_str(), m_sName.c_str() );

		} else if( iStage == eStage_Unser ) {
		
			ZWrite( "memcpy( &%s%s, &data[cur_pos], sizeof(%s%s) );\n", SecBase::param_path().c_str(), m_sName.c_str(), SecBase::param_path().c_str(), m_sName.c_str() );
			ZWrite( "cur_pos += sizeof(%s%s);\n", SecBase::param_path().c_str(), m_sName.c_str() );

		}
	}
};

class SecGroup : public SecBase
{
protected:
	std::string m_sXName;

public:
	SecGroup( std::string sXName ) : m_sXName(sXName) { }

	void x_output( int iStage = 0 ) {
		XWrite( "struct {\n" );
		XIndent( +1 );

		SecBase::x_output( );

		XIndent( -1 );
		if( m_sXName != "" ) {
			XWrite( "} %s;\n", m_sXName.c_str() );
		} else {
			XWrite( "};\n" );
		}
	}

	void z_output( int iStage = 0 ) {
		if( iStage == eStage_Members ) {
			ZWrite( "struct {\n" );
			ZIndent( +1 );

			SecBase::z_output( iStage );

			ZIndent( -1 );
			ZWrite( "};\n" );
		} else if( iStage == eStage_GetSet ) {
			SecBase::z_output( iStage );
		} else if( iStage == eStage_Ser ) {
			SecBase::z_output( iStage );
		} else if( iStage == eStage_Unser ) {
			SecBase::z_output( iStage );
		}
	}
};

class SecEnum : public SecBase
{
	friend class SecEnumEntry;

protected:
	std::string m_sName;
	STypeName m_xType;

public:
	SecEnum( std::string sName, STypeName xType ) : m_sName(sName), m_xType(xType) { }

	virtual eZType GetZType( ) { return eZType_Enum; }

	void x_output( int iStage = 0 ) {
		XWrite( "enum e%s : %s {\n", m_sName.c_str(), GetXTypeName(m_xType.sName).c_str() );
		XIndent( +1 );

		SecBase::x_output( eStage_ENormal );
		SecBase::x_output( eStage_ECompat );

		XIndent( -1 );
		XWrite( "};\n\n" );
	}
};

class SecEnumEntry : public SecBase
{
protected:
	std::string m_sValue;
	std::string m_sName;
	std::string m_sXName;

public:
	SecEnumEntry( std::string sName, std::string sXName, std::string sValue )
		: m_sName(sName), m_sXName(sXName), m_sValue(sValue) { }

	void x_output( int iStage = 0 ) {
		if( m_Parent->GetZType() != eZType_Enum ) {
			throw std::exception( "Enum Entry parent is not an Enum!" );
		}

		SecEnum* pParentEnum = (SecEnum*)m_Parent;
		std::string sEName = pParentEnum->m_sName;

		if( iStage == eStage_ENormal ) {
			XWrite( "e%s_%s = %s,\n", sEName.c_str(), m_sName.c_str(), m_sValue.c_str() );
		}
		if( iStage == eStage_ECompat ) {
			if( m_sXName != ";" ) {
				XWrite( "%s = e%s_%s,\n", m_sXName.c_str(), sEName.c_str(), m_sName.c_str() );
			}
		}
	}

	virtual void AddChild( SecBase* pObj ) {
		throw std::exception( "Cannot add child to Enum Entry" );
	}
};

SecBase* g_NextPush = nullptr;
std::vector<SecBase*> g_Stack;

void StackPush( )
{
	if( g_NextPush ) {
		g_NextPush->SetParent( g_Stack.back() );
		g_Stack.back()->AddChild( g_NextPush );
		g_Stack.push_back( g_NextPush );
		g_NextPush = nullptr;
	} else {
		throw std::exception( "Nothing to Push!" );
	}
}

SecBase* GetStack( ) {
	return g_Stack.back();
}

void StackPop( )
{
	g_Stack.pop_back( );
}

void AddStack( SecBase* pNextPush )
{
	if( g_NextPush ) {
		delete g_NextPush;
	}
	g_NextPush = pNextPush;
}

void AddSec( SecBase* pObj )
{
	pObj->SetParent( g_Stack.back() );
	g_Stack.back()->AddChild( pObj );
}

eZType GetStackZType( )
{
	return g_Stack.back()->GetZType();
}

STypeName ParseTypeName( std::string sType ) {
	STypeName xOut;

	int iArrStart = sType.find_first_of('[');
	int iArrEnd = sType.find_first_of(']');
	int iBitPos = sType.find_first_of(':');

	if( sType.compare(0,2,"E:") == 0 ) {
		xOut.iState = -4;
		xOut.sName = sType.substr(2);
		return xOut;
	}

	if( ( iArrStart != std::string::npos || iArrEnd != std::string::npos ) && iBitPos != std::string::npos ) {
		xOut.iState = -2;
		return xOut;
	}

	if( iArrStart != std::string::npos && iArrEnd == std::string::npos ) {
		xOut.iState = -2;
		return xOut;
	}
	if( iArrEnd != std::string::npos && iArrStart == std::string::npos ) {
		xOut.iState = -2;
		return xOut;
	}

	if( iBitPos != std::string::npos ) {
		xOut.iState = -3;
		xOut.sBitfield = sType.substr(iBitPos+1);
		xOut.sName = sType.substr(0,iBitPos);
		return xOut;
	}

	if( iArrStart != std::string::npos && iArrEnd != std::string::npos ) {
		xOut.iState = -1;
		xOut.sArrayCount = sType.substr(iArrStart+1,iArrEnd-iArrStart-1);
		xOut.sName = sType.substr(0,iArrStart);
		return xOut;
	}

	xOut.iState = 0;
	xOut.sName = sType;
	return xOut;
}

int main( int argc, char* argv[] )
{
	FILE* fh = fopen( "c:\\Users\\Brett\\Desktop\\testIdl.txt", "rb" );
	if( !fh ) {
		printf( "Error Opening File!\n" );
		return 1;
	}

	try {
	
		SecBase* pRoot = new SecRoot( );
		g_Stack.push_back(pRoot);

		while( !feof(fh) ) {

			std::string sToken = ftok(fh);

			if( sToken == ";" ) {
				continue;
			} else if( sToken == "{" ) {
				StackPush( );
				continue;
			} else if( sToken == "}" ) {
				StackPop( );
				continue;
			} else if( sToken == "" ) {
				continue;
			}

			if( GetStackZType() == eZType_Normal )
			{
				if( sToken == "message" ) {
					std::string sName = ftok(fh);
					AddStack( new SecMessage(sName) );
					continue;
				}

				if( sToken == "@align" ) {
					std::string sAlignBytes = ftok(fh);
					AddStack( new SecAlign(sAlignBytes) );
					continue;
				}

				if( sToken == "enum" ) {
					std::string sName = ftok(fh);
					std::string sType = ftok(fh);
					STypeName xType = ParseTypeName(sType);

					if( xType.is_array() || xType.is_bitfield() ) {
						throw std::exception( "Enum Type cannot be Array or Bitfield!" );
					}

					g_EnumMap.insert(std::pair<std::string,std::string>(sName,sType));
					AddStack(new SecEnum(sName,xType));
					continue;
				};

				if( sToken == "list" ) {
					std::string sType = ftok(fh);
					std::string sName = ftok(fh);
					std::string sXCntName = ftok(fh,true);
					std::string sXVarName = ftok(fh,true);
					AddStack( new SecList(sName,sType,sXCntName,sXVarName) );
					continue;
				};

				if( sToken == "union" ) {
					AddStack( new SecUnion() );
					continue;
				}

				if( sToken == "group" ) {
					std::string sXName = ftok(fh,true);
					AddStack( new SecGroup(sXName) );
					continue;
				}

				// Normal Parameters
				std::string sType = sToken;
				std::string sName = ftok(fh);
				std::string sXName = ftok(fh,true);
				STypeName xType = ParseTypeName(sType);

				if( sXName == "%" ) sXName = "";
				if( sName == "%" ) sName = "";

				AddSec(new SecParam( xType, sName, sXName ));
			}
			else if( GetStackZType() == eZType_Enum )
			{
				std::string sEId = sToken;
				std::string sEName = ftok(fh);
				std::string sEXName = ftok(fh,true);

				AddSec(new SecEnumEntry( sEName, sEXName, sEId ));
			}
		}

		if( GetStack() != pRoot ) {
			throw std::exception( "Parser Error - Invalid Root" );
		}
		g_Stack.pop_back( );

		size_t iMsgCnt = pRoot->child_count();

		//pRoot->x_output( );
		pRoot->z_output( );

		printf( "Compiled %d Messages\n", iMsgCnt );

		delete pRoot;
	} catch( std::exception& e ) {
		printf( "EXCEPTION: %s\n", e.what() );
	}

	fclose( fh );
	//system( "PAUSE" );
	return 0;
}