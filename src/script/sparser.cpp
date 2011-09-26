#include "script.h"

namespace script {

	Parser Parser::operator[](int numBack) {
		PNode parent=node;
		while(numBack<0) {
			numBack++;
			parent=parent->Parent();
			if(!parent)
				ThrowException("Error while parsing script ",ScriptName(),": trying to leave top-group");
		}
		return Parser(parent);
	}
	Parser Parser::operator[](const string &name) {
		for(int n=0;n<node->NumSubNodes();n++) {
			PNode subNode=node->SubNode(n);
			if(subNode->Name()==name)
				return Parser(subNode);
		}

		ThrowException("Error while parsing script ",ScriptName(),": node '",name,"' not found in node '",node->Name(),'\'');
		return *this;
	}

	Parser &Parser::operator()(const string &name,string &value) {
		PNode node=(operator[](name)).node;
		value=node->GetString();
		return *this;
	}
	Parser &Parser::operator()(const string &name,int &value) {
		PNode node=(operator[](name)).node;
		value=node->GetInt();
		return *this;
	}
	Parser &Parser::operator()(const string &name,float &value) {
		PNode node=(operator[](name)).node;
		value=node->GetFloat();
		return *this;
	}

	string Parser::ScriptName() const {
		PNode parent=node;
		while(parent->Parent())
			parent=parent->Parent();
		return parent->Name();
	}

}

