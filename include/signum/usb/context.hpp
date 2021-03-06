/*
 * Copyright 2016 C. Brett Witherspoon
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

#ifndef SIGNUM_USB_CONTEXT_HPP_
#define SIGNUM_USB_CONTEXT_HPP_

#include <atomic>
#include <stdexcept>
#include <thread>

#include <libusb.h>

namespace signum {
namespace usb {

class context final
{
public:
    context()
    {
        int error = libusb_init(&context_);
        if (error != LIBUSB_SUCCESS)
            throw std::runtime_error(libusb_strerror(static_cast<libusb_error>(error)));
    }

    context(const context&) = delete;

    context(context &&other) : context_(nullptr)
    {
        operator=(std::move(other));
    }

    ~context()
    {
        if (context_ != nullptr)
            libusb_exit(context_);
    }

    context & operator=(const context &) = delete;

    context & operator=(context &&other)
    {
        if (context_ != other.context_)
        {
            context_ = other.context_;
            other.context_= nullptr;
        }
        return *this;
    }

    bool operator==(const context &rhs) const
    {
        return context_ == rhs.context_;
    }

    bool operator!=(const context &rhs) const
    {
        return !operator==(rhs);
    }

    operator libusb_context*() const { return context_; }

    void set_debug(int level)
    {
        libusb_set_debug(context_, level);
    }

private:
    libusb_context *context_;
};

} // end namespace usb
} // end namespace usb

#endif /* SIGNUM_USB_CONTEXT_HPP_ */
