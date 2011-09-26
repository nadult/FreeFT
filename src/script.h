#ifndef NEWBORN_SCRIPT_H
#define NEWBORN_SCRIPT_H

#include "base.h"
#include <map>

namespace script {
	class Node;
	typedef Ptr <Node> PNode;

	class Node : public RefCounter {
public:
		enum Type
		{
			tGroup,
			tInteger,
			tFloat,
			tString,
		};

		Node(Node *p, string n, Type t)                                   : parent(p), name(n), type(t)
		{
		}

		Node(Node *p, string n, const vector <PNode>&v)   : parent(p), name(n), type(tGroup), nodes(v)
		{
		}

		Node(Node *p, string n, int v)                                    : parent(p), name(n), type(tInteger), i(v)
		{
		}

		Node(Node *p, string n, float v)                                  : parent(p), name(n), type(tFloat), f(v)
		{
		}

		Node(Node *p, string n, string v)                                 : parent(p), name(n), type(tString), s(v)
		{
		}

		void SetName(const string&nm)
		{
			name = nm;
		}

		const string&Name() const
		{
			return name;
		}

		void SetParent(Node *p)
		{
			parent = p;
		}

		Node *Parent() const
		{
			return parent;
		}

		void SetNodes(const vector <PNode>&n)
		{
			nodes = n;
		}

		inline uint NumSubNodes() const
		{
			return nodes.size();
		}

		inline PNode SubNode(uint n) const
		{
			return nodes[n];
		}

		void SetType(Type t)
		{
			type = t;
		}

		inline Type GetType() const
		{
			return type;
		}

		void SetString(const string&str)
		{
			s = str;
		}

		inline string GetString() const
		{
			return type == tString ? s : "";
		}

		void SetInt(int v)
		{
			i = v;
		}

		inline int GetInt() const
		{
			return type == tInteger ? i : type == tFloat ? f : 0;
		}

		void SetFloat(int v)
		{
			f = v;
		}

		inline float GetFloat() const
		{
			return type == tInteger ? i : type == tFloat ? f : 0;
		}

private:
		string name;
		Type   type;
		Node   *parent;

		int            i;
		float          f;
		string         s;
		vector <PNode> nodes;
	};

	class Parser {
public:
		inline Parser(PNode n) : node(n)
		{
		}

		Parser operator[](int numBack);
		Parser operator[](const string&name);
		Parser&operator()(const string&name, string&value);
		Parser&operator()(const string&name, int&value);
		Parser&operator()(const string&name, float&value);

private:
		string ScriptName() const;

		PNode node;
	};

	class Script : public Resource {
public:
		inline Script()
		{
		}

		Script(const char *fileName);

		void Serialize(Serializer&);

		inline Parser Parse()
		{
			return Parser(root);
		}

		PNode root;
	};

// Class T must implement method: void Serialize(script::Parser parser);
	template <class T>
	class ConfigMgr {
public:
		ConfigMgr(const string&fName) : fileName(fName), loaded(0)
		{
		}

		Ptr <T> operator[](const string&name)
		{
			if(!loaded)
				Load();

			typename std::map <string, Ptr <T> >::iterator it;
			it = dict.find(name);
			if(it == dict.end())
				ThrowException("Configuration for object type ", name, " not found.");

			return it->second;
		}

private:
		void Load()
		{
			dict.clear();
			Script script;
			Loader(fileName) & script;
			PNode root = script.root;
			for(int n = 0; n < root->NumSubNodes(); n++) {
				PNode node = root->SubNode(n);

				Ptr <T> obj = new T;
				obj->name = node->Name();
				obj->Serialize(Parser(node));
				dict[node->Name()] = obj;
			}
			loaded = 1;
		}

		string fileName;
		bool   loaded;
		std::map <string, Ptr <T> > dict;
	};
}

#endif
