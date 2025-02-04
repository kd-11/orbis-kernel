#pragma once

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>

namespace orbis {
inline namespace utils {
struct RcBase {
  std::atomic<unsigned> references{0};

  virtual ~RcBase() = default;

  void incRef() {
    if (references.fetch_add(1, std::memory_order::relaxed) > 512) {
      assert(!"too many references");
    }
  }

  // returns true if object was destroyed
  bool decRef() {
    if (references.fetch_sub(1, std::memory_order::relaxed) == 1) {
      delete this;
      return true;
    }

    return false;
  }
};

template <typename T>
concept WithRc = requires(T t) {
  t.incRef();
  t.decRef();
};

template <typename T> class Ref {
  T *m_ref = nullptr;

public:
  Ref() = default;

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref(OT *ref) : m_ref(ref) { ref->incRef(); }

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref(const Ref<OT> &other) : m_ref(other.get()) { m_ref->incRef(); }

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref(Ref<OT> &&other) : m_ref(other.release()) {}

  Ref(const Ref &other) : m_ref(other.get()) { m_ref->incRef(); }
  Ref(Ref &&other) : m_ref(other.release()) {}

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref &operator=(Ref<OT> &&other) {
    other.swap(*this);
    return *this;
  }

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref &operator=(OT *other) {
    *this = Ref(other);
    return *this;
  }

  template<typename OT> requires(std::is_base_of_v<T, OT>)
  Ref &operator=(const Ref<OT> &other) {
    *this = Ref(other);
    return *this;
  }

  Ref &operator=(const Ref &other) {
    *this = Ref(other);
    return *this;
  }

  Ref &operator=(Ref &&other) {
    other.swap(*this);
    return *this;
  }

  ~Ref() {
    if (m_ref != nullptr) {
      m_ref->decRef();
    }
  }

  void swap(Ref<T> &other) { std::swap(m_ref, other.m_ref); }
  T *get() const { return m_ref; }
  T *release() { return std::exchange(m_ref, nullptr); }
  T *operator->() const { return m_ref; }
  explicit operator bool() const { return m_ref != nullptr; }
  bool operator==(std::nullptr_t) const { return m_ref == nullptr; }
  bool operator!=(std::nullptr_t) const { return m_ref != nullptr; }
  auto operator<=>(const T *other) const { return m_ref <=> other; }
  auto operator<=>(const Ref &other) const = default;
};

template <WithRc T, typename... ArgsT>
  requires(std::is_constructible_v<T, ArgsT...>)
Ref<T> create(ArgsT &&...args) {
  auto result = new T(std::forward<ArgsT>(args)...);
  return Ref<T>(result);
}
} // namespace utils
} // namespace orbis
