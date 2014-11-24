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

template<class t_idx>
int verify_index(t_idx& index,collection& col)
{
    /* verify inverted index */
    {
        sdsl::int_vector_mapper<> D(col.file_map[KEY_D]);
        sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
        size_t csum = C[0] + C[1];
        for (size_t i=2; i<C.size(); i++) {
            size_t n = C[i];
            std::vector<uint32_t> tmp(D.begin()+csum,D.begin()+csum+ n);
            LOG_EVERY_N(C.size()/10, INFO) << "Verify invidx list " << 100*i/C.size() << "% (" << n << ")";
            std::sort(tmp.begin(),tmp.end());

            auto itrs = index.m_docidx.list(i);
            auto id_itrs = itrs.first;
            auto freq_itrs = itrs.second;

            // check freqs
            auto ftmp = freq_itrs.first;
            auto fend = freq_itrs.second;
            size_t cur = 0;
            while (ftmp != fend) {
                auto cur_freq = *ftmp;
                auto first = tmp[cur];
                for (size_t i=1; i<cur_freq; i++) {
                    if (tmp[cur+i] != first) {
                        std::cerr << "ERROR IN FREQS OF id <" << i << ">" << std::endl;
                        return -1;
                    }
                }
                cur += cur_freq;
                ++ftmp;
            }

            // check ids
            auto last = std::unique(tmp.begin(),tmp.end());
            auto un = std::distance(tmp.begin(),last);
            auto curid = tmp.begin();
            auto itmp = id_itrs.first;
            auto iend = id_itrs.second;
            if ((size_t)un != itmp.size()) {
                LOG_N_TIMES(5,ERROR) << "ERROR IN IDS SIZE of id <" << i << ">";
            }
            if ((size_t)un != ftmp.size()) {
                LOG_N_TIMES(5,ERROR) << "ERROR IN FREQ SIZE of id <" << i << ">";
                continue;
            }
            while (itmp != iend) {
                if (*itmp != *curid) {
                    LOG_N_TIMES(5,ERROR) << "ERROR IN IDS OF id <" << i << ">";
                    return -1;
                }
                ++curid;
                ++itmp;
            }
            csum += n;
        }
        LOG(INFO) << "INVIDX - OK.";
    }
    /* verify the position index */
    {
        sdsl::int_vector_mapper<> POSPL(col.file_map[KEY_POSPL]);
        sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
        size_t csum = C[0] + C[1];
        for (size_t i=2; i<C.size(); i++) {
            size_t n = C[i];
            LOG_EVERY_N(C.size()/10, INFO) << "Verify abspos list " << 100*i/C.size() << "% (" << n << ")";
            auto begin = POSPL.begin()+csum;
            auto end = POSPL.begin()+csum+n;

            auto itrs = index.list(i);

            // check ids
            auto un = std::distance(begin,end);
            auto curid = begin;
            auto itmp = itrs.first;
            auto iend = itrs.second;
            if ((size_t)un != itmp.size()) {
                LOG(ERROR) << "ERROR IN IDS SIZE of id <" << i << ">: " << un << " - " << itmp.size();
                return -1;
            }
            while (itmp != iend) {
                if (*itmp != *curid) {
                    LOG(ERROR) << "ERROR IN IDS OF id <" << i << ">";
                    return -1;
                }
                ++curid;
                ++itmp;
            }
            csum += n;
        }
        LOG(INFO) << "ABSPOSIDX - OK.";
    }
    return 0;
}



int main(int argc,const char* argv[])
{
    _START_EASYLOGGINGPP(argc,argv);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime : %msg");

    /* parse command line */
    cmdargs_t args = parse_args(argc,argv);

    /* parse the collection */
    collection col(args.collection_dir);

    /* create index */
    {
        using invidx_type = index_invidx<optpfor_list<128,true>,optpfor_list<128,false>>;
        index_abspos<eliasfano_list<true>,invidx_type> index(col);
        verify_index(index,col);
    }
    {
        using invidx_type = index_invidx<eliasfano_list<true>,optpfor_list<128,false>>;
        index_abspos<eliasfano_list<true>,invidx_type> index(col);
        verify_index(index,col);
    }
    return 0;
}
