#pragma once

struct pattern_t {
	size_t id;
	size_t bucket;
	size_t sp;
	size_t ep;
	size_t ndoc;
	size_t nocc;
	size_t m;
	std::vector<uint64_t> tokens;
	pattern_t(const std::string& pline,size_t _id) : id(_id) {
		std::istringstream input(pline);
		std::vector<std::string> elems;
		std::string item;
		while (std::getline(input, item, ';')) {
			elems.push_back(item);
		}
		// parse elems
		bucket = std::atoull(elems[0]);
		ndoc = std::atoull(elems[1]);
		m = std::atoull(elems[2]);
		sp = std::atoull(elems[3]);
		ep = std::atoull(elems[4]);
		nocc = std::atoull(elems[5]);

		std::istringstream pinput(elems[6]);
		std::string pitem;
		while (std::getline(pinput, pitem, ',')) {
			tokens.push_back(std::atoull(pitem));
		}
	}
};

struct pattern_parser {
	static std::vector<pattern_t> parse_file(const std::string& pfile) {
		std::vector<pattern_t> patterns;
		std::ifstream pfs(pfile);
		if(pfs.open()) {
			size_t i = 1;
			for (std::string line; std::getline(pfs, line); ) {
				patterns.emplace_back(pattern_t(line,i++));
			}
		} else {
			throw std::runtime_error("pattern_parser: cannot open pattern file.");
		}
		return patterns;
	}
};
