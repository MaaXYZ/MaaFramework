#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

template <typename Handle, Handle* (*Create)(), void (*Destroy)(Handle*)>
struct HandlerHolder
{
    Handle* buffer;
    bool own;

    HandlerHolder()
        : buffer(Create())
        , own(true)
    {
    }

    HandlerHolder(Handle* handle, bool own)
        : buffer(handle)
        , own(own)
    {
    }

    HandlerHolder(const HandlerHolder& holder) = delete;

    HandlerHolder(HandlerHolder&& holder)
    {
        buffer = holder.buffer;
        own = holder.own;
        holder.own = false;
    }

    ~HandlerHolder()
    {
        if (own) {
            Destroy(buffer);
        }
    }

    HandlerHolder& operator=(const HandlerHolder& holder) = delete;

    HandlerHolder& operator=(HandlerHolder&& holder)
    {
        if (&holder == this) {
            return *this;
        }
        if (own) {
            Destroy(buffer);
        }
        buffer = holder.buffer;
        own = holder.own;
        holder.own = false;
        return *this;
    }
};

template <typename Handle>
struct HandlerReferHolder
{
    Handle* buffer;

    HandlerReferHolder(Handle* handle)
        : buffer(handle)
    {
    }

    HandlerReferHolder(const HandlerReferHolder& holder) = default;

    HandlerReferHolder& operator=(const HandlerReferHolder& holder) = default;
};

struct StringBuffer : public HandlerHolder<MaaStringBuffer, &MaaStringBufferCreate, &MaaStringBufferDestroy>
{
    using HandlerHolder::HandlerHolder;

    operator MaaStringBuffer*() const { return buffer; }

    std::string str() const { return std::string(MaaStringBufferGet(buffer), MaaStringBufferSize(buffer)); }

    void set(std::string_view data) const { MaaStringBufferSetEx(buffer, data.data(), data.size()); }
};

struct StringBufferRefer : public HandlerReferHolder<const MaaStringBuffer>
{
    using HandlerReferHolder::HandlerReferHolder;

    StringBufferRefer(const StringBuffer& buf)
        : HandlerReferHolder(buf.buffer)
    {
    }

    operator const MaaStringBuffer*() const { return buffer; }

    std::string str() const { return std::string(MaaStringBufferGet(buffer), MaaStringBufferSize(buffer)); }
};

struct ImageBuffer : public HandlerHolder<MaaImageBuffer, &MaaImageBufferCreate, &MaaImageBufferDestroy>
{
    using HandlerHolder::HandlerHolder;

    operator MaaImageBuffer*() const { return buffer; }

    maajs::ArrayBufferType data(maajs::EnvType env) const
    {
        auto len = MaaImageBufferGetEncodedSize(buffer);
        auto buf = maajs::ArrayBufferType::New(env, len);
        std::memcpy(buf.Data(), MaaImageBufferGetEncoded(buffer), len);
        return buf;
    }

    void set(maajs::ArrayBufferType data) const
    {
        MaaImageBufferSetEncoded(buffer, reinterpret_cast<uint8_t*>(data.Data()), data.ByteLength());
    }
};

struct ImageBufferRefer : public HandlerReferHolder<const MaaImageBuffer>
{
    using HandlerReferHolder::HandlerReferHolder;

    ImageBufferRefer(const ImageBuffer& buf)
        : HandlerReferHolder(buf.buffer)
    {
    }

    operator const MaaImageBuffer*() const { return buffer; }

    maajs::ArrayBufferType data(maajs::EnvType env) const
    {
        auto len = MaaImageBufferGetEncodedSize(buffer);
        auto buf = maajs::ArrayBufferType::New(env, len);
        std::memcpy(buf.Data(), MaaImageBufferGetEncoded(buffer), len);
        return buf;
    }
};

template <
    typename ListHandle,
    typename Handle,
    typename Wrapper,
    ListHandle* (*Create)(),
    void (*Destroy)(ListHandle*),
    MaaBool (*IsEmpty)(const ListHandle*),
    MaaSize (*Size)(const ListHandle*),
    const Handle* (*At)(const ListHandle*, MaaSize),
    MaaBool (*Append)(ListHandle*, const Handle*),
    MaaBool (*Remove)(ListHandle*, MaaSize),
    MaaBool (*Clear)(ListHandle*)>
struct ListBuffer
{
    ListHandle* buffer;
    bool own;

    ListBuffer()
        : buffer(Create())
        , own(true)
    {
    }

    ListBuffer(ListHandle* handle)
        : buffer(handle)
        , own(false)
    {
    }

    ~ListBuffer()
    {
        if (own) {
            Destroy(buffer);
        }
    }

    operator ListHandle*() const { return buffer; }

    size_t size() const { return Size(buffer); }

    Wrapper at(size_t i) const { return Wrapper(At(buffer, i)); }

    bool append(const Wrapper& wrapper) { return Append(buffer, wrapper); }

    bool remove(size_t i) { return Remove(buffer, i); }

    void clear() { Clear(buffer); }

    template <typename Mapper>
    auto as_vector(Mapper func) const
    {
        std::vector<decltype(func(std::declval<Wrapper>()))> result;
        auto sz = size();
        result.reserve(sz);
        for (size_t i = 0; i < sz; i++) {
            result.push_back(func(at(i)));
        }
        return result;
    }

    template <typename Vec, typename Mapper>
    void set_vector(const Vec& vec, Mapper func)
    {
        clear();
        for (const auto& val : vec) {
            append(func(val));
        }
    }
};

using StringListBuffer = ListBuffer<
    MaaStringListBuffer,
    MaaStringBuffer,
    StringBufferRefer,
    &MaaStringListBufferCreate,
    &MaaStringListBufferDestroy,
    &MaaStringListBufferIsEmpty,
    &MaaStringListBufferSize,
    &MaaStringListBufferAt,
    &MaaStringListBufferAppend,
    &MaaStringListBufferRemove,
    &MaaStringListBufferClear>;

using ImageListBuffer = ListBuffer<
    MaaImageListBuffer,
    MaaImageBuffer,
    ImageBufferRefer,
    &MaaImageListBufferCreate,
    &MaaImageListBufferDestroy,
    &MaaImageListBufferIsEmpty,
    &MaaImageListBufferSize,
    &MaaImageListBufferAt,
    &MaaImageListBufferAppend,
    &MaaImageListBufferRemove,
    &MaaImageListBufferClear>;
