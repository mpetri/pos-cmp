#include <chrono>

#include "utils.hpp"
#include "collection.hpp"
#include "indexes.hpp"
#include "list_types.hpp"
#include "dict_map.hpp"
#include "intersection.hpp"

#include "easylogging++.h"
#include "zmq.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

_INITIALIZE_EASYLOGGINGPP

typedef struct cmdargs {
    std::string collection_dir;
    std::string port;
} cmdargs_t;

void
print_usage(const char* program)
{
    fprintf(stdout,"%s -c <collection directory> \n",program);
    fprintf(stdout,"where\n");
    fprintf(stdout,"  -c <collection directory>  : the directory the collection is stored.\n");
    fprintf(stdout,"  -p <port>  : the port the daemon is running on.\n");
};

cmdargs_t
parse_args(int argc,const char* argv[])
{
    cmdargs_t args;
    int op;
    args.collection_dir = "";
    args.port = std::to_string(5556);
    while ((op=getopt(argc,(char* const*)argv,"c:p:")) != -1) {
        switch (op) {
            case 'c':
                args.collection_dir = optarg;
                break;
            case 'p':
                args.port = optarg;
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
    using clock = std::chrono::high_resolution_clock;

    /* parse command line */
    LOG(INFO) << "Parsing command line arguments";
    cmdargs_t args = parse_args(argc,argv);

    /* parse the collection */
    LOG(INFO) << "Parsing collection directory " << args.collection_dir;
    collection col(args.collection_dir);

    /* create/load index */
    using invidx_type = index_invidx<eliasfano_list<true>,eliasfano_list<false>>;
    index_abspos<eliasfano_list<true>,invidx_type> index(col);

    /* load dict */
    dict_map dict(col);

    /* daemon mode */
    {
        std::cout << "Starting daemon mode on port " << args.port << std::endl;
        zmq::context_t context(1);
        zmq::socket_t server(context, ZMQ_REP);
        server.bind(std::string("tcp://*:"+args.port).c_str());

        while (true) {
            zmq::message_t request;
            /* wait for msg */
            server.recv(&request);
            const char* qry = (const char*) request.data();
            std::string qry_str(qry,request.size());

            auto parse_start = clock::now();
            auto parsed_qry = dict.parse_query(qry_str);
            auto parse_stop = clock::now();
            auto parse_time = parse_stop - parse_start;
            auto total_time = parse_time;

            // json output
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> json_writer(s);
            json_writer.StartObject();

            json_writer.String("qid");
            json_writer.Uint(parsed_qry.id);

            // perform query
            std::cout << "qry[" << parsed_qry << "]" << std::endl;
            if (parsed_qry.ids.size() > 0) {
                auto query_start = clock::now();
                auto doc_lists = index.doc_lists(parsed_qry.ids);
                auto res_list = intersect(doc_lists);
                auto query_stop = clock::now();
                auto query_time = query_stop - query_start;
                total_time += query_time;

                json_writer.String("ids");
                json_writer.StartArray();
                size_t num_ids = std::min((size_t)10,res_list.size());
                for (size_t i=0; i<num_ids; i++) {
                    json_writer.Uint(res_list[i]);
                }
                json_writer.EndArray();
            } else {
                json_writer.String("id");
                json_writer.StartArray();
                json_writer.EndArray();
            }
            // time
            json_writer.String("time");
            json_writer.Double(std::chrono::duration_cast<std::chrono::microseconds>(total_time).count()/1000.0);

            // send
            json_writer.EndObject();
            auto json_str = s.GetString();
            zmq::message_t reply(s.GetSize());
            memcpy(reply.data(),json_str,s.GetSize());
            std::cout << "reply = '" << json_str << "'" << std::endl;
            server.send(reply);

        }
    }


    return 0;
}