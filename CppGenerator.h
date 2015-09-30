#pragma once

#include <stdarg.h>
#include "IdlParser.h"

class GenException : public std::exception
{
protected:
	PTElement *m_pNode;

public:
	GenException( PTElement *pNode, const char* pcText )
		: std::exception( pcText ), m_pNode(pNode)
	{
	}

	PTElement *node() const { return m_pNode; }
};


enum eStages {
	eStage_MAIN = 0,
	eStage_MEMBERS = 1,
	eStage_SER = 2,
	eStage_UNSER = 3,
	eStage_GETSET = 4
};
typedef unsigned int eStage;

class CppGenerator
{
protected:
	PTContainer *m_pRoot;
	int m_iTabs;
	FILE *m_fHandle;
	unsigned short m_unCommand;
	unsigned short m_unMaxCommand;

public:
	CppGenerator( PTContainer *pRoot )
		: m_pRoot(pRoot), m_iTabs(0), m_fHandle(0)
	{
		m_fHandle = fopen( "D:\\testOutput.txt", "wb" );
		m_unCommand = 0x0100;
		m_unMaxCommand = 0x03FF;
	}

	~CppGenerator( )
	{
		fclose( m_fHandle );
	}
	
	void _outTabs( int iTabs ) {
		m_iTabs += iTabs;
	}
	
	void _outTxtX( char *format, ... )
	{
		va_list args;
		va_start( args, format );
		vfprintf( m_fHandle, format, args );
		va_end( args );
	}

	void _outTxt( char *format, ... )
	{
		for( int i = 0; i < m_iTabs; ++i ) fprintf( m_fHandle, "\t" );
		va_list args;
		va_start( args, format );
		vfprintf( m_fHandle, format, args );
		va_end( args );
	}

	std::string _getVarName( PTVar *pVar ) {
		return "__" + pVar->get_name();
	}

	std::string _getListName( PTList *pList ) {
		return "_v" + pList->get_name( );
	}

	std::string _getSerializer( PTContainer *pNode )
	{
		PTContainer *pParent = pNode->get_parent( );
		while( pParent ) {
			if( pParent->type() == ePT_Message ) {
				PTMessage *pMsg = (PTMessage*)pParent;
				return "pak_" + pMsg->get_name();
			}
			pParent = pParent->get_parent( );
		}

		return "";
	}

	std::string _getListPath( PTList *pNode )
	{
		std::string sPath = pNode->get_name( );

		PTElement *pParent = pNode->get_parent( );
		while( pParent && pParent->type() == ePT_List ) {
			PTList *pList = (PTList*)pParent;
			sPath = pList->get_name() + "::" + sPath;
			pParent = pParent->get_parent();
		}

		return sPath;
	}

	void _genContainer( PTContainer *pNode, eStage iStage )
	{
		for( auto i = pNode->get_children().begin(); i != pNode->get_children().end(); ++i ) {
			_gen( *i, iStage );
		}
	}

	void _genRoot( PTRoot *pNode, eStage iStage )
	{
		if( iStage != eStage_MAIN ) {
			throw GenException( pNode, "namespace during incorrect stage" );
		}

		_outTxt( "namespace net {\n" );
		_outTabs( +1 );
		{
			_genContainer( pNode, iStage );
		}
		_outTabs( -1 );
		_outTxt( "};\n" );
	}

	void _genEnum( PTEnum *pNode, eStage iStage )
	{
		if( iStage != eStage_MAIN ) {
			throw GenException( pNode, "enum during incorrect stage" );
		}

		_outTxt( "enum %s {\n", pNode->get_name().c_str() );
		_outTabs( +1 );
		for( auto i = pNode->get_values().begin(); i != pNode->get_values().end(); ) {
			_outTxt( "%s", (*i).c_str() );

			++i;
			if( i != pNode->get_values().end() ) {
				_outTxtX( ",\n" );
			} else {
				_outTxtX( "\n" );
			}
		}
		_outTabs( -1 );
		_outTxt( "};\n" );
	}

	void _genNamespace( PTNamespace *pNode, eStage iStage )
	{
		if( iStage != eStage_MAIN ) {
			throw GenException( pNode, "namespace during incorrect stage" );
		}

		_outTxt( "namespace %s {\n", pNode->get_name().c_str() );
		_outTabs( +1 );

		_genContainer( pNode, iStage );

		_outTabs( -1 );
		_outTxt( "};\n" );
	}

	void _genTypedef( PTTypedef *pNode, eStage iStage )
	{
		if( iStage != eStage_MAIN ) {
			throw GenException( pNode, "typedef during incorrect stage" );
		}

		_outTxt( "typedef %s %s;\n", pNode->get_type().c_str(), pNode->get_name().c_str() );
	}

	PTXMsgNBase* _findBaseNode( const std::string& sName, PTContainer *pStart = nullptr )
	{
		if( pStart == nullptr ) pStart = m_pRoot;

		for( auto i = pStart->get_children().begin(); i != pStart->get_children().end(); ++i ) {
			if( (*i)->type() != ePT_Base ) {
				continue;
			}
			if( (*i)->name() == sName ) {
				return (PTXMsgNBase*)(*i);
			}
		}

		PTContainer *pParent = pStart->get_parent();
		if( pParent ) {
			return _findBaseNode( sName, pParent );
		}
		return nullptr;
	}

	void _genMsgCtx( PTXMsgNBase* pNode, eStage iStage )
	{
		if( iStage == eStage_MEMBERS || iStage == eStage_SER || iStage == eStage_UNSER || iStage == eStage_GETSET ) {
			_genInherits( pNode, iStage );
			_genContainer( pNode, iStage );
		} else {
			throw GenException( pNode, "message context during incorrect stage" );
		}
	}

	void _genInherits( PTXMsgNBase *pNode, eStage iStage )
	{
		for( auto i = pNode->get_inherits().begin(); i != pNode->get_inherits().end(); ++i ) {
			PTXMsgNBase *pBase = _findBaseNode( *i, pNode );
			if( !pBase ) {
				throw GenException( pNode, "could not find inherited base definition" );
			}

			_genMsgCtx( pBase, iStage );
		}
	}

	void _genMessage( PTMessage *pNode, eStage iStage )
	{
		if( iStage == eStage_MAIN ) {
			_outTxt( "class pak_%s : packet {\n", pNode->get_name().c_str() );
			_outTabs( +1 );
			{
				_outTxt( "private:\n" );
				_outTabs( +1 );
				{
					_genMsgCtx( pNode, eStage_MEMBERS );
				}
				_outTabs( -1 );

				_outTxt( "public:\n" );
				_outTabs( +1 );
				{
					_genMsgCtx( pNode, eStage_GETSET );

					_outTxt( "\n" );

					unsigned short unType = m_unCommand++;
					if( unType >= m_unMaxCommand ) {
						throw GenException( pNode, "too many packets! (all packet types have been used)" );
					}
					_outTxt( "static const uint16 type_id = 0x%04x;\n", unType );

					_outTxt( "size_t serialize( char *data, int max_len ) const {\n" );
					_outTabs( +1 );
					{
						_outTxt( "size_t pos = 0;\n" );
						_outTxt( "const pak_%s& vars = *this;\n", pNode->get_name().c_str() );
						_genMsgCtx( pNode, eStage_SER );
						_outTxt( "return pos;\n" );
					}
					_outTabs( -1 );
					_outTxt( "}\n" );

					_outTxt( "void unserialize( char *data, int max_len ) {\n" );
					_outTabs( +1 );
					{
						_outTxt( "size_t pos = 0;\n" );
						_outTxt( "pak_%s& vars = *this;\n", pNode->get_name().c_str() );
						_genMsgCtx( pNode, eStage_UNSER );
					}
					_outTabs( -1 );
					_outTxt( "}\n" );
				}
				_outTabs( -1 );
			}
			_outTabs( -1 );
			_outTxt( "};\n" );
		} else {
			throw GenException( pNode, "message during incorrect stage" );
		}
	}

	void _genBase( PTBase *pNode, eStage iStage )
	{
		// these are only inline-composited into messages, and are generated from there
	}

	void _genList( PTList *pNode, eStage iStage )
	{
		if( iStage == eStage_MEMBERS ) {
			_outTxt( "class %s {\n", pNode->get_name().c_str() );
			_outTabs( +1 );
			{
				std::string vSerializer = _getSerializer( pNode );
				if( vSerializer != "" ) {
					_outTxt( "friend class %s;\n", vSerializer.c_str() );
				}

				_outTxt( "private:\n" );
				_outTabs( +1 );
				{
					_genContainer( pNode, iStage );
				}
				_outTabs( -1 );

				_outTxt( "public:\n" );
				_outTabs( +1 );
				{
					_genContainer( pNode, eStage_GETSET );
				}
				_outTabs( -1 );
			}
			_outTabs( -1 );
			_outTxt( "};\n" );
			_outTxt( "std::vector<%s> %s;\n", pNode->get_name().c_str(), _getListName(pNode).c_str() );
		} else if( iStage == eStage_SER ) {
			_outTxt( "for( auto i =  vars.%s.begin(); i != vars.%s.end(); ++i ) {\n", _getListName(pNode).c_str(), _getListName(pNode).c_str() );
			_outTabs( +1 );
			{
				_outTxt( "const %s& vars = *i;\n", _getListPath(pNode).c_str() );
				_genContainer( pNode, iStage );
			}
			_outTabs( -1 );
			_outTxt( "}\n" );
		} else if( iStage == eStage_UNSER ) {
			_outTxt( "for( auto i =  vars.%s.begin(); i != vars.%s.end(); ++i ) {\n", _getListName(pNode).c_str(), _getListName(pNode).c_str() );
			_outTabs( +1 );
			{
				_outTxt( "%s& vars = *i;\n", _getListPath(pNode).c_str() );
				_genContainer( pNode, iStage );
			}
			_outTabs( -1 );
			_outTxt( "}\n" );
		} else if( iStage == eStage_GETSET ) {
			//_outTxt( "%s get_%s( ) const { return %s; }", pNode->get_type().c_str(), pNode->get_name().c_str(), _getVarName(pNode).c_str() );

			//_outTxt( "void set_%s( %s val ) const { %s = val; }", pNode->get_name().c_str(), pNode->get_type().c_str(), _getVarName(pNode).c_str() );
		} else {
			throw GenException( pNode, "list during incorrect stage" );
		}
	}

	void _genVar( PTVar *pNode, eStage iStage )
	{
		if( iStage == eStage_MEMBERS ) {
			if( pNode->get_arrlen() != "" ) {
				_outTxt( "%s %s[%s];\n", pNode->get_type().c_str(), _getVarName(pNode).c_str(), pNode->get_arrlen().c_str() );
			} else {
				_outTxt( "%s %s;\n", pNode->get_type().c_str(), _getVarName(pNode).c_str() );
			}
		} else if( iStage == eStage_SER ) {
			if( pNode->get_arrlen() != "" ) {
				_outTxt( "::net::encoding::write_arr( vars.%s, %s, data, pos, max_len );\n", _getVarName(pNode).c_str(), pNode->get_arrlen().c_str() );
			} else {
				_outTxt( "::net::encoding::write( vars.%s, data, pos, max_len );\n", _getVarName(pNode).c_str() );
			}
		} else if( iStage == eStage_UNSER ) {
			if( pNode->get_arrlen() != "" ) {
				_outTxt( "::net::encoding::read_arr( vars.%s, %s, data, pos, max_len );\n", _getVarName(pNode).c_str(), pNode->get_arrlen().c_str() );
			} else {
				_outTxt( "::net::encoding::read( vars.%s, data, pos, max_len );\n", _getVarName(pNode).c_str() );
			}
		} else if( iStage == eStage_GETSET ) {
			if( pNode->get_arrlen() != "" ) {
				_outTxt( "%s get_%s( int iIdx ) const { return %s[iIdx]; }\n", pNode->get_type().c_str(), pNode->get_name().c_str(), _getVarName(pNode).c_str() );
				_outTxt( "void set_%s( int iIdx, %s val ) { %s[iIdx] = val; }\n", pNode->get_name().c_str(), pNode->get_type().c_str(), _getVarName(pNode).c_str() );
			} else {
				_outTxt( "%s get_%s( ) const { return %s; }\n", pNode->get_type().c_str(), pNode->get_name().c_str(), _getVarName(pNode).c_str() );
				_outTxt( "void set_%s( %s val ) { %s = val; }\n", pNode->get_name().c_str(), pNode->get_type().c_str(), _getVarName(pNode).c_str() );
			}
		} else {
			throw GenException( pNode, "var during incorrect stage" );
		}
	}

	void _gen( PTElement *pNode, eStage iStage )
	{
#define LAZYMAN(x) if( pNode->type() == ePT_##x ) { _gen##x((PT##x*)pNode,iStage); } 
		LAZYMAN(Root)
		else LAZYMAN(Enum)
		else LAZYMAN(Typedef)
		else LAZYMAN(Namespace)
		else LAZYMAN(Message)
		else LAZYMAN(Base)
		else LAZYMAN(List)
		else LAZYMAN(Var)
#undef LAZYMAN
	}

	bool generate( )
	{
		_gen( m_pRoot, eStage_MAIN );

		return true;
	}

};