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

template<class t_cst>
void generate_patterns(t_cst& cst,collection& col,uint32_t min_size,uint32_t max_size,uint32_t max_patterns_per_bucket)
{
    sdsl::int_vector_mapper<> D(col.file_map[KEY_D]);
    std::multimap<uint64_t,typename t_cst::node_type> patterns;

    std::array<uint64_t,8> buckets = {0ULL,0ULL,0ULL,0ULL,0ULL,0ULL,0ULL};

    std::ofstream pofs(col.path+"/patterns/generated_occs.csv");

    /* try random node selection first */
    LOG(INFO) << "Pick random nodes ";
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0,cst.nodes()-1);
    sdsl::bit_vector nodes_tested(cst.nodes());
    for (size_t i=0; i<10000000; i++) {
        auto id = dis(gen);
        if (nodes_tested[id]==1) continue;
        nodes_tested[id] = 1;
        auto node = cst.inv_id(id);
        auto depth = cst.depth(node);
        if (depth >= min_size && depth <= max_size) {
            // determine how many unique docs are in that range
            auto sp = cst.lb(node);
            auto ep = cst.rb(node);
            std::vector<uint64_t> tmp(D.begin()+sp,D.begin()+ep+1);
            std::sort(tmp.begin(),tmp.end());
            auto last = std::unique(tmp.begin(),tmp.end());
            auto num_unique = std::distance(tmp.begin(),last);
            auto bucket = determine_bucket(num_unique);
            if (bucket != -1 && buckets[bucket] < max_patterns_per_bucket) {
                auto node_depth = cst.depth(node);
                std::vector<uint64_t> edge;
                for (size_t d=1; d<=node_depth; d++) {
                    edge.push_back((uint64_t)cst.edge(node,d));
                }
                pofs << bucket << ";"
                     << num_unique << ";"
                     << node_depth << ";"
                     << sp << ";"
                     << ep << ";"
                     << ep-sp+1 << ";";
                for (size_t i=0; i<edge.size()-1; i++) {
                    pofs << edge[i] << ",";
                }
                pofs << edge.back() << std::endl;
                buckets[bucket]++;
            }
        }
        if (i%100000==0) {
            std::cout << i << " (" << i/(100000.0f)  << ") [";
            for (size_t l=0; l<buckets.size(); l++) {
                std::cout << buckets[l] << " ";
            }
            std::cout << "]" << std::endl;
        }
    }

    /* pick some random nodes and perform a dfs traversal */
    LOG(INFO) << "Pick random nodes and perform DFS";
    for (size_t j=0; j<10000000; j++) {
        auto id = dis(gen);
        if (nodes_tested[id]==1) continue;
        nodes_tested[id] = 1;
        auto node = cst.inv_id(id);
        auto depth = cst.depth(node);
        if (depth <= max_size) {
            auto itr = cst.begin(node);
            auto end = cst.end(node);
            for (; itr!=end; ++itr) {
                if (itr.visit()==1) {
                    auto dfs_node = *itr;
                    auto dfs_node_id = cst.id(dfs_node);
                    if (nodes_tested[dfs_node_id]==1) continue;
                    nodes_tested[dfs_node_id] = 1;
                    auto node_depth = cst.depth(dfs_node);
                    if (node_depth > max_size) {
                        itr.skip_subtree();
                    }
                    if (node_depth >= min_size && node_depth <= max_size) {
                        auto sp = cst.lb(dfs_node);
                        auto ep = cst.rb(dfs_node);
                        std::vector<uint64_t> tmp(D.begin()+sp,D.begin()+ep+1);
                        std::sort(tmp.begin(),tmp.end());
                        auto last = std::unique(tmp.begin(),tmp.end());
                        auto num_unique = std::distance(tmp.begin(),last);
                        auto bucket = determine_bucket(num_unique);
                        if (bucket != -1 && buckets[bucket] < max_patterns_per_bucket) {
                            std::vector<uint64_t> edge;
                            for (size_t d=1; d<=node_depth; d++) {
                                edge.push_back((uint64_t)cst.edge(dfs_node,d));
                            }
                            pofs << bucket << ";"
                                 << num_unique << ";"
                                 << node_depth << ";"
                                 << sp << ";"
                                 << ep << ";"
                                 << ep-sp+1 << ";";
                            for (size_t i=0; i<edge.size()-1; i++) {
                                pofs << edge[i] << ",";
                            }
                            pofs << edge.back() << std::endl;
                            buckets[bucket]++;
                        }
                    }
                }
            }
        }
        if (j%100000==0) {
            std::cout << j << " (" << j/(100000.0f)  << ") [";
            for (size_t l=0; l<buckets.size(); l++) {
                std::cout << buckets[l] << " ";
            }
            std::cout << "]" << std::endl;
        }
    }

    /* then iteratre */
    LOG(INFO) << "Perform BFS";
    typedef sdsl::cst_bfs_iterator<t_cst> iterator;
    iterator begin = iterator(&cst, cst.root());
    iterator end   = iterator(&cst, cst.root(), true, true);
    size_t max_nodes_examined = 10000000;
    for (iterator it = begin; it != end; ++it) {
        auto node = *it;
        auto id = cst.id(node);
        if (nodes_tested[id]==1) continue;
        auto depth = cst.depth(node);
        if (depth >= min_size && depth <= max_size) {
            // determine how many unique docs are in that range
            auto sp = cst.lb(node);
            auto ep = cst.rb(node);
            std::vector<uint64_t> tmp(D.begin()+sp,D.begin()+ep+1);
            std::sort(tmp.begin(),tmp.end());
            auto last = std::unique(tmp.begin(),tmp.end());
            auto num_unique = std::distance(tmp.begin(),last);
            auto bucket = determine_bucket(num_unique);
            if (bucket != -1 && buckets[bucket] < max_patterns_per_bucket) {
                auto node_depth = cst.depth(node);
                std::vector<uint64_t> edge;
                for (size_t d=1; d<=node_depth; d++) {
                    edge.push_back((uint64_t)cst.edge(node,d));
                }
                pofs << bucket << ";"
                     << num_unique << ";"
                     << node_depth << ";"
                     << sp << ";"
                     << ep << ";"
                     << ep-sp+1 << ";";
                for (size_t i=0; i<edge.size()-1; i++) {
                    pofs << edge[i] << ",";
                }
                pofs << edge.back() << std::endl;
                buckets[bucket]++;
            }
        }
        if (max_nodes_examined%100000==0) {
            std::cout << (10000000-max_nodes_examined) << " (" << (10000000-max_nodes_examined)/(100000.0f)  << ") [";
            for (size_t l=0; l<buckets.size(); l++) {
                std::cout << buckets[l] << " ";
            }
            std::cout << "]" << std::endl;
        }
        max_nodes_examined--;
        if (max_nodes_examined == 0) return;
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

    /* create cst */
    sdsl::cst_sct3<sdsl::csa_sada_int<>> cst;
    if (col.file_map.count(KEY_CST) == 0) {
        LOG(INFO) << "Construct CST ";
        sdsl::cache_config cfg;
        cfg.delete_files = false;
        cfg.dir = col.path + "/tmp/";
        cfg.id = "TMP";
        cfg.file_map[sdsl::conf::KEY_SA] = col.file_map[KEY_SA];
        cfg.file_map[sdsl::conf::KEY_TEXT_INT] = col.file_map[KEY_TEXTPERM];
        construct(cst,col.file_map[KEY_TEXTPERM],cfg,0);
        auto cst_path = col.path+"/"+KEY_PREFIX+KEY_CST;
        LOG(INFO) << "Store CST ";
        sdsl::store_to_file(cst,cst_path);
        col.file_map[KEY_CST] = cst_path;
    } else {
        LOG(INFO) << "Load CST ";
        sdsl::load_from_file(cst,col.file_map[KEY_CST]);
    }

    /* walk cst to generate patterns */
    LOG(INFO) << "Walk CST to generate patterns";
    generate_patterns(cst,col,2,10,10000);


    return 0;
}