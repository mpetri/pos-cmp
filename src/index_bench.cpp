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
    while ((op=getopt(argc,(char* const*)argv,"c:p:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
            case 'p':
                args.pattern_file = optarg;
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

template<class t_idx>
void bench_intersection(const t_idx& index,
                        const std::vector<pattern_t>& patterns,
                        const char* name,
                        ostream& ofs)
{
    using clock = std::chrono::high_resolution_clock;
    size_t checksum = 0;
    for (const auto& pattern : patterns) {
        auto start = clock::now();
        auto result = index.phrase_positions(pattern.tokens);
        auto stop = clock::now();
        for (const auto& pos : result) {
            checksum += pos;
        }
        // ofs << name << ";"
        //     << pattern.id << ";"
        //     << pattern.m << ";"
        //     << pattern.ndoc << ";"
        //     << pattern.nocc << ";"
        //     << pattern.bucket << ";"
        //     << std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count() << endl;
    }
    LOG(INFO) << "INDEX = " << name << " CHECKSUM = " << checksum;
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
    auto patterns = pattern_parser::parse_file(args.pattern_file);
    LOG(INFO) << "Parsed " << patterns.size() << " patterns from file " << args.pattern_file;


    /* open output file */
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto sec_since_epoc = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto time_str = std::to_string(sec_since_epoc.count());
    ofstream resfs(col.path+"/results/bench_pos-"+time_str+".csv");

    resfs << "type;id;len;ndoc;nocc;bucket;time_ns" << std::endl;

    /* load indexes and test */
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<uniform_eliasfano_list<128>,invidx_type> index(col);
        bench_intersection(index,patterns,"ABSPOS-UEF-128",resfs);
    }
    // {
    //     using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    //     index_nextword<uniform_eliasfano_list<128>,invidx_type> index(col);
    //     bench_intersection(index,patterns,"NEXTWORD-UEF-128",resfs);
    // }
    // {
    //     using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    //     index_abspos<eliasfano_list<true,false>,invidx_type> index(col);
    //     bench_intersection(index,patterns,"ABSPOS-EF",resfs);
    // }
    return 0;
}
