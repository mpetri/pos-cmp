#pragma once

#include "collection.hpp"
#include "utils.hpp"

struct query_t {
    bool complete;
    uint64_t id;
    std::string org;
    std::vector<uint64_t> ids;
};

struct dict_map {
    std::unordered_map<std::string,uint64_t> id_mapping;
    std::unordered_map<uint64_t,std::string> reverse_id_mapping;
    dict_map(collection& col)
    {
        auto file_name = col.path +"/"+DICTFILE;
        if (utils::file_exists(file_name)) {  // parse
            LOG(INFO) << "CONSTRUCT dict map";
            std::ifstream dfs(file_name);
            std::string term_mapping;
            while (std::getline(dfs,term_mapping)) {
                auto sep_pos = term_mapping.find(' ');
                auto term = term_mapping.substr(0,sep_pos);
                auto idstr = term_mapping.substr(sep_pos+1);
                uint64_t id = std::stoull(idstr);
                id_mapping[term] = id;
                reverse_id_mapping[id] = term;
            }
        } else {
            LOG(FATAL) << "dictionary file '" << file_name << "' not found."
                       throw std::runtime_error("dictionary file '"+file_name+"' not found.");
        }
    }
    query_t
    parse_query(const std::string& query_str)
    {
        auto id_sep_pos = query_str.find(';');
        auto qryid_str = query_str.substr(0,id_sep_pos);
        auto qry_id = std::stoull(qryid_str);
        auto qry_content = query_str.substr(id_sep_pos+1);

        std::vector<uint64_t> ids;
        std::istringstream qry_content_stream(qry_content);
        for (std::string qry_token; std::getline(qry_content_stream,qry_token,' ');) {
            auto id_itr = id_mapping.find(qry_token);
            if (id_itr != id_mapping.end()) {
                ids.push_back(id_itr->second);
            } else {
                LOG(ERROR) << "ERROR: could not find '" << qry_token << "' in the dictionary.";
                return {false,qry_id,qry_content,ids};
            }
        }
        return {true,qry_id,qry_content,ids};
    }

};

std::vector<query_t>
parse_queries(collection& col,const std::string& qry_file)
{
    std::vector<query_t> qrys;
    if (! utils::file_exists(qry_file)) {
        LOG(FATAL) << "Could not read query file '" << qry_file << "'.";
        throw std::runtime_error("Could not read query file '"+qry_file+"'.");
    } else {
        auto dict = dict_map(col);
        std::ifstream qfs(qry_file);
        std::string qry;
        while (std::getline(qfs,qry)) {
            qrys.emplace_back(dict.parse_query(qry));
        }
    }
    return qrys;
}