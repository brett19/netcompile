#pragma once

#include <list>
#include "IdlParser.h"

class ResolveException : public std::exception
{
protected:
	PTElement *m_pNode;

public:
	ResolveException( PTElement *pNode, const char* pcText )
		: std::exception( pcText ), m_pNode(pNode)
	{
	}

	PTElement *node() const { return m_pNode; }
};

enum eFlags
{
	eFlag_AllowRoot			= 1 << 0,
	eFlag_AllowTypedef		= 1 << 1,
	eFlag_AllowNamespace	= 1 << 2,
	eFlag_AllowMessage		= 1 << 3,
	eFlag_AllowBase			= 1 << 4,
	eFlag_AllowList			= 1 << 5,
	eFlag_AllowVar			= 1 << 6,
	eFlag_AllowEnum			= 1 << 7
};
typedef unsigned int eFlag;

static const int FLAGS_ROOT = eFlag_AllowTypedef | eFlag_AllowBase | eFlag_AllowMessage | eFlag_AllowNamespace | eFlag_AllowEnum;
static const int FLAGS_NAMESPACE = eFlag_AllowTypedef | eFlag_AllowBase | eFlag_AllowMessage | eFlag_AllowNamespace | eFlag_AllowEnum;
static const int FLAGS_MESSAGE = eFlag_AllowVar | eFlag_AllowList;
static const int FLAGS_BASE = eFlag_AllowVar | eFlag_AllowList;
static const int FLAGS_LIST = eFlag_AllowVar | eFlag_AllowList;

class IdlResolver
{
protected:
	PTContainer *m_pRoot;
	PTElement *m_pCurElem;

public:
	IdlResolver( PTContainer *pRoot )
		: m_pRoot(pRoot)
	{
	}

	PTXMsgNBase* findBaseNode( const std::string& sName, PTContainer *pStart = nullptr )
	{
		if( pStart == nullptr ) pStart = m_pRoot;

		for( auto i = pStart->get_children().begin(); i != pStart->get_children().end(); ++i ) {
			if( (*i)->type() != ePT_Base && (*i)->type() != ePT_List ) {
				continue;
			}
			if( (*i)->name() == sName ) {
				return (PTXMsgNBase*)(*i);
			}
		}

		PTContainer *pParent = pStart->get_parent();
		if( pParent ) {
			return findBaseNode( sName, pParent );
		}
		return nullptr;
	}

	void _inheritFollow( PTXMsgNBase *pStartNode, PTXMsgNBase *pNode, std::vector<std::string>& vAllInherit, std::vector<std::string> vInherit )
	{
		vAllInherit.push_back( pNode->name() );
		vInherit.push_back( pNode->name() );

		for( auto i = pNode->get_children().begin(); i != pNode->get_children().end(); ++i )
		{
			if( (*i)->type() == ePT_List ) {
				_inheritFollow( pStartNode, (PTList*)(*i), vAllInherit, vInherit );
			}
		}

		for( auto i = pNode->get_inherits().begin(); i != pNode->get_inherits().end(); ++i )
		{
			// Check for circle-inheritance
			for( auto j = vInherit.begin(); j != vInherit.end(); ++j ) {
				if( (*i) == (*j) ) {
					throw ResolveException( pStartNode, "object has a circular inheritance" );
				}
			}

			// Check for multi-inheritance
			for( auto j = vAllInherit.begin(); j != vAllInherit.end(); ++j ) {
				if( (*i) == (*j) ) {
					std::string sErrStr = "object has multiple inheritances of base '" + (*i) + "'";
					throw ResolveException( pStartNode, sErrStr.c_str() );
				}
			}			

			PTXMsgNBase *pElem = findBaseNode( *i, pNode );
			if( pElem ) {
				_inheritFollow( pStartNode, pElem, vAllInherit, vInherit );
			}			
		}
	}

	void _inheritCheck( PTXMsgNBase *pNode )
	{
		for( auto i = pNode->get_inherits().begin(); i != pNode->get_inherits().end(); ++i ) {
			PTXMsgNBase *pElem = findBaseNode( *i, pNode );
			if( !pElem ) {
				std::string sErrStr = "reference to non-existent base '" + (*i) + "'";
				throw ResolveException( pNode, sErrStr.c_str() );
			}
		}

		std::vector<std::string> vInherit;
		//vInherit.push_back( pNode->name() );
		std::vector<std::string> vAllInherit;
		//vAllInherit.push_back( pNode->name() );
		_inheritFollow( pNode, pNode, vAllInherit, vInherit );
	}

	void _chkMsgNBase( PTXMsgNBase *pNode, eFlag iFlags )
	{
		_inheritCheck( pNode );

		_chkContainer( pNode, iFlags );
	}

	void _chkContainer( PTContainer *pNode, eFlag iFlags )
	{
		for( auto i = pNode->get_children().begin(); i != pNode->get_children().end(); ++i ) {
			_chk( *i, iFlags );
		}
	}

	void _chkRoot( PTRoot *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowRoot) ) {
			throw ResolveException( pNode, "invalid root location" );
		}

		_chkContainer( pNode, FLAGS_ROOT );
	}

	void _chkTypedef( PTTypedef *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowTypedef) ) {
			throw ResolveException( pNode, "invalid typedef location" );
		}
	}

	
	void _chkNamespace( PTNamespace *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowNamespace) ) {
			throw ResolveException( pNode, "invalid namespace location" );
		}

		_chkContainer( pNode, FLAGS_NAMESPACE );
	}

	void _chkMessage( PTMessage *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowMessage) ) {
			throw ResolveException( pNode, "invalid message location" );
		}

		_chkMsgNBase( pNode, FLAGS_MESSAGE );
	}

	void _chkBase( PTBase *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowBase) ) {
			throw ResolveException( pNode, "invalid base location" );
		}

		_chkMsgNBase( pNode, FLAGS_BASE );
	}

	void _chkList( PTList *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowList) ) {
			throw ResolveException( pNode, "invalid list location" );
		}

		_chkMsgNBase( pNode, FLAGS_LIST );
	}

	void _chkVar( PTVar *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowVar) ) {
			throw ResolveException( pNode, "invalid var location" );
		}
	}

	void _chkEnum( PTEnum *pNode, eFlag iFlags )
	{
		if( !(iFlags & eFlag_AllowEnum) ) {
			throw ResolveException( pNode, "invalid enum location" );
		}
	}

	void _chk( PTElement *pNode, eFlag iFlags )
	{
#define LAZYMAN(x) if( pNode->type() == ePT_##x ) { _chk##x((PT##x*)pNode,iFlags); } 
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

	bool validate( )
	{
		_chk( m_pRoot, eFlag_AllowRoot );
		return true;
	}

};