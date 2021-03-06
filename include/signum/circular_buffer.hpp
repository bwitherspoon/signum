/*
 * Copyright 2015, 2016 C. Brett Witherspoon
 *
 * This file is part of the signum library
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SIGNUM_CIRCULAR_BUFFER_HPP_
#define SIGNUM_CIRCULAR_BUFFER_HPP_

#include <atomic>  // for std::atomic
#include <cstddef> // for std::size_t
#include <limits>  // for std::numeric_limits
#include <memory>  // for std::shared_ptr
#include <mutex>
#include <condition_variable>

namespace signum
{
namespace circular_buffer
{

template<typename> class writer;
template<typename> class reader;

namespace detail
{
//! A class for shared data
class impl
{
public:
    explicit impl(size_t num_items, size_t item_size);
    ~impl();
    impl(const impl &) = delete;
    impl(impl &&) = delete;
    impl & operator=(const impl &) = delete;
    impl & operator=(impl &&) = delete;
private:
    template<typename> friend class ::signum::circular_buffer::writer;
    template<typename> friend class ::signum::circular_buffer::reader;
    template<template<typename> class, typename> friend class base;
    void * d_base;
    size_t d_size;
    std::atomic<size_t> d_read;
    std::mutex d_read_mutex;
    std::condition_variable d_read_cond;
    std::atomic<size_t> d_write;
    std::mutex d_write_mutex;
    std::condition_variable d_write_cond;
};

//! A base class for a buffer
template<template<typename> class T, typename U>
class base
{
public:
    using value_type      = U;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = U &;
    using const_reference = const U &;
    using pointer         = U *;
    using const_pointer   = const U *;
    using iterator        = U *;
    using const_iterator  = const U *;

    static_assert(std::numeric_limits<size_type>::is_modulo,
                  "circular_buffer::base: modulo arithmetic not supported");

    //! Returns an iterator to the beginning of the buffer
    iterator begin();

    //! Returns a constant iterator to the beginning of the buffer
    const_iterator begin() const;

    //! Returns a constant iterator to the beginning of the buffer
    const_iterator cbegin() const { return begin(); }

    //! Returns an iterator to the end of the buffer
    iterator end();

    //! Returns a constant iterator to the end of the buffer
    const_iterator end() const;

    //! Returns a constant iterator to the end of the buffer
    const_iterator cend() const { return end(); }

    //! Checks whether the buffer is empty
    bool empty() const { return d_impl->d_write == d_impl->d_read; }

    //! Returns the number of items in the buffer
    size_type size() const;

    //! Returns the maximum size of the buffer
    size_type max_size() const;

    //! Consume items from the buffer
    void consume(size_type n);

protected:
    explicit base(size_type n);

    explicit base(std::shared_ptr<impl> ptr);

    base(const base &) = delete;

    base(base &&) = default;

    base & operator=(const base &) = delete;

    base & operator=(base &&) = default;

    std::shared_ptr<impl> d_impl;

private:
    template<template<typename> class V, typename X,
             template<typename> class Y, typename Z>
    friend bool operator==(const base<V,X> & lhs, const base<Y,Z> & rhs);
};

template<template<typename> class T, typename U>
base<T,U>::base(size_type n)
    : d_impl{std::make_shared<impl>(n, sizeof(U))}
{ }

template<template<typename> class T, typename U>
base<T,U>::base(std::shared_ptr<impl> ptr)
    : d_impl{ptr}
{ }

template<template<typename> class T, typename U>
typename base<T,U>::iterator base<T,U>::begin()
{
    const auto base = static_cast<pointer>(d_impl->d_base);
    return base + static_cast<const T<U> *>(this)->position();
}

template<template<typename> class T, typename U>
typename base<T,U>::const_iterator base<T,U>::begin() const
{
    const auto base = static_cast<pointer>(d_impl->d_base);
    return base + static_cast<const T<U> *>(this)->position();
}

template<template<typename> class T, typename U>
typename base<T,U>::iterator base<T,U>::end()
{
    return begin() + size();
}

template<template<typename> class T, typename U>
typename base<T,U>::const_iterator base<T,U>::end() const
{
    return begin() + size();
}

template<template<typename> class T, typename U>
typename base<T,U>::size_type base<T,U>::size() const
{
    const auto max = std::numeric_limits<size_type>::max();
    const auto underflow = max - d_impl->d_size / sizeof(U) + 1;

    auto sz = static_cast<const T<U> *>(this)->offset();

    if (sz >= underflow)
        sz -= underflow;

    return sz;
}

template<template<typename> class T, typename U>
typename base<T,U>::size_type base<T,U>::max_size() const
{
    return d_impl->d_size / sizeof(U) - 1;
}

template<template<typename> class T, typename U>
void base<T,U>::consume(size_type n)
{
    const auto overflow = d_impl->d_size / sizeof(U);

    auto pos = static_cast<const T<U> *>(this)->position() + n;

    if (pos >= overflow)
        pos -= overflow;

    static_cast<T<U>*>(this)->update(pos);
}

//! Returns true of two buffers share the same data
template<template<typename> class T, typename U,
         template<typename> class V, typename X>
bool operator==(const base<T,U> & lhs, const base<V,X> & rhs)
{
  return lhs.d_impl == rhs.d_impl;
}

//! Returns false of two buffers share the same data
template<template<typename> class T, typename U,
         template<typename> class V, typename X>
bool operator!=(const base<T,U> & lhs, const base<V,X> & rhs)
{
  return !(lhs == rhs);
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

//! A class to read from a buffer
template<typename T>
class reader : public detail::base<reader,T>
{
public:
    using value_type      = T;
    using size_type       = typename detail::base<reader,T>::size_type;
    using difference_type = typename detail::base<reader,T>::difference_type;
    using reference       = T &;
    using const_reference = const T &;
    using pointer         = T *;
    using const_pointer   = const T *;
    using iterator        = T *;
    using const_iterator  = const T *;

    reader(const reader &) = delete;

    reader(reader &&) = default;

    reader & operator=(const reader &) = delete;

    reader & operator=(reader &&) = default;

    //! Wait for a number of items to be in the buffer
    void wait(size_type n);

    //! Consume all items in the buffer
    void clear() { this->consume(this->size()); }

private:
    using base_type = detail::base<reader,value_type>;

    template<typename> friend class writer;

    template<template<typename> class, typename> friend class detail::base;

    explicit reader(std::shared_ptr<detail::impl> ptr);

    size_type offset() const
    {
        return base_type::d_impl->d_write - base_type::d_impl->d_read;
    }

    size_type position() const
    {
        return base_type::d_impl->d_read;
    }

    void update(size_type pos)
    {
        {
            std::lock_guard<std::mutex> lock(base_type::d_impl->d_read_mutex);
            base_type::d_impl->d_read = pos;
        }
        base_type::d_impl->d_read_cond.notify_all();
    }
};

template<typename T>
reader<T>::reader(std::shared_ptr<detail::impl> ptr)
    : detail::base<reader,T>(ptr)
{ }

template<typename T>
void reader<T>::wait(size_type n)
{
    std::unique_lock<std::mutex> lock(base_type::d_impl->d_write_mutex);
    base_type::d_impl->d_write_cond.wait(lock, [&]{ return this->size() >= n; });
}

////////////////////////////////////////////////////////////////////////////////

//! A class to write to a buffer
template<typename T>
class writer : public detail::base<writer,T>
{
public:
    using value_type      = T;
    using size_type       = typename detail::base<reader,T>::size_type;
    using difference_type = typename detail::base<reader,T>::difference_type;
    using reference       = T &;
    using const_reference = const T &;
    using pointer         = T *;
    using const_pointer   = const T *;
    using iterator        = T *;
    using const_iterator  = const T *;

    writer();

    explicit writer(size_type n);

    writer(const writer &) = delete;

    writer(writer &&) = default;

    writer & operator=(const writer &) = delete;

    writer & operator=(writer &&) = default;

    //! Wait for a number of items to be in the buffer
    void wait(size_type n);

    template<typename U = T>
    reader<value_type> make_reader();

private:
    using base_type = detail::base<writer,value_type>;

    template<template<typename> class, typename> friend class detail::base;

    size_type offset() const
    {
        return base_type::d_impl->d_read - base_type::d_impl->d_write - 1;
    }

    size_type position() const
    {
        return base_type::d_impl->d_write;
    }

    void update(size_type pos) const
    {
        {
            std::lock_guard<std::mutex> lock(base_type::d_impl->d_write_mutex);
            base_type::d_impl->d_write = pos;;
        }
        base_type::d_impl->d_write_cond.notify_all();
    }
};

template<typename T>
writer<T>::writer()
    : detail::base<writer,T>{std::make_shared<detail::impl>(0, sizeof(T))}
{ }

template<typename T>
writer<T>::writer(size_type n)
    : detail::base<writer,T>{std::make_shared<detail::impl>(n, sizeof(T))}
{ }

template<typename T>
void writer<T>::wait(size_type n)
{
    std::unique_lock<std::mutex> lock(base_type::d_impl->d_read_mutex);
    base_type::d_impl->d_read_cond.wait(lock, [&]{ return this->size() >= n; });
}

template<typename T>
template<typename U>
reader<T> writer<T>::make_reader()
{
    return std::move(reader<U>{detail::base<writer,T>::d_impl});
}

} // namespace circular_buffer
} // namespace signum

#endif /* SIGNUM_CIRCULAR_BUFFER_HPP_ */
