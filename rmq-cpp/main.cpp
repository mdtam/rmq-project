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
struct Naive
{
	static std::string name() { return "QuadraticQuery"; }
	// NOTE: Improved implementations should simply return size_t::MAX.
	static size_t max_n() { return 30'000; }

	const std::vector<uint64_t> *data;

	static Naive build(const std::vector<uint64_t> &data) { return {&data}; }

	size_t space() const { return sizeof(*this); }

	uint64_t query(size_t l, size_t r) const
	{
		uint64_t min = (*data)[l];
		for (size_t i = l + 1; i <= r; ++i)
			min = std::min(min, (*data)[i]);
		return min;
	}
};

struct SegTree
{
	static std::string name() { return "Segment Tree (opt.)"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> segtree;

	static SegTree build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		std::vector<uint64_t> segtree(sz);
		for (size_t i = sz - 1; i > 0; i--)
		{
			uint64_t a = (i << 1) >= sz ? data[(i << 1) - sz] : segtree[(i << 1)];
			uint64_t b = ((i << 1) | 1) >= sz ? data[((i << 1) | 1) - sz] : segtree[(i << 1) | 1];
			segtree[i] = std::min(a, b);
		}
		return {&data, segtree};
	}

	size_t space() const { return sizeof(*this) + (segtree.size() * sizeof(uint64_t)); }

	uint64_t query(size_t l, size_t r) const
	{
		uint64_t mn = std::min((*data)[l], (*data)[r]);
		size_t sz = segtree.size();
		l = (l + sz + 1) >> 1;
		r = (r + sz - 1) >> 1;
		while (l <= r)
		{
			mn = std::min({mn, segtree[l], segtree[r]});
			l = (l + 1) >> 1;
			r = (r - 1) >> 1;
		}
		return mn;
	}
};

struct PreComputeAllAnswers
{
	static std::string name() { return "PreCompute Answers"; }
	static size_t max_n() { return 10'000; }

	const std::vector<uint64_t> *data;
	const std::vector<std::vector<uint64_t>> ans;

	static PreComputeAllAnswers build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		std::vector<std::vector<uint64_t>> ans(sz, std::vector<uint64_t>(sz));
		for (size_t i = 0; i < sz; i++)
		{
			ans[i][i] = data[i];
			for (size_t j = i + 1; j < sz; j++)
			{
				ans[i][j] = std::min(data[j], ans[i][j - 1]);
			}
		}
		return {&data, ans};
	}

	size_t space() const { return sizeof(*this) + (ans.size() * ans[0].size() * sizeof(uint64_t)); }

	uint64_t query(size_t l, size_t r) const
	{
		return ans[l][r];
	}
};

// -------------------------------------------------------------
// TODO: Implement the RMQ interface for additional data structures.
// -------------------------------------------------------------

struct Input
{
	std::vector<uint64_t> data;
	std::vector<std::pair<size_t, size_t>> queries;
};

// Read the given input file.
Input read_input(const std::filesystem::path &file)
{
	std::ifstream f(file);
	size_t n, q;
	f >> n >> q;
	Input input;
	input.data.resize(n);
	for (auto &v : input.data)
		f >> v;
	input.queries.resize(q);
	for (auto &[l, r] : input.queries)
		f >> l >> r;
	return input;
}

// Bench the given RMQ implementation on the given input, and print results in CSV format.
template <typename RMQ>
void bench(const Input &input)
{
	std::cerr << std::setw(10) << input.data.size() << "\t" << std::setw(20) << RMQ::name() << "\t";

	size_t max_n = RMQ::max_n();

	if (input.data.size() > max_n)
	{
		std::cerr << "skipped\n";
		return;
	}

	auto rmq = RMQ::build(input.data);
	std::cerr << std::setw(10) << rmq.space() << "\t";

	auto start = std::chrono::high_resolution_clock::now();
	uint64_t sum = 0;
	for (auto &[l, r] : input.queries)
		sum += rmq.query(l, r);
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

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: rmq-cpp <input_dir>\n";
		return 1;
	}

	std::cout << "n,q,name,space,sum,time\n";

	std::filesystem::path file_or_dir(argv[1]);
	std::cerr << "Reading input from " << file_or_dir << " ..\n";

	std::vector<Input> inputs;
	if (std::filesystem::is_regular_file(file_or_dir))
	{
		inputs.push_back(read_input(file_or_dir));
	}
	else
	{
		for (auto &entry : std::filesystem::directory_iterator(file_or_dir))
		{
			if (entry.path().extension() == ".in")
				inputs.push_back(read_input(entry.path()));
		}
		std::sort(inputs.begin(), inputs.end(),
				  [](const Input &a, const Input &b)
				  { return a.data.size() < b.data.size(); });
	}

	for (const auto &input : inputs)
	{
		bench<Naive>(input);
		bench<SegTree>(input);
		bench<PreComputeAllAnswers>(input);
	}

	return 0;
}
