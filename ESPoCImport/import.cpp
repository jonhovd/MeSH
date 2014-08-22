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

void Soundex(xmlChar* str, char* soundex /* char[5]*/)
{
	int length = xmlStrlen(str);
	int index = 0;
	xmlChar* ptr = str;
	unsigned char b;
	unsigned char prev = '0';
	while (ptr && (ptr-str)<length && 4>index)
	{
		b = *ptr;
		if (0b00000000==(b&0b10000000)) //1-byte UTF-8
		{
			if ('a'<=b && 'z'>=b) //toLower
			{
				b += 'A'-'a';
			}
			
			if ('A'<=b && 'Z'>=b)
			{
#define SOUNDEX(x) if((x)!=prev){prev=(x);*(soundex+index)=(0==index?b:prev);index++;}break
				switch (b)
				{
					case 'B':
					case 'F':
					case 'P':
					case 'V': SOUNDEX('1');

					case 'C':
					case 'G':
					case 'J':
					case 'K':
					case 'Q':
					case 'S':
					case 'X':
					case 'Z': SOUNDEX('2');

					case 'D':
					case 'T': SOUNDEX('3');

					case 'L': SOUNDEX('4');

					case 'M':
					case 'N': SOUNDEX('5');

					case 'R': SOUNDEX('6');

					default: prev='0'; if (0==index) *(soundex+index++) = b; break;
				}
			}
			ptr++;
		}
		else if (0b11000000==(b&0b11100000)) //2-byte UTF-8
		{
			ptr+=2;
		}
		else if (0b11100000==(b&0b11110000)) //3-byte UTF-8
		{
			ptr+=3;
		}
		else if (0b11110000==(b&0b11111000)) //4-byte UTF-8
		{
			ptr+=4;
		}
	}
	
	while (index<4)
	{
		*(soundex+(index++)) = '0';
	}

	*(soundex+index) = 0;
}

void PrepareImport()
{
    std::stringstream index;
    index << "mesh" << "/" << g_language_code;
    if (!g_es->exist(index.str()))
    {
        //g_es->createIndex();
    }
    
    g_es->deleteAll("mesh", CONST_CHAR(g_language_code));
}

xmlChar* AddId(Json::Object& json, xmlNodePtr descriptor_ui_ptr)
{
	xmlChar* id = NULL;
	xmlNodePtr text_ptr = descriptor_ui_ptr->children;
	if (XML_TEXT_NODE==text_ptr->type && NULL!=text_ptr->content)
	{
		id = text_ptr->content;
		json.addMemberByKey("id", CONST_CHAR(id));
	}
	return id;
}

bool AddName(Json::Object& json, xmlNodePtr descriptor_name_ptr)
{
	xmlNodePtr string_ptr = descriptor_name_ptr->children;
	if (XML_ELEMENT_NODE==string_ptr->type && 0==xmlStrcmp(BAD_CAST("String"), string_ptr->name) && NULL!=string_ptr->children)
	{
		xmlNodePtr text_ptr = string_ptr->children;
		if (XML_TEXT_NODE==text_ptr->type && NULL!=text_ptr->content)
		{
			char soundex[5];
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
                Soundex(nor_value, soundex);
                json.addMemberByKey("soundex", CONST_CHAR(soundex));
                xmlFree(nor_value);

                if (eng_value)
                {
                    json.addMemberByKey("english_name", CONST_CHAR(eng_value));
                    Soundex(eng_value, soundex);
                    json.addMemberByKey("english_soundex", CONST_CHAR(soundex));
                    xmlFree(eng_value);
                }
			}
			else
			{
				json.addMemberByKey("name", CONST_CHAR(text_ptr->content));
				Soundex(text_ptr->content, soundex);
				json.addMemberByKey("soundex", CONST_CHAR(soundex));
			}
			return true;
		}
	}
	return false;
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
	xmlChar* id = NULL;
	xmlNodePtr child = descriptor_record_ptr->children;
	while (NULL!=child)
	{
		if (XML_ELEMENT_NODE == child->type)
		{
			if (0==xmlStrcmp(BAD_CAST("DescriptorUI"), child->name))
			{
				id = AddId(json, child);
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
				//ToDo
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
