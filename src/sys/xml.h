#ifndef SYS_XML_H
#define SYS_XML_H

#include "base.h"

#ifndef RAPIDXML_HPP_INCLUDED

namespace rapidxml
{
	template <class Ch = char> class xml_node;
	template <class Ch = char> class xml_document;
};

#endif

class XMLNode {
public:
	XMLNode(const XMLNode &rhs) :m_ptr(rhs.m_ptr), m_doc(rhs.m_doc) { }

	// If an attribute cannot be found or properly parsed,
	// then exception is thrown
	const char *attrib(const char *name) const;
	float  floatAttrib(const char *name) const;
	int      intAttrib(const char *name) const;

	// When adding new nodes, you have to make sure that strings given as
	// arguments will exist as long as XMLNode exists; use 'own' method
	// to reallocate them in the memory pool if you're unsure
	void addAttrib(const char *name, const char *value);
	void addAttrib(const char *name, float value);
	void addAttrib(const char *name, int value);
	
	XMLNode addChild(const char *name, const char *value = nullptr);
	XMLNode sibling(const char *name = nullptr) const;
	XMLNode child(const char *name = nullptr) const;

	const char *value() const;
	const char *name() const;
	
	const char *own(const char *str);
	explicit operator bool() const { return m_ptr != nullptr; }
	
protected:
	XMLNode(rapidxml::xml_node<> *ptr, rapidxml::xml_document<> *doc) :m_ptr(ptr), m_doc(doc) { }
	void parsingError(const char*) const;
	friend class XMLDocument;

	rapidxml::xml_node<> *m_ptr;
	rapidxml::xml_document<> *m_doc;
};

class XMLDocument {
public:
	XMLDocument();
	XMLDocument(const XMLDocument&) = delete;
	XMLDocument(XMLDocument&&);
	~XMLDocument();
	void operator=(XMLDocument&&);
	void operator=(const XMLDocument&) = delete;
	
	void load(const char *file_name);
	void save(const char *file_name);
	void serialize(Serializer&);

	XMLNode addChild(const char *name, const char *value = nullptr) const;
	XMLNode child(const char *name = nullptr) const;

	const char *own(const char *str);

protected:
	unique_ptr<rapidxml::xml_document<> > m_ptr;
};

#endif
