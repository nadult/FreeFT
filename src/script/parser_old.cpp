#include <limits.h>
#include <string.h>
#include <stack>
#include <typeinfo>
#include <boost/type_traits.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/utility.hpp>
#include <boost/spirit/actor.hpp>
#include <boost/spirit/phoenix.hpp>
#include <boost/spirit/phoenix/primitives.hpp>
#include <boost/spirit/phoenix/operators.hpp>
#include <boost/spirit/phoenix/functions.hpp>

#include "script/parser.h"

using namespace std;
using namespace boost::spirit;
using namespace boost::phoenix;

namespace parser
{
	namespace phoenix = boost::phoenix;

	class NodeStack
	{
		std::stack<PNode> data;

	public:
		void Push(PNode);
		void Push(NODE_TYPE);
		template <class T> void Push(NODE_TYPE type,const T &t) { Push(new Node(type,t)); }
		template <class T1,class T2> void Push(NODE_TYPE type,const T1 &t1,const T2 &t2) { Push(new Node(type,t1,t2)); }
		PNode Pop();
		PNode Top();
		u32 Size();
		bool Empty();
		void Free();
	};

	namespace
	{
		struct TypeInfo
		{
				TYPE id;		const char *str;	u32 size;
		}	const types[]=
		{
			{	T_VOID,			"void",				0 },

			{	T_INT,			"int",				4 },
			{	T_DOUBLE,		"double",			8 },
			{	T_STRING,		"string",			4 },
			{	T_BOOLEAN,		"boolean",			4 },

			{	T_UNKNOWN,		0,					0 },
		};

		typedef classic::position_iterator<char const*> Iterator;
		Iterator *tFirstIter;

		u32 GetLine() { return tFirstIter?tFirstIter->get_position().line:-1; }
		u32 GetColumn() { return tFirstIter?tFirstIter->get_position().column:-1; }

		NodeStack nodes;
		string lastIdentifier;
		TYPE lastType;
		u32 errorsNum;

		void JoinOpA(Iterator ,Iterator )
			{ PNode val=nodes.Pop(); nodes.Top()->AddSubNode(val); }
		void JoinOpAB(Iterator ,Iterator )
			{ PNode val2=nodes.Pop(),val1=nodes.Pop(); nodes.Top()->AddSubNode(val1); nodes.Top()->AddSubNode(val2); }
		void JoinAOpB(Iterator ,Iterator )
			{ PNode b=nodes.Pop(),op=nodes.Pop(),a=nodes.Pop(); op->AddSubNode(a); op->AddSubNode(b); nodes.Push(op); }

		struct PushImpl {
			template <class I1,class I2=void,class I3=void> struct result { typedef void type; };
			template <class I> void operator()(I const& i) const { nodes.Push(i); }
			template <class I1,class I2> void operator()(I1 const& i1,I2 const& i2) const { nodes.Push(i1,i2); }
			template <class I1,class I2,class I3> void operator()(I1 const& i1,I2 const& i2,I3 const& i3) const { nodes.Push(i1,i2,i3); }
		}; 
		struct PopImpl { typedef PNode result_type; PNode operator()() const { return nodes.Pop(); } }; 
		struct TopImpl { typedef PNode result_type; PNode operator()() const { return nodes.Top(); } };
		
		phoenix::function<PushImpl> const Push = PushImpl();
		phoenix::function<PopImpl> const Pop = PopImpl();
		phoenix::function<TopImpl> const Top = TopImpl();

		struct HandleError
		{
			template <typename ScannerT, typename ErrorT>
			classic::error_status<>
				operator()(ScannerT const&, ErrorT const& error) const
			{
				cout << "Error on line "<<error.where.get_position().line<<" (col: "<<error.where.get_position().column<<"): "
					<<error.descriptor<<endl;
				errorsNum++;
				return classic::error_status<>::fail;
			}
		};

		struct Types: classic::symbols<TYPE>
			{ Types() { for(u32 n=0;types[n].str;n++) add(types[n].str,types[n].id); } } types_p;

		struct Javalette: public classic::grammar<Javalette>
		{
			#define EXPECT( op , errMsg )		classic::assertion<const char*>(errMsg " expected") ( op )
			#define EXPECTC( chr )				EXPECT(ch_p(chr), #chr )
			#define EXPECTT( op )				EXPECT(op, #op )

			template <class ScannerT> class definition { public:
			definition(const Javalette&self)
			{
				first = programGuard(program)[HandleError()];

				program = 
					( instrList ) >> EXPECT( end_p, "instructions list or end of file");
				//	*( funcDef[&JoinOpA] ) >> EXPECT( end_p, "function definition or end of file");

				type =
					types_p[var(lastType)=arg1];

				keyword =
					type | chseq_p("for") | chseq_p("if") | chseq_p("else") | chseq_p("while") | chseq_p("return") |
						   chseq_p("true") | chseq_p("false");

				identifier =
					((lexeme_d [ (alpha_p | ch_p('_')) >> *( alnum_p | ch_p('_')) ]) - keyword)
						[var(lastIdentifier)=construct_<string>(arg1,arg2)];

				exprSimple =
					  strict_real_p[Push(NT_VALUE,arg1)]
					| int_p[Push(NT_VALUE,arg1)]
					| confix_p('"', (*c_escape_ch_p)[Push(NT_VALUE,construct_<string>(arg1,arg2))], '"')
					|	(  ch_p('(')
						>> EXPECTT( expression )
						>> EXPECTC( ')' ) )

					//| ( identifier[Push(NT_IDENTIFIER,var(lastIdentifier))]
					//	>> !(ch_p('(')[Pop(),Push(NT_FUNCTION_CALL,var(lastIdentifier))] >>
					//		!(expression[&JoinOpA] >> *( ch_p(',') >> EXPECTT( expression )[&JoinOpA] ))
					//	>> EXPECTC( ')' ) )
					//  )
					| chseq_p("true")[Push(NT_VALUE,true)]
					| chseq_p("false")[Push(NT_VALUE,false)];

				exprUnary =
					  exprSimple
					| ((
						  chseq_p("!")[Push(NT_NOT)]
						| chseq_p("+")[Push(NT_UNARY_PLUS)]
						| chseq_p("-")[Push(NT_UNARY_MINUS)]
						) >> EXPECTT( exprSimple )[&JoinOpA] );

				exprMul =
					exprUnary >> *( ((
						  chseq_p("*")[Push(NT_MUL)]
						| chseq_p("%")[Push(NT_MOD)]
						| chseq_p("/")[Push(NT_DIV)]
					) >> EXPECTT( exprUnary ))[&JoinAOpB] );

				exprAddition =
					exprMul >> *( ((
						  chseq_p("+")[Push(NT_ADD)]
						| chseq_p("-")[Push(NT_SUB)]
					) >> EXPECTT( exprMul ))[&JoinAOpB] );
				
				exprRelation =
					exprAddition >> *( ((
						  chseq_p("<=")[Push(NT_LESSER_EQUAL)]
						| chseq_p(">=")[Push(NT_GREATER_EQUAL)]
						| chseq_p("<" )[Push(NT_LESSER)]
						| chseq_p(">" )[Push(NT_GREATER)]
					) >> EXPECTT( exprAddition ) )[&JoinAOpB] );

				exprCompare =
					exprRelation >> *( ((
						  chseq_p("==")[Push(NT_EQUAL)]
						| chseq_p("!=")[Push(NT_NOT_EQUAL)]
					) >> EXPECTT( exprRelation ) )[&JoinAOpB] );

				exprLogicalAnd	=
					exprCompare >> *(
						   chseq_p("&&")[Push(NT_LOGICAL_AND)]
						>> EXPECTT( exprCompare )[&JoinAOpB] );

				expression =
					   exprLogicalAnd
					>> *(chseq_p("||")[Push(NT_LOGICAL_OR)]
						>> EXPECTT( exprLogicalAnd ) [&JoinAOpB]);

			/*	declarator =
					   identifier[Push(NT_IDENTIFIER,var(lastIdentifier))]
					>> !(ch_p('=')
					>> EXPECTT( expression ) [&JoinOpA]),

				instrDeclare =
					   type[Push(NT_DECLARE,var(lastType))]
					>> EXPECTT( declarator ) [&JoinOpA]
					>> *( ch_p(',')
					>> EXPECTT( declarator )[&JoinOpA] ), */

				instrList =
					 *( instruction[&JoinOpA] ),

				instrSet		=
					identifier >> (
						( chseq_p("=")[Push(NT_SET,var(lastIdentifier))]
							>> EXPECT( expression[&JoinOpA], "expression" ) )
						| chseq_p("++")[Push(NT_INCREMENT,var(lastIdentifier))]
						| chseq_p("--")[Push(NT_DECREMENT,var(lastIdentifier))]
						| (  ch_p('{')[Push(NT_INSTRUCTION_LIST,var(lastIdentifier))]
							>> EXPECTT( instrList )
							>> EXPECTC( '}' ) ) );

				instruction = instrSet;
							/* instrGuard(
						(  ch_p('{')[Push(NT_INSTRUCTION_LIST)]
						>> EXPECTT( instrList )
						>> EXPECTC( '}' ) )

					|   (( chseq_p("if")[Push(NT_IF_ELSE)]
						>> EXPECTC( '(' )
						>> EXPECTT( expression )
						>> EXPECTC( ')' )
						>> EXPECTT( instruction ) [&JoinOpAB] )
						>> !( chseq_p("else")
							>> EXPECTT( instruction ) [&JoinOpA] ) )

					|   (  chseq_p("while")[Push(NT_WHILE)]
						>> EXPECTC( '(' )
						>> EXPECTT( expression )
						>> EXPECTC( ')' )
						>> instruction[&JoinOpAB] )

					|	(  chseq_p("for")[Push(NT_FOR)]
						>> EXPECTC( '(' )
						>> EXPECTT( instrSet )
						>> EXPECTC( ';' )
						>> EXPECTT( expression ) [&JoinOpAB]
						>> EXPECTC( ';' )
						>> EXPECTT( instrSet )
						>> EXPECTC( ')' )
						>> EXPECTT( instruction ) [&JoinOpAB] )

					|	(  chseq_p("return")
						>> (
								  ch_p(';')[Push(NT_RETURN)]
								| (expression >> ';')[Push(NT_RETURN,Pop())]
							) )

					|	(  instrSet
						>> EXPECTC( ';' ) )

					|   (  instrDeclare
						>> EXPECTC( ';' ) )

					|   ( expression[Push(NT_DO_EXPRESSION,Pop())] >> ';' )
					)[HandleError()]; */

		/*		funcArg =
					   type
					>> EXPECT ( identifier[Push(NT_FUNCTION_ARG,var(lastIdentifier),var(lastType))], "argument name");

				funcDef =
					funcGuard(
						(  type
						>> EXPECT ( identifier, "function name") [Push(NT_FUNCTION,var(lastIdentifier),var(lastType))]
						>> EXPECTC( '(' )
						>> !(
							   funcArg [&JoinOpA]
							>> *(
								   ch_p(',')
								>> EXPECTT( funcArg ) [&JoinOpA]
							)
						)
						>> EXPECTC( ')' )
						>> EXPECTC( '{' ) [Push(NT_INSTRUCTION_LIST)]
						>> EXPECTT( instrList )
						>> EXPECTC( '}' ) ) [&JoinOpA]
					)[HandleError()]; */
			 }

			classic::rule<ScannerT> identifier,type,keyword;
			classic::rule<ScannerT> expression,exprLogicalAnd,exprCompare,exprRelation,exprAddition,exprMul,exprUnary,exprSimple;
			classic::rule<ScannerT> instruction,instrList,instrDeclare,declarator,instrSet;
			classic::rule<ScannerT> first,program,funcDef,funcArg;

			classic::guard<const char*> programGuard,funcGuard,instrGuard;
			classic::rule<ScannerT> const& start() const
			{
				nodes.Free(); nodes.Push(new Node(NT_PROGRAM));
				return first;
			}

			};

			#undef EXPECT
			#undef EXPECTT
			#undef EXPECTC
		};

		template <class SkipParser>
		bool DoParse(const char *str,u32 charsPerTab,boost::spirit::parser<SkipParser> const &skip)
		{
			Javalette javalette;

			typedef skip_parser_iteration_policy<SkipParser> iter_policy_t;
			typedef scanner_policies<iter_policy_t> scanner_policies_t;
			typedef scanner<Iterator, scanner_policies_t> scanner_t;

			Iterator begin(str, str+strlen(str)),end;
			begin.set_tabchars(charsPerTab);

			iter_policy_t iter_policy(skip.derived());
			scanner_policies_t policies(iter_policy);
			scanner_t scan(begin, end, policies);

			tFirstIter=&scan.first;
			match<boost::spirit::nil_t> hit = javalette.parse(scan);
			scan.skip(scan);
			tFirstIter=0;

			return hit&&(begin==end);
		}

	}

	PNode ParseString(const char *str) {
		enum { charsPerTab=4 };

		errorsNum=0;
		if(!DoParse(str,charsPerTab,space_p|comment_p("//")|comment_p('#')|comment_p("/*","*/"))||nodes.Size()!=1)
			errorsNum++;

		if(!errorsNum) {
			PNode top=nodes.Pop();
			nodes.Free();
			return top;
		}

		nodes.Free();
		ThrowException("Error while parsing string");
		return 0;
	}

	void NodeStack::Push(PNode node)
		{ data.push(node); }
	void NodeStack::Push(NODE_TYPE nodeType)
		{ data.push(new Node(nodeType)); }
	PNode NodeStack::Pop()
		{ PNode out=data.top(); data.pop(); return out; }
	PNode NodeStack::Top()
		{ PNode out=data.top(); return out; }
	u32 NodeStack::Size()
		{ return u32(data.size()); }
	bool NodeStack::Empty()
		{ return data.empty(); }
	void NodeStack::Free()
		{ while(!data.empty()) data.pop(); }



	Node::Node(NODE_TYPE tType,PNode firstChild)
		:i(0),d(0),type(tType),line(GetLine()),column(GetColumn()) { AddSubNode(firstChild); }
	Node::Node(NODE_TYPE tType)
		:i(0),d(0),type(tType),line(GetLine()),column(GetColumn()) { }
	Node::Node()
		:i(0),d(0),type(NT_UNKNOWN),line(GetLine()),column(GetColumn()) { }

	Node::Node(NODE_TYPE tType,std::string tSValue,TYPE tSType)
		:i(0),d(0),s(tSValue),type(tType),valueType(tSType),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,int tIValue)
		:i(tIValue),d(0),type(tType),valueType(T_INT),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,unsigned int tIValue)
		:i(tIValue),d(0),type(tType),valueType(T_INT),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,bool tIValue)
		:i(tIValue),d(0),type(tType),valueType(T_BOOLEAN),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,double tDValue)
		:i(0),d(tDValue),type(tType),valueType(T_DOUBLE),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,std::string tSValue)
		:i(0),d(0),s(tSValue),type(tType),valueType(T_STRING),line(GetLine()),column(GetColumn()) { }
	Node::Node(NODE_TYPE tType,TYPE vType)
		:i(0),d(0),type(tType),valueType(vType),line(GetLine()),column(GetColumn()) { }
	PNode Node::SubNode(u32 idx)
		{ if(idx>=subNodes.size()) return 0; return subNodes[idx]; }
	u32 Node::NumSubNodes() const
		{ return u32(subNodes.size()); }	
	void Node::AddSubNode(PNode a)
		{ subNodes.push_back(a); }

}
