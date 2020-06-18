#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>


void prefetch(void* addr);

// --------------------
//       Math
// --------------------

// 進行度の計算や学習で用いる数学的な関数
namespace Math {
	// シグモイド関数
	//  = 1.0 / (1.0 + std::exp(-x))
	double sigmoid(double x);

	// シグモイド関数の微分
	//  = sigmoid(x) * (1.0 - sigmoid(x))
	double dsigmoid(double x);

	// vを[lo,hi]の間に収まるようにクリップする。
	// ※　Stockfishではこの関数、bitboard.hに書いてある。
	template<class T> constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
		return v < lo ? lo : v > hi ? hi : v;
	}

}

// --------------------
//       Path
// --------------------

// C#にあるPathクラス的なもの。ファイル名の操作。
// C#のメソッド名に合わせておく。
struct Path
{
	// path名とファイル名を結合して、それを返す。
	// folder名のほうは空文字列でないときに、末尾に'/'か'\\'がなければそれを付与する。
	static std::string Combine(const std::string& folder, const std::string& filename)
	{
		if (folder.length() >= 1 && *folder.rbegin() != '/' && *folder.rbegin() != '\\')
			return folder + "/" + filename;

		return folder + filename;
	}

	// full path表現から、(フォルダ名を除いた)ファイル名の部分を取得する。
	static std::string GetFileName(const std::string& path)
	{
		// "\"か"/"か、どちらを使ってあるかはわからない。
		auto path_index1 = path.find_last_of("\\") + 1;
		auto path_index2 = path.find_last_of("/") + 1;
		auto path_index = std::max(path_index1, path_index2);

		return path.substr(path_index);
	}
};

// 途中での終了処理のためのwrapper
static inline void my_exit()
{
	// sleep(3000); // エラーメッセージが出力される前に終了するのはまずいのでwaitを入れておく。
	exit(EXIT_FAILURE);
}

extern void* aligned_malloc(size_t size, size_t align);
static inline void aligned_free(void* ptr) { _mm_free(ptr); }

// alignasを指定しているのにnewのときに無視される＆STLのコンテナがメモリ確保するときに無視するので、
// そのために用いるカスタムアロケーター。
template <typename T>
class AlignedAllocator {
public:
  using value_type = T;

  AlignedAllocator() {}
  AlignedAllocator(const AlignedAllocator&) {}
  AlignedAllocator(AlignedAllocator&&) {}

  template <typename U> AlignedAllocator(const AlignedAllocator<U>&) {}

  T* allocate(std::size_t n) { return (T*)aligned_malloc(n * sizeof(T), alignof(T)); }
  void deallocate(T* p, std::size_t n) { aligned_free(p); }
};
