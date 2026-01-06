#pragma once
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>

// --- Forward decls for your wrappers so the concepts can see them ---
template <class T> class PySeq;
template <class K, class V> class PyDictView;

template <class R>
using range_value_t_no_cvref = std::remove_cvref_t<std::ranges::range_value_t<R>>;

template <class T, class U>
concept same_nocvref =
  std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <class P, class K, class V>
concept pair_like_KV =
  requires {
    typename std::remove_cvref_t<P>::first_type;
    typename std::remove_cvref_t<P>::second_type;
  };

template<class T>
concept StringViewLike = 
  std::same_as<std::remove_cvref_t<T>, std::string> ||
  std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <class T>
concept Numeric =
  std::is_arithmetic_v<std::remove_cvref_t<T>> &&
 !std::same_as<std::remove_cvref_t<T>, bool>;

// ============================================================================
// Sequence-like: std::vector<T>, std::span<T>, PySeq<T>, ...
// ============================================================================
template <class S, class Elem>
concept SequenceLike =
  std::ranges::forward_range<S> &&
  requires (const S& s) {
    { s.size() }  -> std::convertible_to<std::size_t>;
    { s.empty() } -> std::same_as<bool>;
  } &&
  requires (const S& s, std::size_t i) {
    { s[i] } -> std::convertible_to<Elem>;
  } &&
  std::convertible_to<std::ranges::range_value_t<S>, Elem>;

// ============================================================================
// Map-like core (STL maps, unordered maps, PyDictView)
// ============================================================================
template <class M, class K, class V>
concept AssocMapCore =
  std::ranges::input_range<M> &&
  requires (const M& m, const K& k) {
    { m.size() }     -> std::convertible_to<std::size_t>;
    { m.empty() }    -> std::same_as<bool>;
    { m.contains(k) }-> std::same_as<bool>;
    { m.at(k) }      -> std::convertible_to<V>;
  } &&
  // range_value_t must be pair-like <K,Vish>
  pair_like_KV<range_value_t_no_cvref<M>, K, V>;

// --- STL-style find: iterator comparable to end(), deref pair-like<K,V> ---
template <class M, class K, class V>
concept AssocMapFindSTL =
  requires (const M& m, const K& k) {
    { m.find(k) } -> std::input_or_output_iterator;
    { m.end() }   -> same_nocvref<decltype(m.find(k))>;
    // equality with end()
    { m.find(k) == m.end() } -> std::same_as<bool>;
    { m.end() == m.find(k) } -> std::same_as<bool>;
  } &&
  // Deref yields pair-like<K,V> (by ref or value)
  requires (const M& m, const K& k) {
    pair_like_KV<decltype(*m.find(k)), K, V>;
  };

// --- PyDictView-style find: custom handle + sentinel, deref pair-like<K,V> ---
template <class M, class K, class V>
concept AssocMapFindPy =
  requires (const M& m, const K& k) {
    typename M::find_result;
    typename M::find_end_t;
    { m.find(k) } -> same_nocvref<typename M::find_result>;
    { m.find(k) == typename M::find_end_t{} } -> std::same_as<bool>;
    { typename M::find_end_t{} == m.find(k) } -> std::same_as<bool>;
  } &&
  requires (const M& m, const K& k) {
    // *find_result convertible to pair<const K,V> (value or ref is fine)
    { *m.find(k) } -> std::convertible_to<std::pair<K, V>>;
  };

// ============================================================================
// Unified Map concept: requires core + (STL-find OR Py-find)
// ============================================================================
template <class M, class K, class V>
concept AssocMapLike =
  AssocMapCore<M,K,V> &&
  ( AssocMapFindSTL<M,K,V> || AssocMapFindPy<M,K,V> );

template<typename T, typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<std::pair<std::string, T>>>
using unordered_str_map = std::unordered_map<std::string, T, std::hash<std::string_view>, KeyEqual, Allocator>;

template<typename T, typename Compare = std::less<>, typename Allocator = std::allocator<std::pair<std::string, T>>>
using str_map = std::map<std::string, T, Compare, Allocator>;

template<typename Compare = std::less<>, typename Allocator = std::allocator<std::string>>
using str_set = std::set<std::string, Compare, Allocator>;

template<typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<std::string>>
using unordered_str_set = std::unordered_set<std::string, std::hash<std::string_view>, KeyEqual, Allocator>;

template <class T, class... Ts>
  requires (std::equality_comparable_with<T, Ts> && ...)
constexpr bool IN(const T& value, const Ts&... values) {
    return ((value == values) || ...);
}

template<typename map_t, typename map_k_t, typename map_v_t, typename key_t>
  requires StringViewLike<map_k_t>
           && StringViewLike<key_t>
           && AssocMapLike<map_t, map_k_t, map_v_t>
inline map_v_t& str_map_at(map_t map, const key_t& key) {
  auto find_node = map.find(key);
  if (find_node == map.end()) {
    throw std::out_of_range(key);
  } else {
    return find_node->second;
  }
}

struct never_t {};