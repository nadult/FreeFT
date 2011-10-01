#ifndef NEWBORN_SCRIPT_PARSER_H
#define NEWBORN_SCRIPT_PARSER_H

#include <baselib.h>

using namespace baselib;

namespace parser {

	enum TYPE
	{
		T_VOID				= 0,

		T_INT				= 1,
		T_DOUBLE			= 2,
		T_STRING			= 3,
		T_BOOLEAN			= 4,

		T_UNKNOWN			= 5,
	};

	enum NODE_TYPE
	{
		NT_UNKNOWN			= 0,
		NG_UNKNOWN			= 0,

		NG_EXPRESSION		= 0x010000,
		NG_BINARY_EXPR		= NG_EXPRESSION|0x0100,
		NG_UNARY_EXPR		= NG_EXPRESSION|0x0200,
		NG_INSTRUCTION		= 0x020000,
		NG_OTHER			= 0x040000,

		NT_VALUE			= NG_EXPRESSION|0x01,
		NT_FUNCTION_CALL	= NG_EXPRESSION|0x02,
		NT_IDENTIFIER		= NG_EXPRESSION|0x03,

		NT_UNARY_PLUS		= NG_UNARY_EXPR|0x01,
		NT_UNARY_MINUS		= NG_UNARY_EXPR|0x02,
		NT_NOT				= NG_UNARY_EXPR|0x03,

		NT_LOGICAL_OR		= NG_BINARY_EXPR|0x01,
		NT_LOGICAL_AND		= NG_BINARY_EXPR|0x02,
		NT_GREATER			= NG_BINARY_EXPR|0x03,
		NT_GREATER_EQUAL	= NG_BINARY_EXPR|0x04,
		NT_LESSER			= NG_BINARY_EXPR|0x05,
		NT_LESSER_EQUAL		= NG_BINARY_EXPR|0x06,
		NT_EQUAL			= NG_BINARY_EXPR|0x07,
		NT_NOT_EQUAL		= NG_BINARY_EXPR|0x08,

		NT_ADD				= NG_BINARY_EXPR|0x09,
		NT_SUB				= NG_BINARY_EXPR|0x0A,
		NT_MUL				= NG_BINARY_EXPR|0x0B,
		NT_DIV				= NG_BINARY_EXPR|0x0C,
		NT_MOD				= NG_BINARY_EXPR|0x0D,

		NT_INSTRUCTION_LIST	= NG_INSTRUCTION|0x01,
		NT_DO_EXPRESSION	= NG_INSTRUCTION|0x02,
		NT_DECLARE			= NG_INSTRUCTION|0x03,
		NT_SET				= NG_INSTRUCTION|0x04,
		NT_INCREMENT		= NG_INSTRUCTION|0x05,
		NT_DECREMENT		= NG_INSTRUCTION|0x06,
		NT_IF_ELSE			= NG_INSTRUCTION|0x07,
		NT_WHILE			= NG_INSTRUCTION|0x08,
		NT_FOR				= NG_INSTRUCTION|0x09,
		NT_RETURN			= NG_INSTRUCTION|0x0A,

		NT_PROGRAM			= NG_OTHER|0x01,
		// s nazwa valueType typ zwracany
		// kolejne: argumenty
		// ostatni: lista instr
		NT_FUNCTION			= NG_OTHER|0x02,
		// s nazwa valueType typ zwracany
		NT_FUNCTION_ARG		= NG_OTHER|0x03,
	};
	inline NODE_TYPE operator|(NODE_TYPE a,NODE_TYPE b) { return NODE_TYPE(u32(a)|u32(b)); }
	inline NODE_TYPE operator^(NODE_TYPE a,NODE_TYPE b) { return NODE_TYPE(u32(a)^u32(b)); }
	inline NODE_TYPE operator&(NODE_TYPE a,NODE_TYPE b) { return NODE_TYPE(u32(a)&u32(b)); }

	class Node;
	typedef Ptr<Node> PNode;

	class Node: public RefCounter
	{
	public:
		Node(NODE_TYPE type);
		Node(NODE_TYPE type,unsigned int i);
		Node(NODE_TYPE type,int i);
		Node(NODE_TYPE type,bool b);
		Node(NODE_TYPE type,double d);
		Node(NODE_TYPE type,std::string s);
		Node(NODE_TYPE type,std::string s,TYPE valueType);
		Node(NODE_TYPE type,PNode firstChild);
		Node(NODE_TYPE type,TYPE tType);
		Node();

		PNode SubNode(u32 idx);
		u32 NumSubNodes() const;
		void AddSubNode(PNode a);

		NODE_TYPE type;
		TYPE valueType;

		u32 line,column;

		double d;
		int i;
		std::string s;

	private:
		std::vector<PNode> subNodes;
	};

	PNode ParseString(const char *str);

}

#endif

