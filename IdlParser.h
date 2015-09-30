#pragma once

#include <exception>
#include <vector>
#include "IdlLexer.h"

class ParseTokException : public std::exception
{
protected:
	SToken m_xToken;

public:
	ParseTokException( SToken xToken, char *pcWhat )
		: std::exception(pcWhat), m_xToken(xToken)
	{
	}

	SToken token( ) const { return m_xToken; }
};

class ParseException : public std::exception
{
};

enum eParseType
{
	ePT_Unknown = 0,
	ePT_Root,
	ePT_Typedef,
	ePT_Enum,
	ePT_Namespace,
	ePT_Message,
	ePT_Base,
	ePT_List,
	ePT_Var
};

static char* ePT_Names[] = {
	"UNKNOWN",
	"ROOT",
	"TYPEDEF",
	"ENUM",
	"NAMESPACE",
	"MESSAGE",
	"BASE",
	"LIST",
	"VAR"
};

class PTContainer;

class PTElement
{
protected:
	int m_iLineNum;
	PTContainer *m_pParent;

public:
	PTElement( ) : m_pParent(0), m_iLineNum(0) { }
	virtual ~PTElement( ) { }

	virtual eParseType type() const { return ePT_Unknown; }
	virtual std::string name() const { return ""; }

	void set_linenum( int iLineNum ) { m_iLineNum = iLineNum; }
	int line_num( ) const { return m_iLineNum; }

	void set_parent( PTContainer *pParent ) { m_pParent = pParent; }
	PTContainer* get_parent( ) const { return m_pParent; }

};

class PTContainer : public PTElement
{
protected:
	std::vector<PTElement*> m_vChildren;

public:
	const std::vector<PTElement*>& get_children( ) const { return m_vChildren; }
	size_t get_child_cnt( ) const { return m_vChildren.size(); }
	PTElement* get_child( int iIdx ) const { return m_vChildren[iIdx]; }

	void add_child( PTElement *pChild )
	{
		m_vChildren.push_back( pChild );
	}

};

class PTRoot : public PTContainer
{
public:
	virtual eParseType type() const { return ePT_Root; }

};

class PTTypedef : public PTElement
{
protected:
	std::string m_sType;
	std::string m_sName;

public:
	virtual eParseType type() const { return ePT_Typedef; }
	virtual std::string name() const { return m_sName; }

	std::string get_name( ) const { return m_sName; }
	std::string get_type( ) const { return m_sType; }
	
	void set_name( const std::string& sName ) { m_sName = sName; }
	void set_type( const std::string& sType ) { m_sType = sType; }

};

class PTNamespace : public PTContainer
{
protected:
	std::string m_sName;

public:
	virtual eParseType type() const { return ePT_Namespace; }

	std::string get_name( ) const { return m_sName; }

	void set_name( const std::string& sName ) { m_sName = sName; }

};

class PTEnum : public PTElement
{
protected:
	std::string m_sName;
	std::vector<std::string> m_vValues;

public:
	virtual eParseType type() const { return ePT_Enum; }
	virtual std::string name() const { return m_sName; }

	std::string get_name( ) const { return m_sName; }
	const std::vector<std::string>& get_values( ) const { return m_vValues; }
	size_t get_value_cnt( ) const { return m_vValues.size(); }
	std::string get_value( int iIdx ) const { return m_vValues[iIdx]; }

	void set_name( const std::string& sName ) { m_sName = sName; }
	void add_value( const std::string& sName ) { m_vValues.push_back( sName ); }

};

class PTXMsgNBase : public PTContainer
{
protected:
	std::string m_sName;
	std::vector<std::string> m_vInherit;

public:
	virtual eParseType type() const { return ePT_Unknown; }
	virtual std::string name() const { return m_sName; }

	std::string get_name( ) const { return m_sName; }
	const std::vector<std::string>& get_inherits( ) const { return m_vInherit; }
	size_t get_inherit_cnt( ) const { return m_vInherit.size( ); }
	std::string get_inherit( int iIdx ) const { return m_vInherit[iIdx]; }

	void set_name( const std::string& sName ) { m_sName = sName; }
	void add_inherit( const std::string& sName ) { m_vInherit.push_back(sName); }

};

class PTList : public PTXMsgNBase
{
protected:

public:
	virtual eParseType type() const { return ePT_List; }

};

class PTMessage : public PTXMsgNBase
{
protected:

public:
	virtual eParseType type() const { return ePT_Message; }

};

class PTBase : public PTXMsgNBase
{
protected:

public:
	virtual eParseType type() const { return ePT_Base; }

};

class PTVar : public PTElement
{
protected:
	std::string m_sName;
	std::string m_sType;
	std::string m_sArrLen;

public:
	virtual eParseType type() const { return ePT_Var; }
	virtual std::string name() const { return m_sName; }

	std::string get_name( ) const { return m_sName; }
	std::string get_type( ) const { return m_sType; }
	std::string get_arrlen( ) const { return m_sArrLen; }

	void set_name( const std::string& sName ) { m_sName = sName; }
	void set_type( const std::string& sType ) { m_sType = sType; }
	void set_arrlen( const std::string& sArrLen ) { m_sArrLen = sArrLen; }

};

class IdlParser
{
protected:
	IdlLexer *m_pLexer;
	PTRoot *m_pRoot;
	PTContainer *m_pCurNode;

public:
	IdlParser( IdlLexer *pLexer )
		: m_pLexer(pLexer )
	{
		m_pRoot = new PTRoot( );
		m_pCurNode = m_pRoot;
	}

	bool parseTypedef( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_TYPEDEF ) {
			throw ParseTokException( xKeyword, "typedef keyword expected eTok_KEY_ENUM" );
		}

		SToken xType = m_pLexer->readToken();
		if( xType.iType != eTok_LITERAL ) {
			throw ParseTokException( xType, "typedef type expected eTok_LITERAL" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "typedef name expected eTok_LITERAL" );
		}

		SToken xTerm = m_pLexer->readToken();
		if( xTerm.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xTerm, "typedef terminator expected eTok_TERMINATOR" );
		}
	
		PTTypedef* pNode = new PTTypedef( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );
		pNode->set_type( xType.sText );

		m_pCurNode->add_child( pNode );
		
		return true;
	}

	bool parseEnum( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_ENUM ) {
			throw ParseTokException( xKeyword, "enum keyword expected eTok_KEY_ENUM" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "enum name expected eTok_LITERAL" );
		}

		PTEnum* pNode = new PTEnum( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );

		SToken xToken;
		xToken = m_pLexer->readToken( );
		if( xToken.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xKeyword, "enum expected eTok_BRACE_OPEN" );
		}

		// Check for empty enum
		xToken = m_pLexer->peekToken( );
		if( xToken.iType != eTok_BRACE_CLOSE ) {

			// Loop all the enum values
			while( true ) {

				xToken = m_pLexer->readToken( );
				if( xToken.iType != eTok_LITERAL ) {
					throw ParseTokException( xToken, "enum expected eTok_LITERAL" );
				}

				pNode->add_value( xToken.sText );

				xToken = m_pLexer->readToken( );
				if( xToken.iType != eTok_BRACE_CLOSE && xToken.iType != eTok_COMMA ) {
					throw ParseTokException( xToken, "enum expected eTok_BRACE_CLOSE or eTok_COMMA" );
				}
				if( xToken.iType == eTok_BRACE_CLOSE ) {
					break;
				}
			}
		}

		xToken = m_pLexer->readToken( );
		if( xToken.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xToken, "enum expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool parseNamespace( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_NAMESPACE ) {
			throw ParseTokException( xKeyword, "namespace keyword expected eTok_KEY_NAMESPACE" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "namespace name expected eTok_LITERAL" );
		}

		PTNamespace* pNode = new PTNamespace( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );

		SToken xBegin = m_pLexer->readToken( );
		if( xBegin.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xBegin, "namespace beginning expected eTok_BRACE_OPEN" );
		}

		_pushAndParse( pNode );

		SToken xEnd = m_pLexer->readToken( );
		if( xEnd.iType != eTok_BRACE_CLOSE ) {
			throw ParseTokException( xEnd, "namespace ending expected eTok_BRACE_CLOSE" );
		}

		SToken xTerminator = m_pLexer->readToken( );
		if( xTerminator.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xEnd, "namespace terminator expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool parseVar( )
	{
		SToken xType = m_pLexer->readToken( );
		if( xType.iType != eTok_LITERAL ) {
			throw ParseTokException( xType, "var type expected eTok_LITERAL" );
		}

		SToken xName = m_pLexer->readToken( );
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "var name expected eTok_LITERAL" );
		}

		PTVar* pNode = new PTVar( );
		pNode->set_linenum( xName.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );
		pNode->set_type( xType.sText );

		SToken xToken = m_pLexer->peekToken( );
		if( xToken.iType != eTok_TERMINATOR && xToken.iType != eTok_ARR_OPEN ) {
			throw ParseTokException( xToken, "var expected eTok_TERMINATOR or eTok_ARR_OPEN" );
		}

		if( xToken.iType == eTok_ARR_OPEN ) {

			SToken xArrOpen = m_pLexer->readToken( );
			// Already validated above

			SToken xArrSize = m_pLexer->readToken( );
			if( xArrSize.iType != eTok_LITERAL ) {
				throw ParseTokException( xArrSize, "var array size expected eTok_LITERAL" );
			}

			pNode->set_arrlen( xArrSize.sText );

			SToken xArrClose = m_pLexer->readToken( );
			if( xArrClose.iType != eTok_ARR_CLOSE ) {
				throw ParseTokException( xArrClose, "var array closer expected eTok_ARR_CLOSE" );
			}
		}

		SToken xTerminator = m_pLexer->readToken( );
		if( xTerminator.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xTerminator, "var terminator expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool parseMessage( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_MESSAGE ) {
			throw ParseTokException( xKeyword, "message keyword expected eTok_KEY_MESSAGE" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "message name expected eTok_LITERAL" );
		}

		PTMessage* pNode = new PTMessage( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );

		SToken xToken = m_pLexer->peekToken( );
		if( xToken.iType != eTok_SEPERATOR && xToken.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "message expected eTok_BRACE_OPEN or eTok_SEPERATOR" );
		}

		if( xToken.iType == eTok_SEPERATOR ) {

			SToken xSeperator = m_pLexer->readToken( );
			// Already validated above

			while( true ) {
				SToken xIName = m_pLexer->readToken( );
				if( xIName.iType != eTok_LITERAL ) {
					throw ParseTokException( xToken, "message inherit name expected eTok_LITERAL" );
				}

				pNode->add_inherit( xIName.sText );

				SToken xNextSep = m_pLexer->peekToken( );
				if( xNextSep.iType == eTok_COMMA ) {
					// Skip the token and continue
					m_pLexer->readToken( );
				} else if( xNextSep.iType == eTok_BRACE_OPEN ) {
					// Stop gathering inheritances!
					break;
				} else {
					throw ParseTokException( xNextSep, "message expected eTok_COMMA or eTok_BRACE_OPEN" );
				}
			}
		}

		SToken xBegin = m_pLexer->readToken( );
		if( xBegin.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "message beginning expected eTok_BRACE_OPEN" );
		}

		_pushAndParse( pNode );

		SToken xEnd = m_pLexer->readToken( );
		if( xEnd.iType != eTok_BRACE_CLOSE ) {
			throw ParseTokException( xEnd, "message ending expected eTok_BRACE_CLOSE" );
		}

		SToken xTerminator = m_pLexer->readToken( );
		if( xTerminator.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xEnd, "message terminator expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool parseBase( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_BASE ) {
			throw ParseTokException( xKeyword, "base keyword expected eTok_KEY_MESSAGE" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "base name expected eTok_LITERAL" );
		}

		PTBase* pNode = new PTBase( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );

		SToken xToken = m_pLexer->peekToken( );
		if( xToken.iType != eTok_SEPERATOR && xToken.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "base expected eTok_BRACE_OPEN or eTok_SEPERATOR" );
		}

		if( xToken.iType == eTok_SEPERATOR ) {

			SToken xSeperator = m_pLexer->readToken( );
			// Already validated above

			while( true ) {
				SToken xIName = m_pLexer->readToken( );
				if( xIName.iType != eTok_LITERAL ) {
					throw ParseTokException( xToken, "base inherit name expected eTok_LITERAL" );
				}

				pNode->add_inherit( xIName.sText );

				SToken xNextSep = m_pLexer->peekToken( );
				if( xNextSep.iType == eTok_COMMA ) {
					// Skip the token and continue
					m_pLexer->readToken( );
				} else if( xNextSep.iType == eTok_BRACE_OPEN ) {
					// Stop gathering inheritances!
					break;
				} else {
					throw ParseTokException( xNextSep, "base expected eTok_COMMA or eTok_BRACE_OPEN" );
				}
			}
		}

		SToken xBegin = m_pLexer->readToken( );
		if( xBegin.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "base beginning expected eTok_BRACE_OPEN" );
		}

		_pushAndParse( pNode );

		SToken xEnd = m_pLexer->readToken( );
		if( xEnd.iType != eTok_BRACE_CLOSE ) {
			throw ParseTokException( xEnd, "base ending expected eTok_BRACE_CLOSE" );
		}

		SToken xTerminator = m_pLexer->readToken( );
		if( xTerminator.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xEnd, "base terminator expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool parseList( )
	{
		SToken xKeyword = m_pLexer->readToken();
		if( xKeyword.iType != eTok_KEY_LIST ) {
			throw ParseTokException( xKeyword, "list keyword expected eTok_KEY_LIST" );
		}

		SToken xName = m_pLexer->readToken();
		if( xName.iType != eTok_LITERAL ) {
			throw ParseTokException( xName, "list name expected eTok_LITERAL" );
		}

		PTList* pNode = new PTList( );
		pNode->set_linenum( xKeyword.iLineNum );
		pNode->set_parent( m_pCurNode );
		pNode->set_name( xName.sText );

		SToken xToken = m_pLexer->peekToken( );
		if( xToken.iType != eTok_SEPERATOR && xToken.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "list expected eTok_BRACE_OPEN or eTok_SEPERATOR" );
		}

		if( xToken.iType == eTok_SEPERATOR ) {

			SToken xSeperator = m_pLexer->readToken( );
			// Already validated above

			while( true ) {
				SToken xIName = m_pLexer->readToken( );
				if( xIName.iType != eTok_LITERAL ) {
					throw ParseTokException( xToken, "list inherit name expected eTok_LITERAL" );
				}

				pNode->add_inherit( xIName.sText );

				SToken xNextSep = m_pLexer->peekToken( );
				if( xNextSep.iType == eTok_COMMA ) {
					// Skip the token and continue
					m_pLexer->readToken( );
				} else if( xNextSep.iType == eTok_BRACE_OPEN ) {
					// Stop gathering inheritances!
					break;
				} else {
					throw ParseTokException( xNextSep, "list expected eTok_COMMA or eTok_BRACE_OPEN" );
				}
			}
		}

		SToken xBegin = m_pLexer->readToken( );
		if( xBegin.iType != eTok_BRACE_OPEN ) {
			throw ParseTokException( xToken, "list beginning expected eTok_BRACE_OPEN" );
		}

		_pushAndParse( pNode );

		SToken xEnd = m_pLexer->readToken( );
		if( xEnd.iType != eTok_BRACE_CLOSE ) {
			throw ParseTokException( xEnd, "list ending expected eTok_BRACE_CLOSE" );
		}

		SToken xTerminator = m_pLexer->readToken( );
		if( xTerminator.iType != eTok_TERMINATOR ) {
			throw ParseTokException( xEnd, "list terminator expected eTok_TERMINATOR" );
		}

		m_pCurNode->add_child( pNode );

		return true;
	}

	bool _pushAndParse( PTContainer *pNode ) 
	{
		PTContainer *pLastNode = m_pCurNode;
		m_pCurNode = pNode;
		bool bRetVal = _parse( );
		m_pCurNode = pLastNode;
		return bRetVal;
	}

	bool _parse( )
	{
		while( true ) {
			SToken xToken = m_pLexer->peekToken( );

			if( xToken.iType == eTok_EOF || xToken.iType == eTok_BRACE_CLOSE ) {
				break;
			}

			if( xToken.iType == eTok_LITERAL ) {
				parseVar( );
			} else if( xToken.iType == eTok_KEY_ENUM ) {
				parseEnum( );
			} else if( xToken.iType == eTok_KEY_MESSAGE ) {
				parseMessage( );
			} else if( xToken.iType == eTok_KEY_NAMESPACE ) {
				parseNamespace( );
			} else if( xToken.iType == eTok_KEY_TYPEDEF ) {
				parseTypedef( );
			} else if( xToken.iType == eTok_KEY_BASE ) {
				parseBase( );
			} else if( xToken.iType == eTok_KEY_LIST ) {
				parseList( );
			} else {
				throw ParseTokException( xToken, "unexpected token" );
			}

		}

		return true;
	}

	bool parse( )
	{
		_parse( );

		SToken xToken = m_pLexer->readToken( );
		if( xToken.iType != eTok_EOF ) {
			throw ParseTokException( xToken, "unexpected end-of-file" );
		}

		return true;
	}

	PTRoot* get_root( ) const { return m_pRoot; }

	void _dbgTabs( int iLvl ) {
		for( int i = 0; i < iLvl; ++i ) printf( "  " );
	}

	void _dbgOutEnum( PTEnum* pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Name: '%s'\n", pNode->get_name().c_str() );
		_dbgTabs(iLvl); printf( "Values:\n" );
		for( auto i = pNode->get_values().begin(); i != pNode->get_values().end(); ++i ) {
			_dbgTabs(iLvl+1); printf( "'%s'\n", (*i).c_str() );
		}
	}

	void _dbgOutTypedef( PTTypedef* pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Name: '%s'\n", pNode->get_name().c_str() );
		_dbgTabs(iLvl); printf( "Value: '%s'\n", pNode->get_type().c_str() );
	}

	void _dbgOutVar( PTVar* pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Name: '%s'\n", pNode->get_name().c_str() );
		_dbgTabs(iLvl); printf( "Value: '%s'\n", pNode->get_type().c_str() );
		_dbgTabs(iLvl); printf( "ArrLen: '%s'\n", pNode->get_arrlen().c_str() );
	}

	void _dbgOutContainer( PTContainer *pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Children:\n" );
		for( auto i = pNode->get_children().begin(); i != pNode->get_children().end(); ++i ) {
			_dbgOut( *i, iLvl + 1 );
		}
	}

	void _dbgOutMsgNBase( PTXMsgNBase *pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Name: '%s'\n", pNode->get_name().c_str() );
		_dbgTabs(iLvl); printf( "Inherits:\n" );
		for( auto i = pNode->get_inherits().begin(); i != pNode->get_inherits().end(); ++i ) {
			_dbgTabs(iLvl+1); printf( "'%s'\n", (*i).c_str() );
		}
		_dbgOutContainer( pNode, iLvl );
	}

	void _dbgOutMessage( PTMessage *pNode, int iLvl )
	{
		_dbgOutMsgNBase( pNode, iLvl );
	}

	void _dbgOutBase( PTBase *pNode, int iLvl )
	{
		_dbgOutMsgNBase( pNode, iLvl );
	}

	void _dbgOutList( PTList *pNode, int iLvl )
	{
		_dbgOutMsgNBase( pNode, iLvl );
	}

	void _dbgOutRoot( PTRoot *pNode, int iLvl )
	{
		_dbgOutContainer( pNode, iLvl );
	}

	void _dbgOutNamespace( PTNamespace *pNode, int iLvl )
	{
		_dbgTabs(iLvl); printf( "Name: '%s'\n", pNode->get_name().c_str() );

		_dbgOutContainer( pNode, iLvl );
	}

	void _dbgOut( PTElement *pNode, int iLvl = 0 )
	{
		_dbgTabs(iLvl); printf( "['%s'\n", ePT_Names[pNode->type()] );
		iLvl++;

#define LAZYMAN(x) if( pNode->type() == ePT_##x ) { _dbgOut##x((PT##x*)pNode,iLvl); } 
		LAZYMAN(Root)
		else LAZYMAN(Enum)
		else LAZYMAN(Typedef)
		else LAZYMAN(Namespace)
		else LAZYMAN(Message)
		else LAZYMAN(Base)
		else LAZYMAN(List)
		else LAZYMAN(Var)
#undef LAZYMAN

		iLvl--;
		_dbgTabs(iLvl); printf( "]\n" );
	}

	void dbgOutput( )
	{
		_dbgOut( get_root() );
	}
};