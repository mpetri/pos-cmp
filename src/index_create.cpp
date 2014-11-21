#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"

typedef struct cmdargs {
    std::string collection_dir;
} cmdargs_t;

void
print_usage(char* program)
{
    fprintf(stdout,"%s -c <collection directory> \n",program);
    fprintf(stdout,"where\n");
    fprintf(stdout,"  -c <collection directory>  : the directory the collection is stored.\n");
};

cmdargs_t
parse_args(int argc,char* const argv[])
{
    cmdargs_t args;
    int op;
    args.collection_dir = "";
    while ((op=getopt(argc,argv,"c:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
            case '?':
            default:
                print_usage(argv[0]);
        }
    }
    if (args.collection_dir=="") {
        std::cerr << "Missing command line parameters.\n";
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    return args;
}


int main(int argc, char* const argv[])
{
    // using clock = std::chrono::high_resolution_clock;

    /* parse command line */
    cmdargs_t args = parse_args(argc,argv);

    /* parse the collection */
    collection col(args.collection_dir);

    /* create index */
    using invidx_type = index_invidx<eliasfano_list<true>,eliasfano_list<false>>;
    index_abspos<eliasfano_list<true>,invidx_type> index(col);
    std::ofstream vofs(index.file_name+".html");
    sdsl::write_structure<sdsl::HTML_FORMAT>(index,vofs);

    return 0;
}