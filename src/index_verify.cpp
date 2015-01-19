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
        LOG(INFO) << "VERIFY INVIDX";
        sdsl::int_vector_mapper<> D(col.file_map[KEY_D]);
        sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
        size_t csum = C[0] + C[1];
        for (size_t i=2; i<C.size(); i++) {
            size_t n = C[i];
            std::vector<uint32_t> tmp(D.begin()+csum,D.begin()+csum+ n);
            LOG_EVERY_N(C.size()/10, INFO) << "Verify invidx list " << 100*i/C.size() << "% (" << n << ")";
            std::sort(tmp.begin(),tmp.end());

            auto lists = index.m_docidx.list(i);
            auto id_list = lists.first;
            auto freq_list = lists.second;

            // check freqs
            auto ftmp = freq_list.begin();
            auto fend = freq_list.end();
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
            auto itmp = id_list.begin();
            auto iend = id_list.end();
            if ((size_t)un != itmp.size()) {
                LOG_N_TIMES(5,ERROR) << "ERROR IN IDS SIZE of id <" << i << ">";
            }
            if ((size_t)un != ftmp.size()) {
                LOG_N_TIMES(5,ERROR) << "ERROR IN FREQ SIZE of id <" << i << ">";
                continue;
            }
            size_t offset = 0;
            while (itmp != iend) {
                if (*itmp != *curid) {
                    LOG_N_TIMES(5,ERROR) << "ERROR IN IDS OF id <" << i << "> at offset="<<offset<< " : should be: '" << *curid << " is '" << *itmp << "'";
                    return -1;
                }
                ++curid;
                ++itmp;
                offset++;
            }
            csum += n;
        }
        LOG(INFO) << "INVIDX - OK.";
    }
    /* verify the position index */
    {
        LOG(INFO) << "VERIFY ABSPOS";
        sdsl::int_vector_mapper<> POSPL(col.file_map[KEY_POSPL]);
        sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
        size_t csum = C[0] + C[1];
        for (size_t i=2; i<C.size(); i++) {
            size_t n = C[i];
            LOG_EVERY_N(C.size()/10, INFO) << "Verify abspos list " << 100*i/C.size() << "% (" << n << ")";
            auto begin = POSPL.begin()+csum;
            auto end = POSPL.begin()+csum+n;

            auto list = index.list(i);

            // check ids
            auto un = std::distance(begin,end);
            auto curid = begin;
            auto itmp = list.begin();
            auto iend = list.end();

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

template<class t_idx>
void verify_rel_index(t_idx& index,collection& col)
{
    LOG(INFO) << "VERIFY REL IDX";
    sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXTPERM]);
    sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
    for (size_t i=2; i<C.size(); i++) {
        auto list = index.list(i);
        auto iilists = index.m_docidx.list(i);
        auto id_list = iilists.first;
        auto freq_list = iilists.second;

        auto itr = list.begin();

        auto id_itr = id_list.begin();
        auto id_end = id_list.end();
        auto freq_itr = freq_list.begin();

        auto sym = i;
        bool error = false;
        while (id_itr != id_end) {
            auto doc_id = *id_itr;
            auto freq = *freq_itr;
            auto doc_start = index.doc_start(doc_id);

            auto prev = 0;
            for (size_t j=0; j<freq; j++) {
                auto pos = *itr;
                auto text_pos = doc_start + pos + prev;

                if (text[text_pos] != sym) {
                    LOG(ERROR) << j << " ERROR with sym ("<<sym<<")  at pos " << text_pos;
                    LOG(ERROR) << "text[] = " << text[text_pos-1] << " , " << text[text_pos] << " , " << text[text_pos+1];
                    error = true;
                }
                prev = prev+pos;
                ++itr;
            }
            if (error) return;

            ++freq_itr;
            ++id_itr;
        }

        if (error) return;
    }
    LOG(INFO) << "REL IDX - OK.";
}


template<class t_idx>
void verify_nextword_index(t_idx& index,collection& col)
{
    LOG(INFO) << "VERIFY NEXTWORD";
    sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXTPERM]);
    sdsl::int_vector_mapper<> CC(col.file_map[KEY_CC]);
    for (size_t i=0; i<CC.size(); i++) {
        uint64_t sym2 = CC[i] & ((1 << text.width())-1);
        uint64_t sym1 = CC[i] >> text.width();

        if (index.exists(CC[i]) == false) {
            LOG(ERROR) << "ERROR IN id <" << i << ">: " << CC[i] << " <" << sym1 << "," << sym2 << ">";
            continue;
        } else {
            auto list = index.list(CC[i]);
            auto itr = list.begin();
            auto end = list.end();
            bool error = false;
            while (itr != end) {
                auto text_pos = *itr;
                if (text[text_pos] != sym1 || text[text_pos+1] != sym2) {
                    LOG(ERROR) << i << " ERROR with phrase ("<<CC[i]<<") <"<< sym1 << "," << sym2 << "> at pos << " << text_pos;
                    LOG(ERROR) << "text[] = " << text[text_pos-1] << " , " << text[text_pos] << " , " << text[text_pos+1];
                    error = true;
                }
                ++itr;
            }
            if (error) return;
        }
    }
    LOG(INFO) << "NEXTWORD - OK.";
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
        index_abspos<eliasfano_skip_list<64,true>,invidx_type> index(col);
        verify_index(index,col);
    }
    // {
    //     using invidx_type = index_invidx<eliasfano_list<true>,optpfor_list<128,false>>;
    //     index_abspos<eliasfano_list<true>,invidx_type> index(col);
    //     verify_index(index,col);
    // }
    // {
    //     using invidx_type = index_invidx<eliasfano_list<true>,eliasfano_list<false>>;
    //     index_nextword<eliasfano_list<true>,invidx_type> index(col);
    //     verify_nextword_index(index,col);
    // }
    // {
    //     using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    //     index_abspos<uniform_eliasfano_list<128>,invidx_type> index(col);
    //     verify_index(index,col);
    // }
    // {
    //     using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    //     index_relpos<optpfor_list<128,false>,invidx_type> index(col);
    //     verify_rel_index(index,col);
    // }
    // {
    //     using invidx_type = index_invidx<uniform_eliasfano_list<128>,optpfor_list<128,false>>;
    //     index_relpos<eliasfano_list<false>,invidx_type> index(col);
    //     verify_rel_index(index,col);
    // }

    return 0;
}
