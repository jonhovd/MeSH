#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unordered_set>
#include <boost/concept_check.hpp>

#include "elasticsearch/elasticsearch.h"

#define CONST_CHAR(x) (reinterpret_cast<const char*>(x))

#define LANGUAGE_CODE "nor"
#define TRANSLATED_MARK "NOR: "

ElasticSearch* g_es;

enum Action {
    KEEP,
    TAG,
    SKIP
};

Action g_action_duplicates = KEEP;
Action g_action_translated = KEEP;

std::unordered_set<std::string> g_duplicates;


long ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result)
{
    try
    {
        return g_es->search(index, type, query, search_result);
    }
    catch(...)
    {
        return 0L;
    }
}

void RegisterDisplayedDuplicates(const std::string& tree_number)
{
    std::stringstream query;
    query << "{\"from\": 0, \"size\": 100, \"query\": {\"bool\": {\"must\": {\"term\": {\"tree_numbers\": \"" << tree_number << "\"} } } } }";
 
    Json::Object search_result;
    if (0 == ESSearch("mesh", CONST_CHAR(LANGUAGE_CODE), query.str(), search_result))
        return;
    
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();

    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    Json::Array::const_iterator hits_iterator = hits_array.begin();
    for (; hits_iterator!=hits_array.end(); ++hits_iterator)
    {
        const Json::Value hit_value = *hits_iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
        const Json::Array tree_numbers_array = tree_numbers_value.getArray();

        Json::Array::const_iterator tree_number_iterator = tree_numbers_array.begin();
        for (; tree_number_iterator!=tree_numbers_array.end(); ++tree_number_iterator)
        {
            const Json::Value tree_number_value = *tree_number_iterator;
            const std::string tree_number_str = tree_number_value.getString();

            g_duplicates.insert(tree_number_str); //Add this MeSH to set of displayed MeSH'es
        }
    }
}

bool ParseTreeNumber(const char* line, std::string& tree_number)
{
    if (!line)
        return false;

    const char* start = line;
    while (' '==*start) start++;
    if ('['==*start) start++;
    
    const char* end = start;
    while (' '!=*end && ']'!=*end && '\r'!=*end && '\n'!=*end) end++;
    
    if (start != end)
    {
        tree_number = std::string(start, end-start);
        return tree_number.size() > 0;
    }
    
    return false;
}

void ProcessFile(const char* input_filename, const char* output_filename)
{
    FILE* input_fp;
    FILE* output_fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    if (NULL == (input_fp=fopen(input_filename, "rb")))
    {
        fprintf(stderr, "Didn't find file: %s\n\n", input_filename);
        return;
    }

    if (NULL == (output_fp=fopen(output_filename, "wb")))
    {
        fclose(input_fp);
        fprintf(stderr, "Couldn't create file: %s\n\n", output_filename);
        return;
    }

    std::string tree_number;
    while ((read = getline(&line, &len, input_fp)) != -1) {
        if (!line || !*line)
        {
            continue; //Skip emtpy lines
        }
        
        if (g_action_duplicates != KEEP)
        {
            if (ParseTreeNumber(line, tree_number))
            {
                if (g_duplicates.end() != g_duplicates.find(tree_number))
                {
                    if (g_action_duplicates == TAG)
                    {
                        fprintf(output_fp, "[DUPLICATE]");
                    }
                    else //if (g_action_duplicates == SKIP)
                    {
                        continue;
                    }
                }
                else
                {
                    RegisterDisplayedDuplicates(tree_number);
                }
            }
        }

        if (g_action_translated != KEEP)
        {
            if (NULL!=strstr(line, TRANSLATED_MARK))
            {
                if (g_action_translated == TAG)
                {
                    fprintf(output_fp, "[TRANSLATED]");
                }
                else //if (g_action_translated == SKIP)
                {
                    continue;
                }
            }
        }

        fprintf(output_fp, "%s", line);
    }

    fclose(input_fp);
    fclose(output_fp);

    if (line)
    {
        free(line);
    }
}

void Usage(const char* name)
{
    fprintf(stderr, "Usage: %s <ElasticSearch-location> [--tag-duplicates] [--skip-duplicates] [--tag-translated] [--skip-translated] <Input-file> <Output-file>\n\nExample: %s localhost:9200 --skip-translated \"~/Downloads/NOR_tree_structure_updated_m def_040215.txt\" > /tmp/tree_structure.txt\n\n", name, name);
}

int main(int argc, char **argv)
{
	if (argc < 4)
    {
        Usage(argv[0]);
		return -1;
    }

	g_es = new ElasticSearch(argv[1]);
    
    const char* input_filename = NULL;
    const char* output_filename = NULL;

    int current_arg = 2;
    while(current_arg < argc)
    {
        if (0==strcmp("--tag-duplicates", argv[current_arg]))
        {
            g_action_duplicates = TAG;
            current_arg++;
        }
        else if (0==strcmp("--skip-duplicates", argv[current_arg]))
        {
            g_action_duplicates = SKIP;
            current_arg++;
        }
        else if (0==strcmp("--tag-translated", argv[current_arg]))
        {
            g_action_translated = TAG;
            current_arg++;
        }
        else if (0==strcmp("--skip-translated", argv[current_arg]))
        {
            g_action_translated = SKIP;
            current_arg++;
        }
        else if (current_arg == (argc-2))
        {
            input_filename = argv[current_arg];
            output_filename = argv[current_arg+1];
            break;
        }
        else
        {
            Usage(argv[0]);
            return -1;
        }
    }

    ProcessFile(input_filename, output_filename);

	delete g_es;
	
    return 0;
}
