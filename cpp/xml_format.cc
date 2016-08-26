#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"

#include "xml_format.h"

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#define XML_TRUE_STRING "true"
#define XML_FALSE_STRING "false"

using namespace std;

namespace google {
namespace protobuf {



XmlFormat::Printer::Printer() {}
XmlFormat::Printer::~Printer() {}

void XmlFormat::Printer::PrintToXmlString(const Message& message,
                                        string* output) {
  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();

  // create the xml dom
  rapidxml::xml_document<> doc;

  MessageToDOM(message,&doc);
  std::stringstream ss;
  ss << doc;
  *output = ss.str();

 // rapidxml::xml_node<>* root = doc.first_node();
  //std::cout << root->name() << std::endl;

  //! 获取根节点第一个节点  
   ///rapidxml::xml_node<>* node1 = root->first_node();
  //std::cout << node1->name() << std::endl;

 // rapidxml::xml_node<>* node11 = node1->first_node();
 // std::cout << node11->name() << std::endl;
 // std::cout << node11->value() << std::endl;
  //rapidxml::xml_node<>* node12 = node11->first_node();
 // std::cout << node12->name() << std::endl;
 // std::cout << node12->value() << std::endl;

}


void XmlFormat::Printer::MessageToDOM(const Message& message, rapidxml::xml_document<>* doc) {
	// xml version and encoding
	rapidxml::xml_node<>* xml_decl = doc->allocate_node(rapidxml::node_declaration);
	xml_decl->append_attribute(doc->allocate_attribute("version", "1.0"));
	xml_decl->append_attribute(doc->allocate_attribute("encoding", "utf-8"));
	doc->append_node(xml_decl);

	// the root node of the protobuf xml is the name of the protobuf container
	rapidxml::xml_node<> *root_node = doc->allocate_node(rapidxml::node_element, doc->allocate_string(message.GetDescriptor()->name().c_str()));
	doc->append_node(root_node);

	PrintXml(message, doc, root_node);
}

void XmlFormat::Printer::PrintXml(const Message& message,
								rapidxml::xml_document<>* doc,
								rapidxml::xml_node<>* node) {

	const Reflection* reflection = message.GetReflection();
	vector<const FieldDescriptor*> fields;
	reflection->ListFields(message, &fields);
	for (unsigned int i = 0; i < fields.size(); i++) {
		PrintXmlField(message,reflection, fields[i], doc, node);
	}

}


void XmlFormat::Printer::PrintXmlField(const Message& message,
                                     const Reflection* reflection,
                                     const FieldDescriptor* field,
                             		rapidxml::xml_document<>* doc,
                         			rapidxml::xml_node<>* node) {
	int count = 0;
	if (field->is_repeated()) {
		count = reflection->FieldSize(message, field);
	} else if (reflection->HasField(message, field)) {
		count = 1;
	}

	for (int j = 0; j < count; ++j) {
		// Write the field value.
		int field_index = j;
		if (!field->is_repeated()) {
			field_index = -1;
		}

		PrintXmlFieldValue(message, reflection, field, field_index, doc, node);
	}
}


string XmlFormat::Printer::GetXmlFieldName(const Message& message,
                                         const Reflection* reflection,
                                         const FieldDescriptor* field) {
	if (field->is_extension()) {
		// We special-case MessageSet elements for compatibility with proto1.
		if (field->containing_type()->options().message_set_wire_format()
				&& field->type() == FieldDescriptor::TYPE_MESSAGE
				&& field->is_optional()
				&& field->extension_scope() == field->message_type()) {
			//cout << field->message_type()->full_name().c_str()<<endl;
			return field->message_type()->full_name();
		} else {
			//cout << field->full_name().c_str()<<endl;
			return field->full_name();
		}
	} else {
		if (field->type() == FieldDescriptor::TYPE_GROUP) {
			// Groups must be serialized with their original capitalization.
			//cout << field->full_name().c_str() << endl;
			return field->message_type()->name();
		} else {
		//	cout << field->full_name().c_str() << endl;
			return field->name();
		}
	}
}

void XmlFormat::Printer::PrintXmlFieldValue(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field,
    int field_index,
	rapidxml::xml_document<>* doc,
	rapidxml::xml_node<>* node) {

	GOOGLE_DCHECK(field->is_repeated() || (field_index == -1))
    		<< "field_index must be -1 for non-repeated fields";

	switch (field->cpp_type()) {

		//tm use the preprocessor to generate the numerical value cases
		// replace the google used string methods with using a stringstream
#define OUTPUT_FIELD(CPPTYPE, METHOD, NUM_TYPE)                         \
	case FieldDescriptor::CPPTYPE_##CPPTYPE: {                              \
	NUM_TYPE value = field->is_repeated() ? \
	reflection->GetRepeated##METHOD(message, field, field_index) : \
	reflection->Get##METHOD(message, field);                          \
	stringstream number_stream; \
	number_stream << value; \
	rapidxml::xml_node<> *string_node = doc->allocate_node(\
	rapidxml::node_element, \
	doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()), \
	doc->allocate_string(number_stream.str().c_str()));                                              \
    	node->append_node(string_node);                                      \
        break;                                                               \
      }

      OUTPUT_FIELD( INT32,  Int32, int32);
      OUTPUT_FIELD( INT64,  Int64, int64);
      OUTPUT_FIELD(UINT32, UInt32, uint32);
      OUTPUT_FIELD(UINT64, UInt64, uint64);
      OUTPUT_FIELD( FLOAT,  Float, float);
      OUTPUT_FIELD(DOUBLE, Double, double);
#undef OUTPUT_FIELD

	case FieldDescriptor::CPPTYPE_STRING: {
		string scratch;
        const string& value = field->is_repeated() ?
            reflection->GetRepeatedStringReference(
              message, field, field_index, &scratch) :
            reflection->GetStringReference(message, field, &scratch);

    	rapidxml::xml_node<char> *string_node = doc->allocate_node(rapidxml::node_element,
			doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
			doc->allocate_string(value.c_str()));
	     cout << string_node->name() << endl;
		 cout << string_node->value() << endl;
    	node->append_node(string_node);

        break;
    }

    case FieldDescriptor::CPPTYPE_BOOL: {
        if (field->is_repeated()) {
        	if (reflection->GetRepeatedBool(message, field, field_index)) {
        		rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
					doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
        			XML_TRUE_STRING);
				cout << bool_node->name() << endl;
        		node->append_node(bool_node);
        	} else {
        		rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
					doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
        			XML_FALSE_STRING);
				cout << bool_node->name() << endl;
        		node->append_node(bool_node);
        	}
        } else {
        	if (reflection->GetBool(message,field)) {
        		rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
					doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
        			XML_TRUE_STRING);
				cout << bool_node->name() << endl;
        		node->append_node(bool_node);
        	} else {
        		rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
					doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
        			XML_FALSE_STRING);
				cout << bool_node->name() << endl;
        		node->append_node(bool_node);
        	}
        }
        break;
    }

    case FieldDescriptor::CPPTYPE_ENUM: {
        string value = field->is_repeated() ?
          reflection->GetRepeatedEnum(message, field, field_index)->name() :
          reflection->GetEnum(message, field)->name();
    	rapidxml::xml_node<> *enum_node = doc->allocate_node(rapidxml::node_element,
			doc->allocate_string(GetXmlFieldName(message, reflection, field).c_str()),
			doc->allocate_string(value.c_str()));
		
    	node->append_node(enum_node);
        break;
    }
    case FieldDescriptor::CPPTYPE_MESSAGE: {
    	// create the child node and recurse
											   rapidxml::xml_node<> *message_node = doc->allocate_node(rapidxml::node_element, doc->allocate_string(field->name().c_str()));
    	node->append_node(message_node);
		cout << message_node->name() << endl;
    	PrintXml(field->is_repeated() ?
    	                  reflection->GetRepeatedMessage(message, field, field_index) :
    	                  reflection->GetMessage(message, field),
    	         doc, message_node);
        break;
    }
	}
}



/* static */ void XmlFormat::PrintToXmlString(
    const Message& message, string* output) {
  Printer().PrintToXmlString(message, output);
}

/* static */ void XmlFormat::MessageToDOM(
	const Message& message, rapidxml::xml_document<>* doc) {
	Printer().MessageToDOM(message, doc);
}


}  // namespace protobuf
}  // namespace google
