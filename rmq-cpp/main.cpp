#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <bit>
#include <cmath>

// RMQ interface (duck-typed via templates):
//
//   static std::string name();
//   static size_t max_n();               // optional, defaults to SIZE_MAX
//   static RMQ build(const std::vector<uint64_t>& data);
//   size_t space() const;
//   uint64_t query(size_t l, size_t r) const;

// Trivial implementation that computes each query on the fly.
struct Naive {
	static std::string name() { return "QuadraticQuery"; }
	// NOTE: Improved implementations should simply return size_t::MAX.
	static size_t max_n() { return 30'000; }

	const std::vector<uint64_t>* data;

	static Naive build(const std::vector<uint64_t>& data) { return {&data}; }

	size_t space() const { return sizeof(*this); }

	uint64_t query(size_t l, size_t r) const {
		uint64_t min = (*data)[l];
		for(size_t i = l + 1; i <= r; ++i) min = std::min(min, (*data)[i]);
		return min;
	}
};

struct Precompute {
	static std::string name() {return "PrecomputeQueries"; };

	static size_t max_n() { return 30'000; };

	size_t n;
	std::vector<uint64_t> lookup_table;

	static Precompute build(const std::vector<uint64_t>& data) {
		std::vector<uint64_t> lu;
		size_t n = data.size();

		lu.resize((n * n + n) / 2);

		size_t row_start = 0;

		for (size_t l = 0; l < n; l++) {
		lu[row_start] = data[l];
		for (size_t r = l + 1; r < n; r++) {
			lu[row_start + r - l] = std::min(lu[row_start + r - l - 1], data[r]);
		}
			row_start += n - l;
		}
		return {n, std::move(lu)};
	};

	size_t space() const {return sizeof(*this) + (lookup_table.capacity() * sizeof(uint64_t)); };

	uint64_t query(size_t l, size_t r) const {
	size_t row_start = l * n - (l * (l - 1)) / 2;
	return lookup_table[row_start + r - l];
	};
};

struct SparseArrayA {
	static std::string name() { return "SparseArrayA"; };
	static size_t max_n() { return 10'000'000; };
	std::vector<std::vector<uint64_t>> _tree;
	static SparseArrayA build(const std::vector<uint64_t>& data) {
		int k = std::bit_width(data.size()); // maximum level
		decltype(_tree) tree(k, std::vector<uint64_t>(data.size()));
		tree[0] = data;

		for (size_t lvl = 1; lvl < k; lvl++) {
			for (size_t i = 0; i + (1 << lvl) <= data.size(); i++) {
				tree[lvl][i] = std::min(tree[lvl - 1][i], 
																tree[lvl - 1][i + (1 << (lvl - 1))]);
			}
		}
		return {std::move(tree)};
	};
	size_t space() const {
		size_t mem = sizeof(*this);
		mem += _tree.capacity() * sizeof(std::vector<uint64_t>);
		for (const auto& row : _tree) {
			mem += row.capacity() * sizeof(uint64_t);
		}
		return mem;
	};
	uint64_t query(size_t l, size_t r) const {
		int k = std::bit_width(r - l + 1) - 1;
		return std::min(_tree[k][l], _tree[k][r - (1 << k) + 1]);
	};
};

struct SparseArrayB {
	static std::string name() { return "SparseArrayB"; }
	static size_t max_n() { return 10'000'000; } 

	std::vector<std::vector<uint64_t>> _tree;

	static SparseArrayB build(const std::vector<uint64_t>& data) {
		int k = std::bit_width(data.size()); // maximum level
		std::vector<std::vector<uint64_t>> tree(data.size(), std::vector<uint64_t>(k));
		for (size_t i = 0; i < data.size(); ++i) {
			tree[i][0] = data[i];
		}
		// Build the sparse table
		for (size_t lvl = 1; lvl < k; lvl++) {
			for (size_t i = 0; i + (1 << lvl) <= data.size(); i++) {
				tree[i][lvl] = std::min(tree[i][lvl - 1], 
				tree[i + (1 << (lvl - 1))][lvl - 1]);
			}
		}
		return {std::move(tree)};
	}
	size_t space() const {
		size_t mem = sizeof(*this);
		mem += _tree.capacity() * sizeof(std::vector<uint64_t>);
		for (const auto& row : _tree) {
			mem += row.capacity() * sizeof(uint64_t);
		}
		return mem;
	}
	uint64_t query(size_t l, size_t r) const {
		int k = std::bit_width(r - l + 1) - 1;
		return std::min(_tree[l][k], _tree[r - (1 << k) + 1][k]);
	}
};

struct SegmentTree {
	static std::string name() { return "SegmentTree"; };
	static size_t max_n() { return 10'000'000; };
	std::vector<std::vector<uint64_t>> _tree;

	static SegmentTree build(const std::vector<uint64_t>& data) {
		int k = std::bit_width(data.size()); // maximum level
		decltype(_tree) tree;

		if (!data.empty()) {
        tree.reserve(std::bit_width(data.size()) + 1);
    }

		tree.push_back(data);
		size_t lvl = 1;
    while (tree[lvl - 1].size() > 1) {
      size_t prev_size = tree[lvl - 1].size();
      size_t curr_size = (prev_size + 1) / 2;
      tree.emplace_back(curr_size, 0);
      for (size_t i = 0; i < curr_size; ++i) {
        if (2 * i + 1 < prev_size) {
          tree[lvl][i] = std::min(tree[lvl - 1][2 * i],
                                  tree[lvl - 1][2 * i + 1]);
        } else {
          tree[lvl][i] = tree[lvl - 1][2 * i];
        }
      }
      lvl++;
    }
    return {std::move(tree)};
	};

	size_t space() const {
		size_t mem = sizeof(*this);
		mem += _tree.capacity() * sizeof(std::vector<uint64_t>);
		for (const auto& row : _tree) {
		  mem += row.capacity() * sizeof(uint64_t);
		}
		return mem;
	}

  uint64_t query(size_t l, size_t r) const {
  	uint64_t res = std::numeric_limits<uint64_t>::max();
  	size_t lvl = 0;
  	while (l < r && lvl < _tree.size()) {
  	  if (l % 2 == 1) {
  	    res = std::min(res, _tree[lvl][l]);
  	    l++;
  	  }
  	  if (r % 2 == 1) {
  	    r--;
  	    res = std::min(res, _tree[lvl][r]);
  	  }
  	  l /= 2;
  	  r /= 2;
  	  lvl++;
  	}
  	return res;
  }
};

struct BlockBased {
	static std::string name() { return "BlockBased"; };
	static size_t max_n() { return 10'000'000; };
	std::vector<uint64_t> _data;
	std::vector<uint64_t> _blocks;
	size_t _block_size;

	static BlockBased build(const std::vector<uint64_t>& data) {
		size_t block_size = 2 << (static_cast<size_t>(std::log2(data.size())) / 2 - 1);
		size_t num_blocks = (data.size() + block_size - 1) / block_size;
		decltype(_blocks) blocks(num_blocks, std::numeric_limits<uint64_t>::max());
		for (size_t i = 0; i < data.size(); i++) {
			blocks[i / block_size] = std::min(blocks[i / block_size], data[i]);
		}
		//std::cout << "BS:" << block_size << "\n";
		return {data, std::move(blocks), block_size};
	};

	size_t space() const {
		return sizeof(*this) + (_data.capacity() + _blocks.capacity()) * sizeof(uint64_t);
	};
	uint64_t query(size_t l, size_t r) const {
		size_t min = std::numeric_limits<uint64_t>::max();
		size_t ld = l / _block_size;
		size_t rd = r / _block_size;

		if (ld == rd) {
      for (size_t i = l; i < r; ++i) {
        min = std::min(min, _data[i]);
      }
    } else {
      for (size_t i = l; i < (ld + 1) * _block_size; ++i) {
        min = std::min(min, _data[i]);
      }
      for (size_t i = ld + 1; i < rd; ++i) {
        min = std::min(min, _blocks[i]);
      }
      for (size_t i = rd * _block_size; i < r; ++i) {
        min = std::min(min, _data[i]);
      }
    }
		return min;
	};
};

struct CartesianTree {
	static std::string name() { return "CartesianTree"; };
	static size_t max_n();  // optional, defaults to SIZE_MAX
	static Precompute build(const std::vector<uint64_t>& data);
	size_t space() const;
	uint64_t query(size_t l, size_t r) const;
};

struct Input {
	std::vector<uint64_t> data;
	std::vector<std::pair<size_t, size_t>> queries;
};

// Read the given input file.
Input read_input(const std::filesystem::path& file) {
	std::ifstream f(file);
	size_t n, q;
	f >> n >> q;
	Input input;
	input.data.resize(n);
	for(auto& v : input.data) f >> v;
	input.queries.resize(q);
	for(auto& [l, r] : input.queries) f >> l >> r;
	return input;
}

// Bench the given RMQ implementation on the given input, and print results in CSV format.
template <typename RMQ>
void bench(const Input& input) {
	std::cerr << std::setw(10) << input.data.size() << "\t" << std::setw(20) << RMQ::name() << "\t";

	size_t max_n = RMQ::max_n();

	if(input.data.size() > max_n) {
		std::cerr << "skipped\n";
		return;
	}

	auto rmq = RMQ::build(input.data);
	std::cerr << std::setw(10) << rmq.space() << "\t";

	auto start   = std::chrono::high_resolution_clock::now();
	uint64_t sum = 0;
	for(auto& [l, r] : input.queries) sum += rmq.query(l, r);
	auto end = std::chrono::high_resolution_clock::now();

	double elapsed =
		static_cast<double>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()) /
		static_cast<double>(input.queries.size());

	std::cout << input.data.size() << "," << input.queries.size() << "," << RMQ::name() << ","
				<< rmq.space() << "," << sum << "," << elapsed << "\n";
	std::cerr << std::setw(3) << (sum % 1000) << "\t" << std::fixed << std::setprecision(2)
				<< elapsed << "ns/q\n";
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		std::cerr << "Usage: rmq-cpp <input_dir>\n";
		return 1;
	}

	std::cout << "n,q,name,space,sum,time\n";

	std::filesystem::path file_or_dir(argv[1]);
	std::cerr << "Reading input from " << file_or_dir << " ..\n";

	std::vector<Input> inputs;
	if(std::filesystem::is_regular_file(file_or_dir)) {
		inputs.push_back(read_input(file_or_dir));
	} else {
		for(auto& entry : std::filesystem::directory_iterator(file_or_dir)) {
			if(entry.path().extension() == ".in") inputs.push_back(read_input(entry.path()));
		}
		std::sort(inputs.begin(), inputs.end(),
					[](const Input& a, const Input& b) { return a.data.size() < b.data.size(); });
	}

	for(const auto& input : inputs) {
		bench<Naive>(input);
		bench<Precompute>(input);
		bench<SparseArrayA>(input);
		bench<SparseArrayB>(input);
		bench<SegmentTree>(input);
		bench<BlockBased>(input);
	}

	return 0;
}
