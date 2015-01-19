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

int main(int argc,const char* argv[])
{
    _START_EASYLOGGINGPP(argc,argv);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime : %msg");

    LOG(INFO) << "Parsing command line arguments";
    cmdargs_t args = parse_args(argc,argv);

    LOG(INFO) << "Parsing collection directory " << args.collection_dir;
    collection col(args.collection_dir);

    auto patterns = pattern_parser::parse_file<true>(args.pattern_file);
    LOG(INFO) << "Parsed " << patterns.size() << " patterns from file " << args.pattern_file;

    /* create index */
    using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
    index_abspos<eliasfano_list<true>,invidx_type> index(col);

    std::ofstream ofs(args.pattern_file);
    for (const auto& pattern : patterns) {

        size_t min_list_size = std::numeric_limits<size_t>::max();
        size_t list_size_sum = 0;
        for (const auto& t : pattern.tokens) {
            auto lst = index.list(t);
            min_list_size = std::min(min_list_size,lst.size());
            list_size_sum += lst.size();
        }

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
