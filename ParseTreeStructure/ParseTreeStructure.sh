#!/bin/sh
./ParseTreeStructure localhost:9200 --tag-duplicates --tag-translated "../MeSHImport/NOR_tree_structure.txt" "/tmp/NOR_tree_structure_parsed_duplicates_translated.txt"
