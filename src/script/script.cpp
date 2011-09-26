#include "script.h"
#include "script/parser.h"
#include <memory.h>
#include <fstream>
#include <iostream>

using namespace parser;

namespace script {
	namespace {
		struct Value
		{
			Value(int v) : type(tInt), i(v) { }
			Value(const string&v) : type(tString), s(v) { }
			Value(double v) : type(tDouble), d(v) { }

			enum Type {
				tInt, tDouble, tString
			} type;

			int GetInt() const { return type == tInt ? i : type == tDouble ? int(d) : 0; }
			double GetDouble() const { return type == tDouble ? d : type == tInt ? i : 0.0; }
			string GetString() const { return type == tString ? s : ""; }

			Value operator+(const Value &other)
			{
				if(type == tInt)
					return Value(GetInt() + other.GetInt());
				if(type == tDouble)
					return Value(GetDouble() + other.GetDouble());

				return GetString() + other.GetString();
			}

			Value operator-(const Value &other)
			{
				if(type == tInt)
					return Value(GetInt() - other.GetInt());
				if(type == tDouble)
					return Value(GetDouble() - other.GetDouble());

				return GetString();
			}

			Value operator*(const Value &other)
			{
				if(type == tInt)
					return Value(GetInt() * other.GetInt());
				if(type == tDouble)
					return Value(GetDouble() * other.GetDouble());

				return GetString();
			}

			Value operator/(const Value &other)
			{
				if(type == tInt)
					return Value(GetInt() / other.GetInt());
				if(type == tDouble)
					return Value(GetDouble() / other.GetDouble());

				return GetString();
			}

			int i;
			double d;
			string s;
		};

		Value Evaluate(parser::PNode node)
		{
			switch(node->type) {
				case NT_VALUE:
					switch(node->valueType) {
					case T_INT:
					case T_BOOLEAN:
						return Value(node->i);

					case T_DOUBLE:
						return Value(node->d);

					case T_STRING:
						return Value(node->s);
					}
					break;

				case NT_ADD:
				case NT_SUB:
				case NT_MUL:
				case NT_DIV:
				{
					Value a = Evaluate(node->SubNode(0)), b = Evaluate(node->SubNode(1));
					if(a.type == Value::tString || b.type == Value::tString) {
						if(a.type != Value::tString || b.type != Value::tString || node->type != NT_ADD)
							ThrowException("Syntax error in line ", node->line);
						return Value(a + b);
					}

					if(node->type == NT_ADD)
						return Value(a + b);
					if(node->type == NT_SUB)
						return Value(a - b);
					if(node->type == NT_MUL)
						return Value(a * b);
					if(node->type == NT_DIV)
						return Value(a / b);

					break;
				}
			}

			ThrowException("Syntax error in line ", node->line);
		}

		PNode Convert(Node *parent, parser::PNode node)
		{
			switch(node->type) {
				case NT_SET:
				{
					Value val = Evaluate(node->SubNode(0));

					switch(val.type) {
					case Value::tInt:
						return new Node(parent, node->s, val.GetInt());

					case Value::tString:
						return new Node(parent, node->s, val.GetString());

					case Value::tDouble:
						return new Node(parent, node->s, (float)val.GetDouble());
					}

					break;
				}

				case NT_INSTRUCTION_LIST:
				case NT_PROGRAM:
				{
					vector <PNode> subNodes;
					PNode out = new Node(parent, node->s, Node::tGroup);
					for(int n = 0; n < node->NumSubNodes(); n++)
						subNodes.push_back(Convert(&*out, node->SubNode(n)));
					out->SetNodes(subNodes);
					return out;
				}
			}

			ThrowException("Error while parsing script: unknown node type");
		}
	}

	Script::Script(const char *fileName)
	{
		Loader(fileName) & *this;
	}

	void Script::Serialize(Serializer&sr)
	{
		if(sr.IsSaving())
			ThrowException("Script saving not avaliable for now");

		if(sr.IsLoading()) {
			vector <char> buffer;
			buffer.resize(sr.Size() + 1);
			sr.Data(&buffer[0], sr.Size());
			buffer.back() = 0;

			try {
				parser::PNode node = ParseString(&buffer[0]);
				root = Convert(0, node);
				root->SetName(sr.Name());
			}
			catch(const std::exception&ex) {
				ThrowException("Error while loading script ", sr.Name(), ":\n", ex.what());
			}
		}
	}
}
