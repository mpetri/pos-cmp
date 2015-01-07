#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"

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
verify_intersection(const t_idx& index,const std::vector<pattern_t>& patterns,const collection& col) {
	sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXTPERM]);
	for(const auto& pattern : patterns) {
		LOG(INFO) << "check intersection for pattern " << pattern.id;
		size_t len = pattern.m;
		auto result = phrase_positions(pattern.tokens);
		if(result.size() != pattern.nocc) {
			LOG(ERROR) << "pattern=" << pattern.id << " nocc=" << pattern.nocc << " is=" << result.size();
		}
		for(const auto& pos : result) {
			for(size_t i=0;i<len;i++) {
				if(text[pos+i] != pattern.tokens[i]) {
					LOG(ERROR) << "pattern=" << pattern.id << " pos=" << pos << " offset=" << i;
				}
			}
		}
	}
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

    /* load pattern file */
    auto patterns = pattern_parser::parse_file(args.pattern_file);
    LOG(INFO) << "Parsed " << patterns.size() << " patterns from file " << args.pattern_file;

    /* load indexes and test */
    {
        using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
        index_abspos<uniform_eliasfano_list<128>,invidx_type> index(col);
        verify_intersection(index,patterns,col);
    }


    return 0;
}