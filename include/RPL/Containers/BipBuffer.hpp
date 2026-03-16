/**
 * @file BipBuffer.hpp
 * @brief RPL Bipartite Buffer Implementation
 *
 * Implements a Bipartite Circular Buffer (BipBuffer) that guarantees contiguous
 * memory access. It maintains two regions (A and B) to handle the wrap-around
 * case by starting a new region at the beginning of the buffer instead of
 * splitting data.
 */

#ifndef RPL_BIPBUFFER_HPP
#define RPL_BIPBUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace RPL::Containers {

/**
 * @brief Bipartite Buffer class
 *
 * Guarantees that any readable data chunk is physically contiguous in memory.
 * Eliminates the need for wrap-around checks during parsing.
 *
 * @tparam SIZE Buffer size, must be power of 2
 */
template <size_t SIZE> class BipBuffer {
  static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");

  alignas(64) uint8_t buffer[SIZE]{};

  // Region A: The head of the FIFO (data to be read first)
  size_t region_a_start{0};
  size_t region_a_size{0};

  // Region B: The tail of the FIFO (data to be read after A)
  // region_b_start is always 0 in this implementation when it exists
  size_t region_b_size{0};

public:
  /**
   * @brief Get contiguous buffer for writing
   *
   * Returns the largest contiguous block available for writing.
   * This may switch to the beginning of the buffer if space at the end is low.
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    // Case 1: We have data in B. We can only append to B.
    // B always starts at 0 and grows. Space is limited by A's start.
    if (region_b_size > 0) {
      size_t free_space = region_a_start - region_b_size;
      return {buffer + region_b_size, free_space};
    }

    // Case 2: Only Region A exists (or empty).
    // We can write after A up to end of buffer.
    // OR if A exists, we can start B at 0 if A is not at 0.

    // 2a: Try appending to A
    size_t a_end = region_a_start + region_a_size;
    size_t space_at_end = SIZE - a_end;

    // If we have enough space at end, or if we can't switch to B yet (A starts
    // at 0), we return the end space. 

    if (space_at_end > 0) {
      return {buffer + a_end, space_at_end};
    }

    // 2b: No space at end. Try starting Region B at 0.
    // Space available is up to region_a_start.
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }

    // Buffer full
    return {};
  }

  /**
   * @brief Reserve space/Commit write
   *
   * @param length Bytes written
   * @return true if successful
   */
  bool advance_write_index(size_t length) {
    if (length == 0)
      return true;

    // Determine where we wrote based on state
    if (region_b_size > 0) {
      // Must have written to B
      if (region_b_size + length > region_a_start)
        return false; // Overflow
      region_b_size += length;
      return true;
    }

    size_t a_end = region_a_start + region_a_size;

    if (length <= SIZE - a_end) {
      region_a_size += length;
      return true;
    }

    if (region_a_size == 0) {
      // Empty buffer logic
      region_a_size += length;
      return true;
    }

    // If we have space at end, get_write_buffer returns end.
    if (SIZE - a_end > 0) {
      if (length > SIZE - a_end)
        return false;
      region_a_size += length;
      return true;
    }

    // If no space at end, get_write_buffer returned start.
    // We create B.
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief Manual switch to Region B (Write from head)
   *
   * Force the buffer to skip the remaining space at end and start writing at 0.
   * Useful when the remaining space at end is too small for the incoming
   * packet.
   * @return Span at start of buffer (Region B)
   */
  std::span<uint8_t> get_write_buffer_force_wrap() {
    if (region_b_size > 0) {
      // Already in B mode, just return B write space
      return get_write_buffer();
    }
    // Force creation of B if possible
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }
    return {};
  }

  /**
   * @brief Commit write to forced wrap region
   */
  bool advance_write_index_wrapped(size_t length) {
    if (region_b_size > 0) {
      return advance_write_index(length);
    }
    // Create B
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief Copy data into buffer
   */
  bool write(const uint8_t *data, size_t length) {
    if (length == 0)
      return true;

    auto span = get_write_buffer();

    // Logic: Try to fit in current write region (A or B)
    if (span.size() >= length) {
      std::memcpy(span.data(), data, length);
      return advance_write_index(length);
    }

    // If not, and we are currently writing to A (B is empty),
    // Check if we can switch to B.
    if (region_b_size == 0) {
      // Check space at start
      if (region_a_start >= length) {
        // Switch to B!
        std::memcpy(buffer, data, length);
        region_b_size = length;
        return true;
      }
    }

    return false; // No space contiguous enough
  }

  /**
   * @brief Get contiguous data for reading
   */
  [[nodiscard]] std::span<const uint8_t>
  get_contiguous_read_buffer() const noexcept {
    if (region_a_size > 0) {
      return {buffer + region_a_start, region_a_size};
    }
    return {};
  }

  /**
   * @brief Discard data (advance read index)
   */
  bool discard(size_t length) {
    if (length == 0)
      return true;
    if (length > region_a_size)
      return false;

    region_a_start += length;
    region_a_size -= length;

    // If A is empty, promote B to A
    if (region_a_size == 0) {
      if (region_b_size > 0) {
        region_a_start = 0; // B always starts at 0
        region_a_size = region_b_size;
        region_b_size = 0;
      } else {
        // Reset to 0 for cleanliness when empty
        region_a_start = 0;
      }
    }
    return true;
  }

  // Convenience methods
  size_t available() const { return region_a_size + region_b_size; }

  // Space calculation is tricky:
  // It's max(space_at_end_of_A, space_at_start_before_A) - but logic depends on
  // state. Roughly:
  size_t space() const {
    if (region_b_size > 0)
      return region_a_start - region_b_size;
    // Max contiguous write? Or total free bytes?
    // Total free bytes:
    return (SIZE - (region_a_start + region_a_size)) + region_a_start;
  }

  bool full() const { return space() == 0; }
  bool empty() const { return available() == 0; }
  void clear() {
    region_a_start = region_a_size = 0;
    region_b_size = 0;
  }

  static constexpr size_t size() { return SIZE; }

  /**
   * @brief Peek (copying if necessary, but BipBuffer logic tries to avoid it)
   * Note: BipBuffer read structure makes 'peek' across boundary tricky without
   * copy.
   */
  bool peek(uint8_t *data, size_t offset, size_t length) const {
    if (offset + length > available())
      return false;

    // Reading from A
    if (offset < region_a_size) {
      size_t read_from_a = std::min(length, region_a_size - offset);
      std::memcpy(data, buffer + region_a_start + offset, read_from_a);

      if (read_from_a < length) {
        // Remainder from B
        std::memcpy(data + read_from_a, buffer, length - read_from_a);
      }
    } else {
      // Reading purely from B
      size_t offset_in_b = offset - region_a_size;
      std::memcpy(data, buffer + offset_in_b, length);
    }
    return true;
  }

  bool read(uint8_t *data, size_t length) {
    if (!peek(data, 0, length))
      return false;
    discard(length);
    return true;
  }
};

} // namespace RPL::Containers

#endif // RPL_BIPBUFFER_HPP
