module;

export module maa.nodejs.utils;

import maa.core;
import napi;
import stdmock;

import maa.nodejs.wrapper;

export template <typename Result, StringHolder name>
class SimpleAsyncWork : public Napi::AsyncWorker
{
public:
    SimpleAsyncWork(
        Napi::Env env,
        std::function<Result()> execute,
        std::function<Napi::Value(Napi::Env, const Result&)> ok = [](Napi::Env env,
                                                                     const Result& res) { return JSConvert<Result>::to_value(env, res); })
        : AsyncWorker(env)
        , execute(execute)
        , ok(ok)
        , deferred(Napi::Promise::Deferred::New(env))
    {
    }

    void Execute() override { result = execute(); }

    void OnOK() override
    {
        try {
            deferred.Resolve(ok(Env(), result));
        }
        catch (const MaaNodeException& exc) {
            deferred.Reject(Napi::TypeError::New(Env(), std::format("maa.{}: {}", name.data, exc.what())).Value());
        }
    }

    void OnError(const Napi::Error& e) override { deferred.Reject(e.Value()); }

    Napi::Promise Promise() { return deferred.Promise(); }

private:
    Result result;
    std::function<Result()> execute;
    std::function<Napi::Value(Napi::Env, const Result&)> ok;
    Napi::Promise::Deferred deferred;
};

export template <typename Type>
void DeleteFinalizer([[maybe_unused]] Napi::Env env, Type data)
{
    delete data;
}

export template <typename Handle, Handle* (*Create)(), void (*Destroy)(Handle*)>
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

export template <typename Handle>
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

export struct StringBuffer : public HandlerHolder<MaaStringBuffer, &MaaStringBufferCreate, &MaaStringBufferDestroy>
{
    using HandlerHolder::HandlerHolder;

    operator MaaStringBuffer*() const { return buffer; }

    std::string str() const { return std::string(MaaStringBufferGet(buffer), MaaStringBufferSize(buffer)); }

    void set(std::string_view data) const { MaaStringBufferSetEx(buffer, data.data(), data.size()); }
};

export struct StringBufferRefer : public HandlerReferHolder<const MaaStringBuffer>
{
    using HandlerReferHolder::HandlerReferHolder;

    StringBufferRefer(const StringBuffer& buf)
        : HandlerReferHolder(buf.buffer)
    {
    }

    operator const MaaStringBuffer*() const { return buffer; }

    std::string str() const { return std::string(MaaStringBufferGet(buffer), MaaStringBufferSize(buffer)); }
};

export struct ImageBuffer : public HandlerHolder<MaaImageBuffer, &MaaImageBufferCreate, &MaaImageBufferDestroy>
{
    using HandlerHolder::HandlerHolder;

    operator MaaImageBuffer*() const { return buffer; }

    Napi::ArrayBuffer data(Napi::Env env) const
    {
        auto len = MaaImageBufferGetEncodedSize(buffer);
        auto buf = Napi::ArrayBuffer::New(env, len);
        std::memcpy(buf.Data(), MaaImageBufferGetEncoded(buffer), len);
        return buf;
    }

    void set(Napi::ArrayBuffer data) const { MaaImageBufferSetEncoded(buffer, reinterpret_cast<uint8_t*>(data.Data()), data.ByteLength()); }
};

export struct ImageBufferRefer : public HandlerReferHolder<const MaaImageBuffer>
{
    using HandlerReferHolder::HandlerReferHolder;

    ImageBufferRefer(const ImageBuffer& buf)
        : HandlerReferHolder(buf.buffer)
    {
    }

    operator const MaaImageBuffer*() const { return buffer; }

    Napi::ArrayBuffer data(Napi::Env env) const
    {
        auto len = MaaImageBufferGetEncodedSize(buffer);
        auto buf = Napi::ArrayBuffer::New(env, len);
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

export using StringListBuffer = ListBuffer<
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

export using ImageListBuffer = ListBuffer<
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
