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
void verify_index(t_idx& index,const std::vector<pattern_t>& patterns,const char* name)
{
    LOG(INFO) << "VERIFY = " << name;
    using clock = std::chrono::high_resolution_clock;
    size_t dchecksum = 0;
    size_t fchecksum = 0;
    std::chrono::nanoseconds total(0);
    for (const auto& pattern : patterns) {
        // LOG(INFO) << "("<<pattern.id<<")" << " res = " << pattern.ndoc;
        auto start = clock::now();
        auto result = index.phrase_list(pattern.tokens);
        auto stop = clock::now();
        total += (stop-start);
        for (const auto& df : result) {
            dchecksum += df.first;
            fchecksum += df.second;
        }
        if (result.size() != pattern.ndoc) {
            LOG(ERROR) << "result does not mach ndoc. res = " << result.size() << " ndoc = " << pattern.ndoc;
            return;
        }
    }

    LOG(INFO) << "INDEX = " << name << " DCHECKSUM = " << dchecksum << " FCHECKSUM = " << fchecksum;
    LOG(INFO) << "INDEX = " << name << " time = " <<
              std::chrono::duration_cast<std::chrono::milliseconds>(total).count()/1000.0f
              << " secs";
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

    auto patterns = pattern_parser::parse_file(args.pattern_file);
    LOG(INFO) << "Parsed " << patterns.size() << " patterns from file " << args.pattern_file;

    /* verify index */
    // {
    //     index_sort<> index(col);
    //     verify_index(index,patterns,"SORT");
    // }
    // {
    //     index_wt<> index(col);
    //     verify_index(index,patterns,"WT");
    // }
    // {
    //     index_sada<> index(col);
    //     verify_index(index,patterns,"SADA");
    // }
    {
        using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
        index_abspos<eliasfano_list<true>,invidx_type> index(col);
        verify_index(index,patterns,"ABS-EF");
    }
    {
        using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
        index_abspos<eliasfano_skip_list<64,true>,invidx_type> index(col);
        verify_index(index,patterns,"ABS-ESF-64");
    }
    {
        using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
        index_abspos<uniform_eliasfano_list<128>,invidx_type> index(col);
        verify_index(index,patterns,"ABS-UEF-128");
    }
    {
        using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
        index_nextword<eliasfano_list<true>,invidx_type> index(col);
        verify_index(index,patterns,"NEXT-EF");
    }

    return 0;
}
