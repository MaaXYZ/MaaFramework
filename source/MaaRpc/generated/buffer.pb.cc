// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: buffer.proto

#include "buffer.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace maarpc {
PROTOBUF_CONSTEXPR ImageInfoResponse::ImageInfoResponse(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_._has_bits_)*/{}
  , /*decltype(_impl_._cached_size_)*/{}
  , /*decltype(_impl_.size_)*/nullptr
  , /*decltype(_impl_.type_)*/0} {}
struct ImageInfoResponseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR ImageInfoResponseDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~ImageInfoResponseDefaultTypeInternal() {}
  union {
    ImageInfoResponse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ImageInfoResponseDefaultTypeInternal _ImageInfoResponse_default_instance_;
}  // namespace maarpc
static ::_pb::Metadata file_level_metadata_buffer_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_buffer_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_buffer_2eproto = nullptr;

const uint32_t TableStruct_buffer_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::maarpc::ImageInfoResponse, _impl_._has_bits_),
  PROTOBUF_FIELD_OFFSET(::maarpc::ImageInfoResponse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::maarpc::ImageInfoResponse, _impl_.type_),
  PROTOBUF_FIELD_OFFSET(::maarpc::ImageInfoResponse, _impl_.size_),
  1,
  0,
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 8, -1, sizeof(::maarpc::ImageInfoResponse)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::maarpc::_ImageInfoResponse_default_instance_._instance,
};

const char descriptor_table_protodef_buffer_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\014buffer.proto\022\006maarpc\032\013types.proto\"Y\n\021I"
  "mageInfoResponse\022\021\n\004type\030\001 \001(\005H\000\210\001\001\022\037\n\004s"
  "ize\030\002 \001(\0132\014.maarpc.SizeH\001\210\001\001B\007\n\005_typeB\007\n"
  "\005_size2\236\003\n\005Image\0226\n\006create\022\024.maarpc.Empt"
  "yRequest\032\026.maarpc.HandleResponse\0227\n\007dest"
  "roy\022\025.maarpc.HandleRequest\032\025.maarpc.Empt"
  "yResponse\0227\n\010is_empty\022\025.maarpc.HandleReq"
  "uest\032\024.maarpc.BoolResponse\0225\n\005clear\022\025.ma"
  "arpc.HandleRequest\032\025.maarpc.EmptyRespons"
  "e\0228\n\004info\022\025.maarpc.HandleRequest\032\031.maarp"
  "c.ImageInfoResponse\0228\n\007encoded\022\025.maarpc."
  "HandleRequest\032\026.maarpc.BufferResponse\022@\n"
  "\013set_encoded\022\033.maarpc.HandleBufferReques"
  "t\032\024.maarpc.BoolResponseb\006proto3"
  ;
static const ::_pbi::DescriptorTable* const descriptor_table_buffer_2eproto_deps[1] = {
  &::descriptor_table_types_2eproto,
};
static ::_pbi::once_flag descriptor_table_buffer_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_buffer_2eproto = {
    false, false, 551, descriptor_table_protodef_buffer_2eproto,
    "buffer.proto",
    &descriptor_table_buffer_2eproto_once, descriptor_table_buffer_2eproto_deps, 1, 1,
    schemas, file_default_instances, TableStruct_buffer_2eproto::offsets,
    file_level_metadata_buffer_2eproto, file_level_enum_descriptors_buffer_2eproto,
    file_level_service_descriptors_buffer_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_buffer_2eproto_getter() {
  return &descriptor_table_buffer_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_buffer_2eproto(&descriptor_table_buffer_2eproto);
namespace maarpc {

// ===================================================================

class ImageInfoResponse::_Internal {
 public:
  using HasBits = decltype(std::declval<ImageInfoResponse>()._impl_._has_bits_);
  static void set_has_type(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
  static const ::maarpc::Size& size(const ImageInfoResponse* msg);
  static void set_has_size(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
};

const ::maarpc::Size&
ImageInfoResponse::_Internal::size(const ImageInfoResponse* msg) {
  return *msg->_impl_.size_;
}
void ImageInfoResponse::clear_size() {
  if (_impl_.size_ != nullptr) _impl_.size_->Clear();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
ImageInfoResponse::ImageInfoResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:maarpc.ImageInfoResponse)
}
ImageInfoResponse::ImageInfoResponse(const ImageInfoResponse& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  ImageInfoResponse* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){from._impl_._has_bits_}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.size_){nullptr}
    , decltype(_impl_.type_){}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  if (from._internal_has_size()) {
    _this->_impl_.size_ = new ::maarpc::Size(*from._impl_.size_);
  }
  _this->_impl_.type_ = from._impl_.type_;
  // @@protoc_insertion_point(copy_constructor:maarpc.ImageInfoResponse)
}

inline void ImageInfoResponse::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.size_){nullptr}
    , decltype(_impl_.type_){0}
  };
}

ImageInfoResponse::~ImageInfoResponse() {
  // @@protoc_insertion_point(destructor:maarpc.ImageInfoResponse)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void ImageInfoResponse::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  if (this != internal_default_instance()) delete _impl_.size_;
}

void ImageInfoResponse::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void ImageInfoResponse::Clear() {
// @@protoc_insertion_point(message_clear_start:maarpc.ImageInfoResponse)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000001u) {
    GOOGLE_DCHECK(_impl_.size_ != nullptr);
    _impl_.size_->Clear();
  }
  _impl_.type_ = 0;
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ImageInfoResponse::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // optional int32 type = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          _Internal::set_has_type(&has_bits);
          _impl_.type_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // optional .maarpc.Size size = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          ptr = ctx->ParseMessage(_internal_mutable_size(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  _impl_._has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ImageInfoResponse::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:maarpc.ImageInfoResponse)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // optional int32 type = 1;
  if (_internal_has_type()) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(1, this->_internal_type(), target);
  }

  // optional .maarpc.Size size = 2;
  if (_internal_has_size()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(2, _Internal::size(this),
        _Internal::size(this).GetCachedSize(), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:maarpc.ImageInfoResponse)
  return target;
}

size_t ImageInfoResponse::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:maarpc.ImageInfoResponse)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    // optional .maarpc.Size size = 2;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
          *_impl_.size_);
    }

    // optional int32 type = 1;
    if (cached_has_bits & 0x00000002u) {
      total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_type());
    }

  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ImageInfoResponse::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    ImageInfoResponse::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ImageInfoResponse::GetClassData() const { return &_class_data_; }


void ImageInfoResponse::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<ImageInfoResponse*>(&to_msg);
  auto& from = static_cast<const ImageInfoResponse&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:maarpc.ImageInfoResponse)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_mutable_size()->::maarpc::Size::MergeFrom(
          from._internal_size());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_impl_.type_ = from._impl_.type_;
    }
    _this->_impl_._has_bits_[0] |= cached_has_bits;
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void ImageInfoResponse::CopyFrom(const ImageInfoResponse& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:maarpc.ImageInfoResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ImageInfoResponse::IsInitialized() const {
  return true;
}

void ImageInfoResponse::InternalSwap(ImageInfoResponse* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(ImageInfoResponse, _impl_.type_)
      + sizeof(ImageInfoResponse::_impl_.type_)
      - PROTOBUF_FIELD_OFFSET(ImageInfoResponse, _impl_.size_)>(
          reinterpret_cast<char*>(&_impl_.size_),
          reinterpret_cast<char*>(&other->_impl_.size_));
}

::PROTOBUF_NAMESPACE_ID::Metadata ImageInfoResponse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_buffer_2eproto_getter, &descriptor_table_buffer_2eproto_once,
      file_level_metadata_buffer_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace maarpc
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::maarpc::ImageInfoResponse*
Arena::CreateMaybeMessage< ::maarpc::ImageInfoResponse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::maarpc::ImageInfoResponse >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>