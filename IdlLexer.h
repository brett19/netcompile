#pragma once
#pragma warning( disable: 4996 )

#include <iostream>
#include <string>
#include <vector>
#include <exception>

class LexException : public std::exception
{
protected:
	int m_iLineNum;

public:
	LexException( int iLineNum, char *pcWhat )
		: std::exception(pcWhat), m_iLineNum(iLineNum)
	{
	}

	int line_num( ) const { return m_iLineNum; }
};

enum eTok
{
	eTok_UNKNOWN = 0,
	eTok_LITERAL,
	eTok_COMMA,
	eTok_BRACE_OPEN,
	eTok_BRACE_CLOSE,
	eTok_ARR_OPEN,
	eTok_ARR_CLOSE,
	eTok_SEPERATOR,
	eTok_TERMINATOR,
	eTok_COMMENT,
	eTok_EOF,
	eTok_KEY_MESSAGE,
	eTok_KEY_BASE,
	eTok_KEY_ENUM,
	eTok_KEY_TYPEDEF,
	eTok_KEY_NAMESPACE,
	eTok_KEY_LIST
};

static char* eTok_Names[] = { 
	"UNKNOWN",
	"LITERAL", 
	"COMMA", 
	"BRACE_OPEN", 
	"BRACE_CLOSE",
	"ARR_OPEN",
	"ARR_CLOSE",
	"SEPERATOR",
	"TERMINATOR", 
	"COMMENT",
	"EOF",
	"KEY_MESSAGE",
	"KEY_BASE",
	"KEY_ENUM",
	"KEY_TYPEDEF",
	"KEY_NAMESPACE",
	"KEY_LIST"
};

struct SToken
{
	int iLineNum;
	eTok iType;
	int iValue;
	std::string sText;
};

class IdlLexer
{
protected:
	std::vector<SToken> m_asPeeks;
	FILE *m_fHandle;
	int m_iLineNum;

	bool isEof( )
	{
		return feof( m_fHandle ) != 0;
	}

	static bool isEndOfLine( char cValue ) {
		return cValue == '\n';
	}

	static bool isWhitespace( char cValue ) {
		return cValue == '\r' || cValue == '\n' || cValue == '\t' || cValue == ' ';
	}

	static bool isNonLiteral( char cValue ) {
		return cValue == '{' || cValue == '}' || cValue == '[' || cValue == ']' || cValue == ';' || cValue == ':' || cValue == ',' || cValue == '/';
	}

	static eTok getNonLiteralType( char cValue ) {
		if( cValue == '{' ) {
			return eTok_BRACE_OPEN;
		} else if( cValue == '}' ) {
			return eTok_BRACE_CLOSE;
		} else if( cValue == '[' ) {
			return eTok_ARR_OPEN;
		} else if( cValue == ']' ) {
			return eTok_ARR_CLOSE;
		} else if( cValue == ',' ) {
			return eTok_COMMA;
		} else if( cValue == ':' ) {
			return eTok_SEPERATOR;
		} else if( cValue == ';' ) {
			return eTok_TERMINATOR;
		}
		return eTok_UNKNOWN;
	}

	SToken _readToken( )
	{
		bool bInComment = false;
		bool bCommentStarted = false;
		bool bTokenStarted = false;
		SToken xToken;

		while( !isEof() ) {
			int cValue = fgetc( m_fHandle );
			if( cValue == EOF ) continue;

			// Update line number info
			if( cValue == '\n' ) {
				m_iLineNum++;
			}

			// +++ Ignore Windows Fail Newlines
			if( cValue == '\r' ) continue;
			// ---

			// +++ Handle Comments
			if( !bInComment ) {
				if( cValue == '/' ) {
					char cValue2 = fgetc( m_fHandle );
					if( cValue2 == '/' ) {
						bInComment = true;
						continue;
					} else {
						throw LexException( m_iLineNum, "unexpected '/' literal" );
					}
				}
			} else {
				if( isEndOfLine(cValue) ) {
					bInComment = false;
				} else {
					continue;
				}
			}		
			/* TOKENIZE COMMENTS
			if( !bInComment ) {
				if( cValue == '/' ) {
					char cValue2 = fgetc( m_fHandle );
					if( cValue2 == '/' ) {
						bInComment = true;
						xToken.iType = eTok_COMMENT;
						continue;
					} else {
						throw LexException( m_iLineNum, "unexpected '/' literal" );
					}
				}
			} else {
				if( isEndOfLine(cValue) ) {
					return xToken;
				}

				if( !bCommentStarted ) {
					if( !isWhitespace(cValue) ) {
						bCommentStarted = true;
					} else {
						continue;
					}
				}

				xToken.sText.push_back( cValue );				
				continue;
			}
			*/
			// --- Handle Comments

			// +++ Don't Lex Whitespace +++
			if( bTokenStarted ) {
				if( isWhitespace(cValue) ) {
					break;
				} else if( isNonLiteral(cValue) ) {
					fseek( m_fHandle, -1, SEEK_CUR );
					break;
				}
			} else {
				if( isWhitespace(cValue) ) {
					continue;
				} else {
					bTokenStarted = true;
					xToken.iLineNum = m_iLineNum;
				}
			}
			// --- Don't Lex Whitespace ---

			if( isNonLiteral(cValue) ) {
				xToken.iType = getNonLiteralType( cValue );
				xToken.sText.push_back( cValue );
				return xToken;
			}

			xToken.iType = eTok_LITERAL;
			xToken.sText.push_back( cValue );
		}

		if( isEof() && xToken.sText.size() == 0 ) {
			xToken.iType = eTok_EOF;
		}

		if( xToken.iType == eTok_UNKNOWN ) {
			throw LexException( m_iLineNum, "bad token" );
		}

		eTok iNewType = eTok_UNKNOWN;
		if( xToken.sText == "message" ) {
			iNewType = eTok_KEY_MESSAGE;
		} else if( xToken.sText == "base" ) {
			iNewType = eTok_KEY_BASE;
		} else if( xToken.sText == "enum" ) {
			iNewType = eTok_KEY_ENUM;
		} else if( xToken.sText == "namespace" ) {
			iNewType = eTok_KEY_NAMESPACE;
		} else if( xToken.sText == "@type" ) {
			iNewType = eTok_KEY_TYPEDEF;
		} else if( xToken.sText == "list" ) {
			iNewType = eTok_KEY_LIST;
		}

		if( iNewType != eTok_UNKNOWN ) {
			if( xToken.iType != eTok_LITERAL ) {
				throw LexException( m_iLineNum, "invalid use of language keyword" );
			}
			xToken.iType = iNewType;
		}

		return xToken;
	}

public:
	IdlLexer( FILE *fHandle ) 
		: m_fHandle(fHandle), m_iLineNum(1)
	{
	}

	SToken readToken( )
	{
		if( m_asPeeks.size() >= 1 ) {
			SToken xToken = m_asPeeks.back( );
			m_asPeeks.pop_back( );
			return xToken;
		}

		SToken xToken = _readToken( );

		//printf( "% 14s - %s\n", eTok_Names[xToken.iType], xToken.sText.c_str() );

		return xToken;
	}

	SToken peekToken( )
	{
		SToken xToken = readToken( );
		m_asPeeks.push_back( xToken );
		return xToken;
	}

	bool dbgOutput( )
	{
		while( !isEof() ) {
			SToken xToken = readToken( );
			printf( "% 14s - %s\n", eTok_Names[xToken.iType], xToken.sText.c_str() );
		}

		return true;
	}

};