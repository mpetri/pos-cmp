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
    args.patterns_per_bucket = 100;
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

template<class t_idx>
void bench_doc_intersection(const t_idx& index,
                            const std::vector<pattern_t>& patterns,
                            const char* name,
                            ostream& ofs)
{
    LOG(INFO) << "BENCH = " << name;
    using clock = std::chrono::high_resolution_clock;
    size_t dchecksum = 0;
    size_t fchecksum = 0;
    std::chrono::nanoseconds total(0);
    for (const auto& pattern : patterns) {
        LOG(INFO) << "id=" << pattern.id << " m=" << pattern.m << " ndoc=" << pattern.ndoc << " nocc=" << pattern.nocc
                  << " list_sum=" << pattern.list_size_sum << " min_list=" << pattern.min_list_size;
        auto start = clock::now();
        auto result = index.phrase_list(pattern.tokens);
        auto stop = clock::now();
        for (const auto& df : result) {
            dchecksum += df.first;
            fchecksum += df.second;
        }
        total += (stop-start);
        ofs << name << ";"
            << pattern.id << ";"
            << pattern.m << ";"
            << pattern.ndoc << ";"
            << pattern.nocc << ";"
            << pattern.list_size_sum << ";"
            << pattern.min_list_size << ";"
            << pattern.bucket << ";"
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count() << endl;
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
        if (freq >= args.patterns_per_bucket || bucket > 6) {
            itr = patterns.erase(itr);
        } else {
            itr++;
            cnt++;
        }
        freq++;
    }
    LOG(INFO) << "bucket = " << bucket << " cnt = " << cnt;
    LOG(INFO) << "Filtered " << patterns.size() << " patterns";

    /* open output file */
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto sec_since_epoc = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto time_str = std::to_string(sec_since_epoc.count());
    ofstream resfs(col.path+"/results/bench_doc-"+time_str+".csv");

    resfs << "type;id;len;ndoc;nocc;list_sum;min_list_len;bucket;time_ns" << std::endl;

    /* load indexes and test */
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<uniform_eliasfano_list<128>,invidx_type> index(col);
        bench_doc_intersection(index,patterns,"ABSPOS-UEF-128",resfs);
    }
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<eliasfano_sskip_list<64,true>,invidx_type> index(col);
        bench_doc_intersection(index,patterns,"ABSPOS-ESSF",resfs);
    }
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<eliasfano_skip_list<64,true>,invidx_type> index(col);
        bench_doc_intersection(index,patterns,"ABSPOS-ESF",resfs);
    }
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_nextword<uniform_eliasfano_list<128>,invidx_type> index(col);
        bench_doc_intersection(index,patterns,"NEXTWORD-UEF-128",resfs);
    }
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<eliasfano_list<true,false>,invidx_type> index(col);
        bench_doc_intersection(index,patterns,"ABSPOS-EF",resfs);
    }
    {
        index_sort<> index(col);
        bench_doc_intersection(index,patterns,"SORT",resfs);
    }
    {
        index_wt<> index(col);
        bench_doc_intersection(index,patterns,"WT",resfs);
    }
    {
        index_sada<> index(col);
        bench_doc_intersection(index,patterns,"SADA",resfs);
    }

    return 0;
}
