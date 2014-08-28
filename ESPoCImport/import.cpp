#include <stdio.h>
#include <sys/stat.h>
#include <libxml/xmlreader.h>
#include <boost/concept_check.hpp>

#include "elasticsearch/elasticsearch.h"

#define CONST_CHAR(x) (reinterpret_cast<const char*>(x))


xmlTextReaderPtr g_reader;
long g_filesize;
ElasticSearch* g_es;

xmlChar* g_language_code = NULL;


void printStatus(long descriptors)
{
	float progress = (float)xmlTextReaderByteConsumed(g_reader)/g_filesize*100.0;
	fprintf(stdout, "Processed descriptors: %ld  (%0.1f%%)\r", descriptors, progress);
	fflush(stdout);
}

void PrepareImport()
{
    std::stringstream index;
    index << "mesh" << "/" << g_language_code;
    if (!g_es->exist(index.str()))
    {
        std::stringstream mapping;
        mapping << "{\"mappings\": {\"" << g_language_code << "\": {\"properties\": {"
                << "\"id\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"tree_numbers\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"concepts.id\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"concepts.preferred\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"concepts.terms.id\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"concepts.terms.language\": {\"type\": \"string\", \"index\": \"not_analyzed\"}, "
                << "\"concepts.terms.preferred\": {\"type\": \"string\", \"index\": \"not_analyzed\"} "
                << "} } } }";
                
        g_es->createIndex("mesh", mapping.str().c_str());
    }
    
    g_es->deleteAll("mesh", CONST_CHAR(g_language_code));
}

bool GetThesaurusLanguage(const xmlChar* thesaurus_id, std::string& language)
{
    const xmlChar* end_ptr = xmlStrchr(thesaurus_id, '(');
    if (!end_ptr) end_ptr = xmlStrchr(thesaurus_id, ' ');
    while (end_ptr>thesaurus_id && ' '==*(end_ptr-1))
        end_ptr--;

    int end_index = (end_ptr ? end_ptr-thesaurus_id : xmlStrlen(thesaurus_id));
    
    if (0 == xmlStrncmp(BAD_CAST("AHCPR"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("AU"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("BAN"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("BIOETHICS"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("CA"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("FDA SRS"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("GHR"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("IE"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("INN"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("IOM"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("JAN"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("LCSH"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("NLM"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("OMIM"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("ORD"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("POPLINE"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("UK"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("UMLS"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("UNK"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("USAN"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("USP"), thesaurus_id, end_index) ||
        0 == xmlStrncmp(BAD_CAST("US"), thesaurus_id, end_index))
    {
        language = "eng";
        return true;
    }
    else if (0 == xmlStrncmp(BAD_CAST("nor"), thesaurus_id, end_index))
    {
        language = "nor";
        return true;
    }
    else if (0 == xmlStrncmp(BAD_CAST("DE"), thesaurus_id, end_index))
    {
        language = "ger";
        return true;
    }
    else if (0 == xmlStrncmp(BAD_CAST("ES"), thesaurus_id, end_index) ||
             0 == xmlStrncmp(BAD_CAST("MX"), thesaurus_id, end_index))
    {
        language = "spa";
        return true;
    }
    else if (0 == xmlStrncmp(BAD_CAST("FR"), thesaurus_id, end_index))
    {
        language = "fre";
        return true;
    }
    else if (0 == xmlStrncmp(BAD_CAST("NL"), thesaurus_id, end_index))
    {
        language = "dut";
        return true;
    }
    else
    {
        language = "unknown";
    }
    return false;
}

const xmlChar* GetAttribute(const char* name, xmlNodePtr node_ptr)
{
    xmlAttrPtr attribute_ptr = node_ptr ? node_ptr->properties : NULL;
    while (attribute_ptr)
    {
        if (0 == xmlStrcmp(BAD_CAST(name), attribute_ptr->name))
        {
            xmlNodePtr text_node_ptr = attribute_ptr->children;
            if (XML_TEXT_NODE==text_node_ptr->type)
            {
                return text_node_ptr->content;
            }
        }
        
        attribute_ptr = attribute_ptr->next;
    }
    return NULL;
}

const xmlChar* GetText(xmlNodePtr text_ptr)
{
    xmlNodePtr text_node_ptr = text_ptr->children;
    if (XML_TEXT_NODE==text_node_ptr->type)
    {
        return text_node_ptr->content;
    }
    return NULL;
}

const xmlChar* AddText(Json::Object& json, const std::string& key, xmlNodePtr text_ptr)
{
    const xmlChar* text_str = GetText(text_ptr);
    if (text_str)
    {
        json.addMemberByKey(key, CONST_CHAR(text_str));
    }
    return text_str;
}

bool AddName(Json::Object& json, xmlNodePtr descriptor_name_ptr)
{
	xmlNodePtr string_ptr = descriptor_name_ptr->children;
	if (XML_ELEMENT_NODE==string_ptr->type && 0==xmlStrcmp(BAD_CAST("String"), string_ptr->name) && NULL!=string_ptr->children)
	{
		xmlNodePtr text_ptr = string_ptr->children;
		if (XML_TEXT_NODE==text_ptr->type && NULL!=text_ptr->content)
		{
			const xmlChar* left_bracket = xmlStrchr(text_ptr->content, '[');
			const xmlChar* right_bracket = left_bracket ? xmlStrchr(left_bracket, ']') : NULL;
			if (left_bracket && right_bracket)
			{
				xmlChar* nor_value = xmlStrndup(text_ptr->content, left_bracket - text_ptr->content);
				xmlChar* eng_value = xmlStrndup(left_bracket+1, right_bracket-(left_bracket+1));
				if (0==xmlStrcasecmp(BAD_CAST("Not Translated"), nor_value))
				{
                    xmlFree(nor_value);
                    nor_value = eng_value;
                    eng_value = NULL;
				}

                json.addMemberByKey("name", CONST_CHAR(nor_value));
                xmlFree(nor_value);

                if (eng_value)
                {
                    json.addMemberByKey("english_name", CONST_CHAR(eng_value));
                    xmlFree(eng_value);
                }
			}
			else
			{
				json.addMemberByKey("name", CONST_CHAR(text_ptr->content));
			}
			return true;
		}
	}
	return false;
}

xmlChar* AddLanguage(Json::Object& json, xmlNodePtr thesaurus_id_list_ptr)
{
    xmlChar* text_str = NULL;
    xmlNodePtr thesaurus_id_node_ptr = thesaurus_id_list_ptr->children;
    if (XML_ELEMENT_NODE==thesaurus_id_node_ptr->type)
    {
        const xmlChar* thesaurus_id = GetText(thesaurus_id_node_ptr);
        std::string language;
        if (GetThesaurusLanguage(thesaurus_id, language))
        {
            json.addMemberByKey("language", language);
        }
    }

    return text_str;
}

void ReadTreeNumberList(Json::Object& json, xmlNodePtr tree_number_list_ptr)
//<!ELEMENT TreeNumberList (TreeNumber)+>
{
	Json::Array tree_number_array;
	xmlNodePtr tree_number_ptr = tree_number_list_ptr->children;
	while (NULL!=tree_number_ptr)
	{
		if (XML_ELEMENT_NODE==tree_number_ptr->type && 0==xmlStrcmp(BAD_CAST("TreeNumber"), tree_number_ptr->name) && NULL!=tree_number_ptr->children)
		{
			xmlNodePtr text_ptr = tree_number_ptr->children;
			if (XML_TEXT_NODE==text_ptr->type && NULL!=text_ptr->content)
			{
				Json::Value tree_number;
				tree_number.setString(CONST_CHAR(text_ptr->content));
				tree_number_array.addElement(tree_number);
			}
		}
		
		tree_number_ptr=tree_number_ptr->next;
	}
	
	if (!tree_number_array.empty())
	{
		json.addMemberByKey("tree_numbers", tree_number_array);
	}
}

void ReadTermList(Json::Object& json, xmlNodePtr term_list_ptr)
//<!ELEMENT TermList (Term+)>
{
    Json::Array term_array;
    xmlNodePtr term_ptr = term_list_ptr->children;
    while (NULL!=term_ptr)
    {
        if (XML_ELEMENT_NODE==term_ptr->type && 0==xmlStrcmp(BAD_CAST("Term"), term_ptr->name) && NULL!=term_ptr->children)
        {
            Json::Object term;
            xmlNodePtr child = term_ptr->children;
            while (NULL!=child)
            {
                if (XML_ELEMENT_NODE == child->type)
                {
                    if (0==xmlStrcmp(BAD_CAST("TermUI"), child->name))
                    {
                        AddText(term, "id", child);
                    }
                    else if (0==xmlStrcmp(BAD_CAST("String"), child->name))
                    {
                        AddText(term, "text", child);
                    }
                    else if (0==xmlStrcmp(BAD_CAST("ThesaurusIDlist"), child->name))
                    {
                        AddLanguage(term, child);
                    }
                }
                child = child->next;
            }
            bool preferred = (0 == xmlStrcmp(BAD_CAST("Y"), GetAttribute("ConceptPreferredTermYN", term_ptr)));
            term.addMemberByKey("preferred", preferred ? "yes" : "no");

            term_array.addElement(term);
        }
        term_ptr=term_ptr->next;
    }
    
    if (!term_array.empty())
    {
        json.addMemberByKey("terms", term_array);
    }
}

void ReadConceptList(Json::Object& json, xmlNodePtr concept_list_ptr)
//<!ELEMENT ConceptList (Concept+)  >
{
    Json::Array concept_array;
    xmlNodePtr concept_ptr = concept_list_ptr->children;
    while (NULL!=concept_ptr)
    {
        if (XML_ELEMENT_NODE==concept_ptr->type && 0==xmlStrcmp(BAD_CAST("Concept"), concept_ptr->name) && NULL!=concept_ptr->children)
        {
            Json::Object concept;
            xmlNodePtr child = concept_ptr->children;
            while (NULL!=child)
            {
                if (XML_ELEMENT_NODE == child->type)
                {
                    if (0==xmlStrcmp(BAD_CAST("ConceptUI"), child->name))
                    {
                        AddText(concept, "id", child);
                    }
                    else if (0==xmlStrcmp(BAD_CAST("ConceptName"), child->name))
                    {
                        AddName(concept, child);
                    }
                    else if (0==xmlStrcmp(BAD_CAST("ScopeNote"), child->name))
                    {
                        AddText(concept, "description", child);
                    }
                    else if (0==xmlStrcmp(BAD_CAST("TermList"), child->name))
                    {
                        ReadTermList(concept, child);
                    }
                }
                child = child->next;
            }
            bool preferred = (0 == xmlStrcmp(BAD_CAST("Y"), GetAttribute("PreferredConceptYN", concept_ptr)));
            concept.addMemberByKey("preferred", preferred ? "yes" : "no");

            concept_array.addElement(concept);
        }
        concept_ptr=concept_ptr->next;
    }
    
    if (!concept_array.empty())
    {
        json.addMemberByKey("concepts", concept_array);
    }
}

bool ProcessDescriptorRecord(xmlNodePtr descriptor_record_ptr)
//<!ELEMENT DescriptorRecord (%DescriptorReference;,
//                            DateCreated,
//                            DateRevised?,
//                            DateEstablished?,
//                            ActiveMeSHYearList,
//                            AllowableQualifiersList?,
//                            Annotation?,
//                            HistoryNote?,
//                            OnlineNote?,
//                            PublicMeSHNote?,
//                            PreviousIndexingList?,
//                            EntryCombinationList?,
//                            SeeRelatedList?,
//                            ConsiderAlso?,
//                            PharmacologicalActionList?,
//                            RunningHead?,
//                            TreeNumberList?,
//                            RecordOriginatorsList,
//                            ConceptList) >
//<!ATTLIST DescriptorRecord DescriptorClass (1 | 2 | 3 | 4)  "1">
//<!ENTITY  % DescriptorReference "(DescriptorUI, DescriptorName)">
{
    Json::Object json;
	const xmlChar* id = NULL;
	xmlNodePtr child = descriptor_record_ptr->children;
	while (NULL!=child)
	{
		if (XML_ELEMENT_NODE == child->type)
		{
			if (0==xmlStrcmp(BAD_CAST("DescriptorUI"), child->name))
			{
				id = AddText(json, "id", child);
			}
			else if (0==xmlStrcmp(BAD_CAST("DescriptorName"), child->name))
			{
				AddName(json, child);
			}
			else if (0==xmlStrcmp(BAD_CAST("TreeNumberList"), child->name))
			{
				ReadTreeNumberList(json, child);
			}
			else if (0==xmlStrcmp(BAD_CAST("ConceptList"), child->name))
			{
                ReadConceptList(json, child);
			}
		}
		
		child = child->next;
	}

	if (id)
	{
		g_es->index("mesh", CONST_CHAR(g_language_code), CONST_CHAR(id), json);
	}

	return true;
}

bool ReadDescriptorRecordSet()
//<!ELEMENT DescriptorRecordSet (DescriptorRecord*)>
//<!ATTLIST DescriptorRecordSet LanguageCode (cze|dut|eng|fin|fre|ger|ita|jpn|lav|por|scr|slv|spa) #REQUIRED>
{
	if (XML_READER_TYPE_ELEMENT!=xmlTextReaderNodeType(g_reader) || 0!=xmlStrcmp(BAD_CAST("DescriptorRecordSet"), xmlTextReaderConstName(g_reader)))
		return false;

    g_language_code = xmlStrdup(xmlTextReaderGetAttribute(g_reader, BAD_CAST("LanguageCode")));
    if (!g_language_code)
        return false;

    PrepareImport();

	if (1 != xmlTextReaderRead(g_reader)) //Skip to first DescriptorRecord
		return false;

	long processed = 0L;
	bool more = true;
	xmlNodePtr descriptor_record_ptr;
	while (more &&
	       NULL!=(descriptor_record_ptr=xmlTextReaderExpand(g_reader)) &&
	       XML_ELEMENT_NODE==descriptor_record_ptr->type && 0==xmlStrcmp(BAD_CAST("DescriptorRecord"), descriptor_record_ptr->name)) //Read and parse current DescriptorRecord
	{
		ProcessDescriptorRecord(descriptor_record_ptr);
		more = (1 == xmlTextReaderNext(g_reader)); //Skip to next DescriptorRecord

		if (!more || 0==(++processed)%500)
		{
			printStatus(processed);
		}
	}
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <ElasticSearch-location> <MeSH-file>\n\nExample: %s localhost:9200 ~/Downloads/nordesc2014.xml\n\n", argv[0], argv[0]);
		return -1;
    }

	LIBXML_TEST_VERSION

	g_es = new ElasticSearch(argv[1]);

	const char* filename = argv[2];
	struct stat filestat;
	stat(filename, &filestat);
	g_filesize = filestat.st_size;

	g_reader = xmlReaderForFile(filename, NULL, XML_PARSE_NOBLANKS|XML_PARSE_NOCDATA|XML_PARSE_COMPACT);
	if (!g_reader)
	{
		fprintf(stderr, "File Not Found: %s\n", filename);
	}
	else
	{
		if (1==xmlTextReaderNext(g_reader) && XML_READER_TYPE_DOCUMENT_TYPE==xmlTextReaderNodeType(g_reader) && //Skip DOCTYPE
		    1==xmlTextReaderNext(g_reader)) //Read DescriptorRecordSet
		{
			ReadDescriptorRecordSet();
		}

		xmlFreeTextReader(g_reader);
        xmlFree(g_language_code);
	}

    xmlCleanupParser();
	delete g_es;
	
    return 0;
}
