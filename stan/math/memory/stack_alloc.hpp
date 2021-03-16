#ifndef STAN_MATH_MEMORY_STACK_ALLOC_HPP
#define STAN_MATH_MEMORY_STACK_ALLOC_HPP

// TODO(Bob): <cstddef> replaces this ifdef in C++11, until then this
//            is best we can do to get safe pointer casts to uints.
#include <stdint.h>
#include <cstdlib>
#include <stan/math/prim/meta.hpp>
#include <Eigen/src/Core/util/Memory.h>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace stan {
namespace math {


namespace internal {
constexpr size_t DEFAULT_INITIAL_NBYTES = 1 << 16;  // 64KB
}  // namespace internal

using byte = unsigned char;

/**
 * An instance of this class provides a memory pool through
 * which blocks of raw memory may be allocated and then collected
 * simultaneously.
 *
 * This class is useful in settings where large numbers of small
 * objects are allocated and then collected all at once.  This may
 * include objects whose destructors have no effect.
 *
 * Memory is allocated on a stack of blocks.  Each block allocated
 * is twice as large as the previous one.  The memory may be
 * recovered, with the blocks being reused, or all blocks may be
 * freed, resetting the stack of blocks to its original state.
 *
 * Alignment up to 8 byte boundaries guaranteed for the first malloc,
 * and after that it's up to the caller.  On 64-bit architectures,
 * all struct values should be padded to 8-byte boundaries if they
 * contain an 8-byte member or a virtual function.
 */
class stack_alloc {
 private:
  std::vector<byte*> blocks_;  // storage for blocks,
                               // may be bigger than cur_block_
  std::vector<size_t> sizes_;  // could store initial & shift for others
  size_t cur_block_;           // index into blocks_ for next alloc
  byte* cur_block_end_;        // ptr to cur_block_ptr_ + sizes_[cur_block_]
  byte* next_loc_;             // ptr to next available spot in cur
                               // block
  // next three for keeping track of nested allocations on top of stack:
  std::vector<size_t> nested_cur_blocks_;
  std::vector<byte*> nested_next_locs_;
  std::vector<byte*> nested_cur_block_ends_;

  /**
   * Moves us to the next block of memory, allocating that block
   * if necessary, and allocates len bytes of memory within that
   * block.
   *
   * @param len Number of bytes to allocate.
   * @return A pointer to the allocated memory.
   */
  inline byte* move_to_next_block(size_t len) noexcept {
    byte* result;
    ++cur_block_;
    // Find the next block (if any) containing at least len bytes.
    while ((cur_block_ < blocks_.size()) && (sizes_[cur_block_] < len)) {
      ++cur_block_;
    }
    // Allocate a new block if necessary.
    if (unlikely(cur_block_ >= blocks_.size())) {
      [&]() STAN_COLD_PATH {
        // New block should be max(2*size of last block, len) bytes.
        size_t newsize = sizes_.back() * 2;
        if (newsize < len) {
          newsize = len;
        }
        blocks_.push_back(reinterpret_cast<byte*>(Eigen::internal::aligned_malloc(newsize)));
        sizes_.push_back(newsize);
      }();
    }
    result = blocks_[cur_block_];
    // Get the object's state back in order.
    next_loc_ = result + len;
    cur_block_end_ = result + sizes_[cur_block_];
    return result;
  }

 public:
  /**
   * Construct a resizable stack allocator initially holding the
   * specified number of bytes.
   *
   * @param initial_nbytes Initial number of bytes for the
   * allocator.  Defaults to <code>(1 << 16) = 64KB</code> initial bytes.
   * @throws std::runtime_error if the underlying malloc is not 8-byte
   * aligned.
   */
  inline explicit stack_alloc(size_t initial_nbytes = internal::DEFAULT_INITIAL_NBYTES) noexcept
      : blocks_(1, reinterpret_cast<byte*>(Eigen::internal::aligned_malloc(initial_nbytes))),
        sizes_(1, initial_nbytes),
        cur_block_(0),
        cur_block_end_(blocks_[0] + initial_nbytes),
        next_loc_(blocks_[0]) {
  }

  /**
   * Destroy this memory allocator.
   *
   * This is implemented as a no-op as there is no destruction
   * required.
   */
  ~stack_alloc() noexcept {
    // free ALL blocks
    for (auto& block : blocks_) {
      if (block) {
        free(block);
      }
    }
  }

  /**
   * Return a newly allocated block of memory of the appropriate
   * size managed by the stack allocator.
   *
   * The allocated pointer will be 8-byte aligned.
   *
   * This function may call C++'s <code>malloc()</code> function,
   * with any exceptions percolated through this function.
   *
   * @param len Number of bytes to allocate.
   * @return A pointer to the allocated memory.
   */
  inline void* alloc(size_t len) noexcept {
    // Typically, just return and increment the next location.
    byte* result = next_loc_;
    next_loc_ += len;
    // Occasionally, we have to switch blocks.
    if (unlikely(next_loc_ >= cur_block_end_)) {
      result = move_to_next_block(len);
    }
    return reinterpret_cast<void*>(result);
  }

  /**
   * Allocate an array on the arena of the specified size to hold
   * values of the specified template parameter type.
   *
   * @tparam T type of entries in allocated array.
   * @param[in] n size of array to allocate.
   * @return new array allocated on the arena.
   */
  template <typename T>
  inline T* alloc_array(size_t n) noexcept {
    return reinterpret_cast<T*>(alloc(n * sizeof(T)));
  }

  /**
   * Increase the capacity of the allocator to a value that's greater or equal
   * to new_cap. If new_cap is greater than the size remaining in the current
   * block, a new block of uninitialized memory is created of size new_cap,
   * otherwise the method does nothing.
   * @param len Number of bytes to allocate.
   */
  inline void reserve(size_t new_cap) noexcept {
    // if current block has enough then no-op
    if (unlikely(next_loc_ + new_cap >= cur_block_end_)) {
      // move the current block up until we find a block that has enough space
      ++cur_block_;
      while ((cur_block_ < blocks_.size()) && (sizes_[cur_block_] < new_cap)) {
        ++cur_block_;
      }
      // Allocate a new block if necessary.
      if (unlikely(cur_block_ >= blocks_.size())) {
        [&]() STAN_COLD_PATH {
          // New block should be max(2*size of last block, len) bytes.
          blocks_.push_back(reinterpret_cast<byte*>(Eigen::internal::aligned_malloc(new_cap)));
          sizes_.push_back(new_cap);
        }();
      }
      // Move everything up to the new block
      next_loc_ = blocks_[cur_block_];
      cur_block_end_ = blocks_[cur_block_] + sizes_[cur_block_];
    }
  }

  template <typename T = double>
  inline void reserve_array(size_t new_cap) {
    this->reserve(new_cap * sizeof(T));
  }


  /**
   * Recover all the memory used by the stack allocator.  The stack
   * of memory blocks allocated so far will be available for further
   * allocations.  To free memory back to the system, use the
   * function free_all().
   */
  inline void recover_all() noexcept {
    cur_block_ = 0;
    next_loc_ = blocks_[0];
    cur_block_end_ = next_loc_ + sizes_[0];
  }

  /**
   * Store current positions before doing nested operation so can
   * recover back to start.
   */
  inline void start_nested() noexcept {
    nested_cur_blocks_.push_back(cur_block_);
    nested_next_locs_.push_back(next_loc_);
    nested_cur_block_ends_.push_back(cur_block_end_);
  }

  /**
   * recover memory back to the last start_nested call.
   */
  inline void recover_nested() noexcept {
    if (unlikely(nested_cur_blocks_.empty())) {
      recover_all();
    }

    cur_block_ = nested_cur_blocks_.back();
    nested_cur_blocks_.pop_back();

    next_loc_ = nested_next_locs_.back();
    nested_next_locs_.pop_back();

    cur_block_end_ = nested_cur_block_ends_.back();
    nested_cur_block_ends_.pop_back();
  }

  /**
   * Free all memory used by the stack allocator other than the
   * initial block allocation back to the system.  Note:  the
   * destructor will free all memory.
   */
  inline void free_all() noexcept {
    // frees all BUT the first (index 0) block
    for (size_t i = 1; i < blocks_.size(); ++i) {
      if (blocks_[i]) {
        free(blocks_[i]);
      }
    }
    sizes_.resize(1);
    blocks_.resize(1);
    recover_all();
  }

  /**
   * Return number of bytes allocated to this instance by the heap.
   * This is not the same as the number of bytes allocated through
   * calls to memalloc_.  The latter number is not calculatable
   * because space is wasted at the end of blocks if the next
   * alloc request doesn't fit.  (Perhaps we could trim down to
   * what is actually used?)
   *
   * @return number of bytes allocated to this instance
   */
  inline size_t bytes_allocated() const noexcept {
    size_t sum = 0;
    for (size_t i = 0; i <= cur_block_; ++i) {
      sum += sizes_[i];
    }
    return sum;
  }

  /**
   * Indicates whether the memory in the pointer
   * is in the stack.
   *
   * @param[in] ptr memory location
   * @return true if the pointer is in the stack,
   *    false otherwise.
   */
  inline bool in_stack(const void* ptr) const noexcept {
    for (size_t i = 0; i < cur_block_; ++i) {
      if (ptr >= blocks_[i] && ptr < blocks_[i] + sizes_[i]) {
        return true;
      }
    }
    if (ptr >= blocks_[cur_block_] && ptr < next_loc_) {
      return true;
    }
    return false;
  }
};

}  // namespace math
}  // namespace stan
#endif
