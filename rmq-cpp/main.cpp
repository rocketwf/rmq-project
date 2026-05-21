#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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

struct SparseArray {
	static std::string name() { return "SparseArray"; };
	static size_t max_n();  // optional, defaults to SIZE_MAX
	static SparseArray build(const std::vector<uint64_t>& data);
	size_t space() const;
	uint64_t query(size_t l, size_t r) const;
};

struct SegmentTree {
  static std::string name() { return "SegmentTree"; };
  static size_t max_n();  // optional, defaults to SIZE_MAX
  static SegmentTree build(const std::vector<uint64_t>& data);
  size_t space() const;
  uint64_t query(size_t l, size_t r) const;
};

struct BlockBased {
  static std::string name() { return "BlockBased"; };
  static size_t max_n();  // optional, defaults to SIZE_MAX
  static BlockBased build(const std::vector<uint64_t>& data);
  size_t space() const;
  uint64_t query(size_t l, size_t r) const;
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
		// TODO: Add other implementations here.
	}

	return 0;
}
