#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"
#include "patterns.hpp"
#include "intersection.hpp"

#include "sdsl/suffix_trees.hpp"
#include "sdsl/suffix_arrays.hpp"

#include "easylogging++.h"

#include <map>

_INITIALIZE_EASYLOGGINGPP

typedef struct cmdargs {
    std::string collection_dir;
    std::string pattern_file;
    uint32_t patterns_per_bucket;
} cmdargs_t;

void
print_usage(const char* program)
{
    fprintf(stdout,"%s -c <collection directory> -p <pattern file>\n",program);
    fprintf(stdout,"where\n");
    fprintf(stdout,"  -c <collection directory>  : the directory the collection is stored.\n");
    fprintf(stdout,"  -p <pattern file>  : the pattern file.\n");
    fprintf(stdout,"  -n <patterns per bucket>  : number of patterns per bucket to run.\n");
};

cmdargs_t
parse_args(int argc,const char* argv[])
{
    cmdargs_t args;
    int op;
    args.collection_dir = "";
    args.pattern_file = "";
    args.patterns_per_bucket = 0;
    while ((op=getopt(argc,(char* const*)argv,"c:p:n:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
            case 'p':
                args.pattern_file = optarg;
                break;
            case 'n':
                args.patterns_per_bucket = std::stoul(optarg);
                break;
        }
    }
    if (args.collection_dir==""||args.pattern_file=="") {
        std::cerr << "Missing command line parameters.\n";
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    return args;
}

template<class t_idx,class t_idx2>
void compare_index(const t_idx& indexA,const t_idx2& indexB,const std::vector<pattern_t>& patterns)
{
    size_t dchecksumA = 0;
    size_t dchecksumB = 0;
    for (const auto& pattern : patterns) {

        auto resultA = indexA.intersection(pattern.tokens);
        auto resultB = indexB.intersection(pattern.tokens);
        for (const auto& id : resultA) {
            dchecksumA += id;
        }
        for (const auto& id : resultB) {
            dchecksumB += id;
        }

        if (dchecksumA != dchecksumB) {
            std::cout << "ERROR!" << std::endl;
            std::cout << "pattern id = " << pattern.id << std::endl;
            std::cout << "pattern len = " << pattern.m << std::endl;
            std::cout << "|RA| = " << resultA.size() << std::endl;
            std::cout << "|RB| = " << resultB.size() << std::endl;
            // for(size_t i=0;i<resultB.size();i++) {
            // 	if(i < resultA.size()) {
            // 		if(resultA[i]!=resultB[i]) {
            // 			std::cout << "RES[" << i << "] = " << resultA[i] << " - " << resultB[i] << std::endl;
            // 		}
            // 	} else {
            // 		std::cout << "RES A MISSING AT POS " << i << " v= " << resultA[i] << std::endl;
            // 	}
            // }
            dchecksumA = 0;
            dchecksumB = 0;
        }
    }
    LOG(INFO) << "INDEXA " << dchecksumA;
    LOG(INFO) << "INDEXB " << dchecksumB;
}

int main(int argc,const char* argv[])
{
    _START_EASYLOGGINGPP(argc,argv);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime : %msg");

    /* parse command line */
    LOG(INFO) << "Parsing command line arguments";
    cmdargs_t args = parse_args(argc,argv);

    /* parse the collection */
    LOG(INFO) << "Parsing collection directory " << args.collection_dir;
    collection col(args.collection_dir);

    /* load pattern file */
    auto patterns = pattern_parser::parse_file<false>(args.pattern_file);
    LOG(INFO) << "Parsed " << patterns.size() << " patterns from file " << args.pattern_file;

    /* filter patterns */
    if (args.patterns_per_bucket != 0) {
        std::sort(patterns.begin(), patterns.end(), [](const pattern_t& a, const pattern_t& b) {
            return a.bucket < b.bucket;
        });
        size_t freq = 0;
        size_t cnt = 0;
        size_t bucket = patterns[0].bucket;
        auto itr = patterns.begin();
        while (itr != patterns.end()) {
            if (itr->bucket != bucket) {
                LOG(INFO) << "bucket = " << bucket << " cnt = " << cnt;
                bucket = itr->bucket;
                freq = 0;
                cnt = 0;
            }
            if (freq >= args.patterns_per_bucket) {
                itr = patterns.erase(itr);
            } else {
                itr++;
                cnt++;
            }
            freq++;
        }
        LOG(INFO) << "bucket = " << bucket << " cnt = " << cnt;
        LOG(INFO) << "Filtered " << patterns.size() << " patterns";
    }

    /* load indexes and test */
    using invidx_typeA = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    invidx_typeA indexA(col);

    using invidx_typeB = index_invidx<eliasfano_skip_list<64,true>,optpfor_list<128,false>>;
    invidx_typeB indexB(col);
    compare_index(indexA,indexB,patterns);

    return 0;
}
