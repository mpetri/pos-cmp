#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"
#include "patterns.hpp"
#include "easylogging++.h"

_INITIALIZE_EASYLOGGINGPP

typedef struct cmdargs {
    std::string collection_dir;
    std::string pattern_file;
    std::string pattern_file_out;
} cmdargs_t;

void
print_usage(const char* program)
{
    fprintf(stdout,"%s -c <collection directory> -p <pattern file>\n",program);
    fprintf(stdout,"where\n");
    fprintf(stdout,"  -c <collection directory>  : the directory the collection is stored.\n");
    fprintf(stdout,"  -p <pattern file>  : the pattern file.\n");
};

cmdargs_t
parse_args(int argc,const char* argv[])
{
    cmdargs_t args;
    int op;
    args.collection_dir = "";
    args.pattern_file = "";
    args.pattern_file_out = "";
    while ((op=getopt(argc,(char* const*)argv,"c:p:o:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
            case 'p':
                args.pattern_file = optarg;
                break;
            case 'o':
                args.pattern_file_out = optarg;
                break;
        }
    }
    if (args.collection_dir==""||args.pattern_file==""||args.pattern_file_out=="") {
        std::cerr << "Missing command line parameters.\n";
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    return args;
}

int
determine_bucket(int64_t n)
{
    if (n < 25)
        return 0;
    if (n >= 50 && n <= 100)
        return 1;
    if (n >= 250 && n <= 500)
        return 2;
    if (n >= 1000 && n <= 5000)
        return 3;
    if (n >= 10000 && n <= 25000)
        return 4;
    if (n >= 100000 && n <= 250000)
        return 5;
    if (n >= 1000000 && n <= 2500000)
        return 6;
    if (n >= 10000000 && n <= 25000000)
        return 7;
    return -1;
}


int main(int argc,const char* argv[])
{
    _START_EASYLOGGINGPP(argc,argv);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime : %msg");

    LOG(INFO) << "Parsing command line arguments";
    cmdargs_t args = parse_args(argc,argv);

    LOG(INFO) << "Parsing collection directory " << args.collection_dir;
    collection col(args.collection_dir);

    /* load dict */
    auto dict_file = col.path + "/" + DICTFILE;
    std::unordered_map<std::string,uint64_t> id_mapping;
    if (utils::file_exists(dict_file)) {
        std::ifstream dfs(dict_file);
        std::string token_mapping;
        while (std::getline(dfs,token_mapping)) {
            auto sep_pos = token_mapping.find(' ');
            auto token_str = token_mapping.substr(0,sep_pos);
            auto token_id_str = token_mapping.substr(sep_pos+1);
            auto token_id = std::stoull(token_id_str);
            id_mapping[token_str] = token_id;
        }
    } else {
        LOG(FATAL) << "Cannot open dict file '" << dict_file << "'";
    }

    /* load patterns */
    std::vector<pattern_t> partial_patterns;
    if (utils::file_exists(args.pattern_file)) {
        std::ifstream pfs(args.pattern_file);
        std::string pattern_str;
        while (std::getline(pfs,pattern_str)) {
            pattern_t p;

            auto sep_pos = pattern_str.find(';');
            auto pattern_id_str = pattern_str.substr(0,sep_pos);
            auto pattern_tokens_str = pattern_str.substr(sep_pos+1);
            auto pattern_id = std::stoull(pattern_id_str);
            p.id = pattern_id;

            std::istringstream qry_content_stream(pattern_tokens_str);
            bool complete = true;
            for (std::string qry_token; std::getline(qry_content_stream,qry_token,' ');) {
                auto id_itr = id_mapping.find(qry_token);
                if (id_itr != id_mapping.end()) {
                    p.tokens.push_back(id_itr->second);
                } else {
                    complete = false;
                    std::cerr << "ERROR: could not find '" << qry_token << "' in the dictionary." << std::endl;
                }
            }
            if (complete) {
                p.m = p.tokens.size();
                partial_patterns.push_back(p);
            }
        }
    } else {
        LOG(FATAL) << "Cannot open pattern file '" << args.pattern_file << "'";
    }

    /* load index */
    using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
    index_abspos<eliasfano_list<true>,invidx_type> index(col);

    std::ofstream ofs(args.pattern_file_out);
    for (auto& pattern : partial_patterns) {

        size_t min_list_size = std::numeric_limits<size_t>::max();
        size_t list_size_sum = 0;
        for (const auto& t : pattern.tokens) {
            auto lst = index.list(t);
            min_list_size = std::min(min_list_size,lst.size());
            list_size_sum += lst.size();
        }
        pattern.min_list_size = min_list_size;
        pattern.list_size_sum = list_size_sum;

        {
            auto result = index.phrase_list(pattern.tokens);
            pattern.ndoc = result.size();
            pattern.nocc = 0;
            for (const auto& df : result) {
                pattern.nocc += df.second;
            }
            pattern.bucket = determine_bucket(pattern.ndoc);
        }
        pattern.sp = 0;
        pattern.ep = 0;

        ofs << pattern.id << ";"
            << pattern.bucket << ";"
            << pattern.m << ";"
            << pattern.sp << ";"
            << pattern.ep << ";"
            << pattern.ndoc << ";"
            << pattern.nocc << ";"
            << list_size_sum << ";"
            << min_list_size << ";";
        for (size_t i=0; i<pattern.tokens.size()-1; i++) {
            ofs << pattern.tokens[i] << ",";
        }
        ofs << pattern.tokens.back() << std::endl;

    }

    return 0;
}
