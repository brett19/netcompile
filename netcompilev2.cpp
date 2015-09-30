#include <iostream>
#include "IdlLexer.h"
#include "IdlParser.h"
#include "IdlResolver.h"
#include "CppGenerator.h"

int main( int argc, char* argv[] )
{
	char *pcFilename = "C:\\Users\\Brett\\Desktop\\newIdl.txt";

	FILE *fHandle = fopen( pcFilename, "rb" );

	if( !fHandle ) {
		printf( "Failed to open input file!" );
		return -1;
	}

	try {
		IdlLexer xLexer( fHandle );
		// xLexer.dbgOutput( );

		IdlParser xParser( &xLexer );
		xParser.parse( );
		//xParser.dbgOutput( );

		IdlResolver xResolver( xParser.get_root() );
		xResolver.validate( );

		CppGenerator xGen( xParser.get_root() );
		xGen.generate( );

	} catch( LexException e ) {
		printf( "%s(%d): lexer error: %s\n", pcFilename, e.line_num(), e.what() );
	} catch( ParseTokException e ) {
		printf( "%s(%d): parser error: %s -> '%s'\n", pcFilename, e.token().iLineNum, e.what(), e.token().sText.c_str() );
	} catch( ResolveException e ) {
		printf( "%s(%d): validate error: %s\n", pcFilename, e.node()->line_num(), e.what() );
	} catch( GenException e ) {
		printf( "%s(%d): validate error: %s\n", pcFilename, e.node()->line_num(), e.what() );
	}

	system( "PAUSE" );
	return 0;
}