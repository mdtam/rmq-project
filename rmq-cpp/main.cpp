#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cmath>

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
		// std::vector<uint64_t> segtree2(sz);
		// for (size_t i = sz - 1; i > 0; i--)
		// {
		// 	uint64_t a = (i << 1) >= sz ? data[(i << 1) - sz] : segtree2[(i << 1)];
		// 	uint64_t b = ((i << 1) | 1) >= sz ? data[((i << 1) | 1) - sz] : segtree2[(i << 1) | 1];
		// 	segtree2[i] = std::min(a, b);
		// }

		for (size_t i = sz - 1; i > (sz >> 1); i--)
		{
			segtree[i] = std::min(data[(i << 1) - sz], data[((i << 1) | 1) - sz]);
		}

		if (sz & 1)
		{
			segtree[sz >> 1] = std::min(segtree[sz - 1], data[0]);
		}
		else
		{
			segtree[sz >> 1] = std::min(data[0], data[1]);
		}

		for (size_t i = (sz >> 1) - 1; i > 0; i--)
		{
			segtree[i] = std::min(segtree[(i << 1)], segtree[(i << 1) | 1]);
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

struct SegTreeNode
{
	SegTreeNode *lft = nullptr;
	SegTreeNode *rgt = nullptr;
	SegTreeNode(size_t left, size_t right) { l = left, r = right; }
	size_t l, r;
	uint64_t min;

	size_t calculate_space() const
	{
		size_t total = sizeof(SegTreeNode);
		if (lft)
			total += lft->calculate_space();
		if (rgt)
			total += rgt->calculate_space();
		return total;
	}
};

struct SegTreePointers
{
	static std::string name() { return "Segment Tree (Pointers)"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	SegTreeNode *segtree;

	static SegTreePointers build(const std::vector<uint64_t> &data)
	{
		SegTreePointers ret = {&data, new SegTreeNode(0, data.size() - 1)};
		for (size_t i = 0; i < data.size(); i++)
			ret.update(i);
		return ret;
	}

	size_t space() const
	{
		size_t res = sizeof(*this);
		res += segtree->calculate_space();
		return res;
	}

	uint64_t query(size_t l, size_t r, SegTreeNode *node = nullptr) const
	{
		SegTreeNode *nod = (node == nullptr) ? segtree : node;
		if (nod->l >= l && nod->r <= r)
		{
			return nod->min;
		}
		if (nod->l == nod->r)
		{
			return -1;
		}
		size_t mid = (nod->l + nod->r) / 2;
		uint64_t res = (*data)[l];
		if (l <= mid)
		{
			res = std::min(res, query(l, r, nod->lft));
		}
		if (r > mid)
		{
			res = std::min(res, query(l, r, nod->rgt));
		}
		return res;
	}

	void update(size_t idx, SegTreeNode *node = nullptr) const
	{
		SegTreeNode *nod = (node == nullptr) ? segtree : node;
		if (nod->l == nod->r && nod->l == idx)
		{
			nod->min = (*data)[idx];
			return;
		}
		size_t mid = (nod->l + nod->r) / 2;
		if (idx <= mid)
		{
			if (nod->lft == nullptr)
				nod->lft = new SegTreeNode(nod->l, mid);
			update(idx, nod->lft);
			nod->min = (nod->rgt != nullptr) ? std::min(nod->lft->min, nod->rgt->min) : nod->lft->min;
		}
		else
		{
			if (nod->rgt == nullptr)
				nod->rgt = new SegTreeNode(mid + 1, nod->r);
			update(idx, nod->rgt);
			nod->min = (nod->lft != nullptr) ? std::min(nod->lft->min, nod->rgt->min) : nod->rgt->min;
		}
	}
};

struct PreComputeAllAnswers
{
	static std::string name() { return "PreCompute Answers"; }
	static size_t max_n() { return 10'000; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> ans;

	static PreComputeAllAnswers build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		std::vector<uint64_t> ans(sz * sz);
		for (size_t i = 0; i < sz; i++)
		{
			ans[i * sz + i] = data[i];
			for (size_t j = i + 1; j < sz; j++)
			{
				ans[i * sz + j] = std::min(data[j], ans[i * sz + j - 1]);
			}
		}
		return {&data, ans};
	}

	size_t space() const { return sizeof(*this) + (ans.size() * sizeof(uint64_t)); }

	uint64_t query(size_t l, size_t r) const
	{
		return ans[l * (*data).size() + r];
	}
};

struct SparseTable
{
	static std::string name() { return "Sparse Table"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> table;
	const size_t sz;
	const size_t lg;

	static SparseTable build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		size_t lg = 32 - __builtin_clz((unsigned int)sz);
		std::vector<uint64_t> table(lg * sz);
		for (size_t j = 0; j < sz; j++)
		{
			table[j] = data[j];
		}
		for (size_t i = 1; i < lg; i++)
		{
			size_t prev_offset = (i - 1) * sz;
			size_t curr_offset = i * sz;
			size_t step = (1 << (i - 1));

			for (size_t j = 0; j + step < sz; j++)
			{
				table[curr_offset + j] = std::min(table[prev_offset + j], table[prev_offset + j + step]);
			}
		}
		return {&data, table, sz, lg};
	}

	size_t space() const { return sizeof(*this) + (table.size() * sizeof(uint64_t)); }

	uint64_t query(size_t l, size_t r) const
	{
		size_t range = (r - l + 1);
		size_t k = 31 - __builtin_clz(range);
		return std::min(table[k * sz + l], table[k * sz + r - (1 << k) + 1]);
	}
};

struct SqrtBlocks
{
	static std::string name() { return "Sqrt Blocks"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> blocks;
	const size_t sz, sq;

	static SqrtBlocks build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		size_t sq = std::sqrt(sz);
		std::vector<uint64_t> blocks((sz + sq - 1) / sq, -1);
		for (size_t i = 0; i < sz; i++)
		{
			size_t idx = i / sq;
			if (sq * idx == i)
				blocks[idx] = data[i];
			else
				blocks[idx] = std::min(blocks[idx], data[i]);
		}
		return {&data, blocks, sz, sq};
	}

	size_t space() const { return sizeof(*this) + (blocks.size() * sizeof(uint64_t)); }

	uint64_t query(size_t l, size_t r) const
	{
		size_t ldx = l / sq;
		size_t cur = l;
		uint64_t mn = (*data)[l];

		if (ldx * sq < l)
		{
			size_t pre = (ldx + 1) * sq;
			while (cur < std::min(pre, r))
			{
				mn = std::min(mn, (*data)[cur]);
				cur++;
			}
			ldx++;
		}
		while (cur + sq <= r + 1)
		{
			mn = std::min(mn, blocks[ldx++]);
			cur += sq;
		}
		while (cur <= r)
		{
			mn = std::min(mn, (*data)[cur]);
			cur++;
		}
		return mn;
	}
};

struct SqrtBlocksPrefix
{
	static std::string name() { return "Sqrt Blocks (prefix)"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> blocks;
	const std::vector<uint64_t> pre;
	const std::vector<uint64_t> suf;
	const size_t sq;
	const bool save_data;
	const std::vector<uint64_t> saved_data;

	static SqrtBlocksPrefix build(const std::vector<uint64_t> &data, const bool save_data = false)
	{
		size_t sz = data.size();
		size_t sq = std::sqrt(sz);
		std::vector<uint64_t> blocks((sz + sq - 1) / sq, -1);
		std::vector<uint64_t> pre(sz);
		std::vector<uint64_t> suf(sz);
		for (size_t i = 0; i < sz; i++)
		{
			size_t idx = i / sq;
			if (idx * sq == i)
			{
				blocks[idx] = data[i];
				pre[i] = data[i];
			}
			else
			{
				blocks[idx] = std::min(blocks[idx], data[i]);
				pre[i] = std::min(data[i], pre[i - 1]);
			}
		}

		for (int i = sz - 1; i >= 0; i--)
		{
			size_t idx = i / sq;
			if (i == sz - 1 || (i - idx * sq) == sq - 1)
				suf[i] = data[i];
			else
				suf[i] = std::min(data[i], suf[i + 1]);
		}
		if (save_data)
			return {&data, blocks, pre, suf, sq, save_data, data};
		return {&data, blocks, pre, suf, sq, save_data};
	}

	size_t space() const
	{
		size_t ret = sizeof(*this);
		ret += ((blocks.size() + pre.size() + suf.size()) * sizeof(uint64_t));
		ret += save_data ? (saved_data.size() * sizeof(uint64_t)) : 0;
		return ret;
	}

	uint64_t query(size_t l, size_t r) const
	{
		size_t ldx = l / sq, rdx = r / sq;
		if (ldx == rdx)
		{
			uint64_t mn = save_data ? saved_data[l] : (*data)[l];
			for (size_t i = l + 1; i <= r; i++)
			{
				mn = std::min(mn, save_data ? saved_data[i] : (*data)[i]);
			}
			return mn;
		}
		uint64_t mn = std::min(suf[l], pre[r]);
		for (size_t i = ldx + 1; i < rdx; i++)
		{
			mn = std::min(mn, blocks[i]);
		}
		return mn;
	}
};

struct MultilevelSqrtBlocks
{
	static std::string name() { return "Multilevel Sqrt Blocks"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> blocks;
	const std::vector<uint64_t> pre;
	const std::vector<uint64_t> suf;
	const size_t sq;
	const SqrtBlocksPrefix l2_blocks;

	static MultilevelSqrtBlocks build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();
		size_t sq = std::sqrt(std::sqrt(sz));
		std::vector<uint64_t> blocks((sz + sq - 1) / sq, -1);
		std::vector<uint64_t> pre(sz), suf(sz);
		for (size_t i = 0; i < sz; i++)
		{
			size_t idx = i / sq;
			if (idx * sq == i)
			{
				blocks[idx] = data[i];
				pre[i] = data[i];
			}
			else
			{
				blocks[idx] = std::min(blocks[idx], data[i]);
				pre[i] = std::min(data[i], pre[i - 1]);
			}
		}
		for (int i = sz - 1; i >= 0; i--)
		{
			size_t idx = i / sq;
			if (i == sz - 1 || (i - idx * sq) == sq - 1)
				suf[i] = data[i];
			else
				suf[i] = std::min(data[i], suf[i + 1]);
		}
		SqrtBlocksPrefix l2 = SqrtBlocksPrefix::build(blocks, true);
		return {&data, blocks, pre, suf, sq, l2};
	}

	size_t space() const { return sizeof(*this) + ((blocks.size() + pre.size() + suf.size()) * sizeof(uint64_t)) + l2_blocks.space(); }

	uint64_t query(size_t l, size_t r) const
	{
		size_t ldx = l / sq, rdx = r / sq;
		if (ldx == rdx)
		{
			uint64_t mn = (*data)[l];
			for (size_t i = l + 1; i <= r; i++)
			{
				mn = std::min(mn, (*data)[i]);
			}
			return mn;
		}
		uint64_t mn = std::min(suf[l], pre[r]);
		if (rdx - ldx > 1)
			mn = std::min(mn, l2_blocks.query(ldx + 1, rdx - 1));
		return mn;
	}
};

struct CartesianTrees
{
	static std::string name() { return "Cartesian Tree (LCA)"; }
	static size_t max_n() { return -1; }

	const std::vector<uint64_t> *data;
	const std::vector<uint64_t> tour_vals;
	const std::vector<int> tour_depths;
	const std::vector<std::vector<int>> st;
	const std::vector<int> first_occurrence;

	static CartesianTrees build(const std::vector<uint64_t> &data)
	{
		size_t sz = data.size();

		std::vector<int> left(sz, -1), right(sz, -1);
		std::vector<int> s;
		for (int i = 0; i < sz; i++)
		{
			int last = -1;
			while (!s.empty() && data[s.back()] > data[i])
			{
				last = s.back();
				s.pop_back();
			}
			if (!s.empty())
				right[s.back()] = i;
			if (last != -1)
				left[i] = last;
			s.push_back(i);
		}

		int root = s.empty() ? -1 : 0;
		while (!s.empty())
		{
			root = s.back();
			s.pop_back();
		}

		std::vector<uint64_t> tour_vals;
		std::vector<int> tour_depths;
		std::vector<int> first_occ(sz);

		auto dfs = [&](auto self, int u, int d) -> void
		{
			if (u == -1)
				return;
			first_occ[u] = tour_vals.size();
			tour_vals.push_back(data[u]);
			tour_depths.push_back(d);

			if (left[u] != -1)
			{
				self(self, left[u], d + 1);
				tour_vals.push_back(data[u]);
				tour_depths.push_back(d);
			}
			if (right[u] != -1)
			{
				self(self, right[u], d + 1);
				tour_vals.push_back(data[u]);
				tour_depths.push_back(d);
			}
		};
		dfs(dfs, root, 0);

		int m = tour_depths.size();
		int lg = 32 - __builtin_clz((unsigned int)m);
		std::vector<std::vector<int>> st(lg, std::vector<int>(m));
		for (int i = 0; i < m; i++)
			st[0][i] = i;

		for (int j = 1; j < lg; j++)
		{
			for (int i = 0; i + (1 << j) <= m; i++)
			{
				int left = st[j - 1][i];
				int right = st[j - 1][i + (1 << (j - 1))];
				st[j][i] = (tour_depths[left] < tour_depths[right]) ? left : right;
			}
		}

		return {&data, std::move(tour_vals), std::move(tour_depths), std::move(st), std::move(first_occ)};
	}

	uint64_t query(size_t l, size_t r) const
	{
		int l_idx = first_occurrence[l];
		int r_idx = first_occurrence[r];
		if (l_idx > r_idx)
			std::swap(l_idx, r_idx);

		int len = r_idx - l_idx + 1;
		int k = 31 - __builtin_clz((unsigned int)len);

		int idx1 = st[k][l_idx];
		int idx2 = st[k][r_idx - (1 << k) + 1];
		int lca_idx = (tour_depths[idx1] < tour_depths[idx2]) ? idx1 : idx2;

		return tour_vals[lca_idx];
	}

	size_t space() const
	{
		size_t total = sizeof(*this) + (tour_vals.size() + tour_depths.size()) * sizeof(uint64_t);
		for (const auto &row : st)
			total += row.size() * sizeof(int);
		total += first_occurrence.size() * sizeof(int);
		return total;
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
		bench<SegTreePointers>(input);
		bench<PreComputeAllAnswers>(input);
		bench<SparseTable>(input);
		bench<SqrtBlocks>(input);
		bench<SqrtBlocksPrefix>(input);
		bench<MultilevelSqrtBlocks>(input);
		bench<CartesianTrees>(input);
	}

	return 0;
}
