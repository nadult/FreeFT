#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "script/parser.h"

using namespace script;

namespace
{
	namespace fusion = boost::fusion;
	namespace phoenix = boost::phoenix;
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;

	struct mini_xml;

	typedef boost::variant<boost::recursive_wrapper<mini_xml>, std::string> mini_xml_node;

	struct mini_xml
	{
		std::string name;
		std::vector<mini_xml_node> children;
	};

	struct String {
		String() :size(0), start(0) { }

		void Add(char c, vector<char> &buffer) {
			if(size == 0)
				start = (int)buffer.size();

			buffer.push_back(c);
			size++;
		}

		int start, size;
	};

	struct Tuple {
		enum { maxSize = 32 };

		Tuple() :size(0) { }

		void AddElement(const Variant &elem) {
			InputAssert(size < maxSize);
			elements[size++] = elem;
		}

		Variant elements[maxSize];
		int size;
	};
}

// We need to tell fusion about our mini_xml struct
// to make it a first-class fusion citizen
//[tutorial_xml1_adapt_structures
BOOST_FUSION_ADAPT_STRUCT(
	mini_xml,
	(std::string, name)
	(std::vector<mini_xml_node>, children)
)
//]

namespace
{
		
	phoenix::function<PushImpl> const Push = PushImpl();
	phoenix::function<PopImpl> const Pop = PopImpl();
	phoenix::function<TopImpl> const Top = TopImpl();

	///////////////////////////////////////////////////////////////////////////
	//  Our mini XML grammar definition
	///////////////////////////////////////////////////////////////////////////
	//[tutorial_xml1_grammar
	template <typename Iterator>
	struct mini_xml_grammar : qi::grammar<Iterator, mini_xml(), ascii::space_type>
	{
		vector<char> stringBuffer;

		void operator+=(char c) { }

		mini_xml_grammar() : mini_xml_grammar::base_type(xml)
		{
			using qi::lit;
			using qi::lexeme;
			using ascii::char_;
			using ascii::string;
			using namespace qi::labels;

			using phoenix::at_c;
			using phoenix::push_back;

			text = lexeme[+(char_ - '<')		[_val += _1]];
			node = (xml | text)				 [_val = _1];

			vString = lexeme['"' >> *(char_ - '"') [_T += _1] >> '"'];

			start_tag =
					'<'
				>>  !lit('/')
				>>  lexeme[+(char_ - '>')	   [_val += _1]]
				>>  '>'
			;

			end_tag =
					"</"
				>>  string(_r1)
				>>  '>'
			;

			xml =
					start_tag				   [at_c<0>(_val) = _1]
				>>  *node					   [push_back(at_c<1>(_val), _1)]
				>>  end_tag(at_c<0>(_val))
			;
		}

		qi::rule<Iterator, String(), ascii::space_type> vString;
		qi::rule<Iterator, Variant(), ascii::space_type> value;

		qi::rule<Iterator, mini_xml(), ascii::space_type> xml;
		qi::rule<Iterator, mini_xml_node(), ascii::space_type> node;
		qi::rule<Iterator, std::string(), ascii::space_type> text;
		qi::rule<Iterator, std::string(), ascii::space_type> start_tag;
		qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
	};
	//]
}

bool ParseTree(const char *data, int dataSize, mini_xml &out) {
	using boost::spirit::ascii::space;
	const char *iter = data;

	mini_xml_grammar<const char*> grammar;
	bool ret = phrase_parse(iter, data + dataSize, grammar, space, out);
	return ret && iter == data + dataSize;
}

namespace script
{



}
