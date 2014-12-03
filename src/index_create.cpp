#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"

#include "easylogging++.h"

_INITIALIZE_EASYLOGGINGPP

typedef struct cmdargs {
    std::string collection_dir;
} cmdargs_t;

void
print_usage(const char* program)
{
    fprintf(stdout,"%s -c <collection directory> \n",program);
    fprintf(stdout,"where\n");
    fprintf(stdout,"  -c <collection directory>  : the directory the collection is stored.\n");
};

cmdargs_t
parse_args(int argc,const char* argv[])
{
    cmdargs_t args;
    int op;
    args.collection_dir = "";
    while ((op=getopt(argc,(char* const*)argv,"c:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
        }
    }
    if (args.collection_dir=="") {
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
    // using clock = std::chrono::high_resolution_clock;

    /* parse command line */
    LOG(INFO) << "Parsing command line arguments";
    cmdargs_t args = parse_args(argc,argv);

    /* parse the collection */
    LOG(INFO) << "Parsing collection directory " << args.collection_dir;
    collection col(args.collection_dir);

    /* create index */
    using invidx_type = index_invidx<eliasfano_list<true>,eliasfano_list<false>>;
    {
        index_abspos<optpfor_list<128,true>,invidx_type> index(col);
    }
    {
        index_nextword<optpfor_list<128,true>,invidx_type> index(col);
    }

    return 0;
}