#pragma once

struct pattern_t {
    size_t id;
    size_t bucket;
    size_t sp;
    size_t ep;
    size_t ndoc;
    size_t nocc;
    size_t m;
    size_t min_list_size;
    size_t list_size_sum;
    std::vector<uint64_t> tokens;
    pattern_t(const std::string& pline,size_t _id,bool old_format = false) : id(_id)
    {
        std::istringstream input(pline);
        std::vector<std::string> elems;
        std::string item;
        while (std::getline(input, item, ';')) {
            elems.push_back(item);
        }
        // parse elems

        if (old_format) {
            bucket = std::stoull(elems[0]);
            ndoc = std::stoull(elems[1]);
            m = std::stoull(elems[2]);
            sp = std::stoull(elems[3]);
            ep = std::stoull(elems[4]);
            nocc = std::stoull(elems[5]);
            std::istringstream pinput(elems[6]);
            std::string pitem;
            while (std::getline(pinput, pitem, ',')) {
                tokens.push_back(std::stoull(pitem));
            }
        } else {
            id = std::stoull(elems[0]);
            bucket = std::stoull(elems[1]);
            m = std::stoull(elems[2]);
            sp = std::stoull(elems[3]);
            ep = std::stoull(elems[4]);
            ndoc = std::stoull(elems[5]);
            nocc = std::stoull(elems[6]);
            list_size_sum = std::stoull(elems[7]);
            min_list_size = std::stoull(elems[8]);
            std::istringstream pinput(elems[9]);
            std::string pitem;
            while (std::getline(pinput, pitem, ',')) {
                tokens.push_back(std::stoull(pitem));
            }
        }
    }
};

struct pattern_parser {
    template<bool t_old_format>
    static std::vector<pattern_t> parse_file(const std::string& pfile)
    {
        std::vector<pattern_t> patterns;
        std::ifstream pfs(pfile);
        if (pfs.is_open()) {
            size_t i = 1;
            for (std::string line; std::getline(pfs, line);) {
                patterns.emplace_back(pattern_t(line,i++,t_old_format));
            }
        } else {
            throw std::runtime_error("pattern_parser: cannot open pattern file.");
        }
        return patterns;
    }
};
