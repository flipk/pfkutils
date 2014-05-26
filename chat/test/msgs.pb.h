// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: msgs.proto

#ifndef PROTOBUF_msgs_2eproto__INCLUDED
#define PROTOBUF_msgs_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace PFK {
namespace TestMsgs {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_msgs_2eproto();
void protobuf_AssignDesc_msgs_2eproto();
void protobuf_ShutdownFile_msgs_2eproto();

class CommandAdd_m;
class Command_m;
class ResponseAdd_m;
class Response_m;

enum CommandType {
  COMMAND_ADD = 1
};
bool CommandType_IsValid(int value);
const CommandType CommandType_MIN = COMMAND_ADD;
const CommandType CommandType_MAX = COMMAND_ADD;
const int CommandType_ARRAYSIZE = CommandType_MAX + 1;

const ::google::protobuf::EnumDescriptor* CommandType_descriptor();
inline const ::std::string& CommandType_Name(CommandType value) {
  return ::google::protobuf::internal::NameOfEnum(
    CommandType_descriptor(), value);
}
inline bool CommandType_Parse(
    const ::std::string& name, CommandType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<CommandType>(
    CommandType_descriptor(), name, value);
}
enum ResponseType {
  RESPONSE_ADD = 1
};
bool ResponseType_IsValid(int value);
const ResponseType ResponseType_MIN = RESPONSE_ADD;
const ResponseType ResponseType_MAX = RESPONSE_ADD;
const int ResponseType_ARRAYSIZE = ResponseType_MAX + 1;

const ::google::protobuf::EnumDescriptor* ResponseType_descriptor();
inline const ::std::string& ResponseType_Name(ResponseType value) {
  return ::google::protobuf::internal::NameOfEnum(
    ResponseType_descriptor(), value);
}
inline bool ResponseType_Parse(
    const ::std::string& name, ResponseType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ResponseType>(
    ResponseType_descriptor(), name, value);
}
// ===================================================================

class CommandAdd_m : public ::google::protobuf::Message {
 public:
  CommandAdd_m();
  virtual ~CommandAdd_m();

  CommandAdd_m(const CommandAdd_m& from);

  inline CommandAdd_m& operator=(const CommandAdd_m& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CommandAdd_m& default_instance();

  void Swap(CommandAdd_m* other);

  // implements Message ----------------------------------------------

  CommandAdd_m* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CommandAdd_m& from);
  void MergeFrom(const CommandAdd_m& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required int32 a = 1;
  inline bool has_a() const;
  inline void clear_a();
  static const int kAFieldNumber = 1;
  inline ::google::protobuf::int32 a() const;
  inline void set_a(::google::protobuf::int32 value);

  // required int32 b = 2;
  inline bool has_b() const;
  inline void clear_b();
  static const int kBFieldNumber = 2;
  inline ::google::protobuf::int32 b() const;
  inline void set_b(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:PFK.TestMsgs.CommandAdd_m)
 private:
  inline void set_has_a();
  inline void clear_has_a();
  inline void set_has_b();
  inline void clear_has_b();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::int32 a_;
  ::google::protobuf::int32 b_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(2 + 31) / 32];

  friend void  protobuf_AddDesc_msgs_2eproto();
  friend void protobuf_AssignDesc_msgs_2eproto();
  friend void protobuf_ShutdownFile_msgs_2eproto();

  void InitAsDefaultInstance();
  static CommandAdd_m* default_instance_;
};
// -------------------------------------------------------------------

class Command_m : public ::google::protobuf::Message {
 public:
  Command_m();
  virtual ~Command_m();

  Command_m(const Command_m& from);

  inline Command_m& operator=(const Command_m& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const Command_m& default_instance();

  void Swap(Command_m* other);

  // implements Message ----------------------------------------------

  Command_m* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Command_m& from);
  void MergeFrom(const Command_m& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required .PFK.TestMsgs.CommandType type = 1;
  inline bool has_type() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 1;
  inline ::PFK::TestMsgs::CommandType type() const;
  inline void set_type(::PFK::TestMsgs::CommandType value);

  // optional .PFK.TestMsgs.CommandAdd_m add = 2;
  inline bool has_add() const;
  inline void clear_add();
  static const int kAddFieldNumber = 2;
  inline const ::PFK::TestMsgs::CommandAdd_m& add() const;
  inline ::PFK::TestMsgs::CommandAdd_m* mutable_add();
  inline ::PFK::TestMsgs::CommandAdd_m* release_add();
  inline void set_allocated_add(::PFK::TestMsgs::CommandAdd_m* add);

  // @@protoc_insertion_point(class_scope:PFK.TestMsgs.Command_m)
 private:
  inline void set_has_type();
  inline void clear_has_type();
  inline void set_has_add();
  inline void clear_has_add();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::PFK::TestMsgs::CommandAdd_m* add_;
  int type_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(2 + 31) / 32];

  friend void  protobuf_AddDesc_msgs_2eproto();
  friend void protobuf_AssignDesc_msgs_2eproto();
  friend void protobuf_ShutdownFile_msgs_2eproto();

  void InitAsDefaultInstance();
  static Command_m* default_instance_;
};
// -------------------------------------------------------------------

class ResponseAdd_m : public ::google::protobuf::Message {
 public:
  ResponseAdd_m();
  virtual ~ResponseAdd_m();

  ResponseAdd_m(const ResponseAdd_m& from);

  inline ResponseAdd_m& operator=(const ResponseAdd_m& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ResponseAdd_m& default_instance();

  void Swap(ResponseAdd_m* other);

  // implements Message ----------------------------------------------

  ResponseAdd_m* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ResponseAdd_m& from);
  void MergeFrom(const ResponseAdd_m& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required int32 sum = 1;
  inline bool has_sum() const;
  inline void clear_sum();
  static const int kSumFieldNumber = 1;
  inline ::google::protobuf::int32 sum() const;
  inline void set_sum(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:PFK.TestMsgs.ResponseAdd_m)
 private:
  inline void set_has_sum();
  inline void clear_has_sum();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::int32 sum_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];

  friend void  protobuf_AddDesc_msgs_2eproto();
  friend void protobuf_AssignDesc_msgs_2eproto();
  friend void protobuf_ShutdownFile_msgs_2eproto();

  void InitAsDefaultInstance();
  static ResponseAdd_m* default_instance_;
};
// -------------------------------------------------------------------

class Response_m : public ::google::protobuf::Message {
 public:
  Response_m();
  virtual ~Response_m();

  Response_m(const Response_m& from);

  inline Response_m& operator=(const Response_m& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const Response_m& default_instance();

  void Swap(Response_m* other);

  // implements Message ----------------------------------------------

  Response_m* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Response_m& from);
  void MergeFrom(const Response_m& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required .PFK.TestMsgs.ResponseType type = 1;
  inline bool has_type() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 1;
  inline ::PFK::TestMsgs::ResponseType type() const;
  inline void set_type(::PFK::TestMsgs::ResponseType value);

  // optional .PFK.TestMsgs.ResponseAdd_m add = 2;
  inline bool has_add() const;
  inline void clear_add();
  static const int kAddFieldNumber = 2;
  inline const ::PFK::TestMsgs::ResponseAdd_m& add() const;
  inline ::PFK::TestMsgs::ResponseAdd_m* mutable_add();
  inline ::PFK::TestMsgs::ResponseAdd_m* release_add();
  inline void set_allocated_add(::PFK::TestMsgs::ResponseAdd_m* add);

  // @@protoc_insertion_point(class_scope:PFK.TestMsgs.Response_m)
 private:
  inline void set_has_type();
  inline void clear_has_type();
  inline void set_has_add();
  inline void clear_has_add();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::PFK::TestMsgs::ResponseAdd_m* add_;
  int type_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(2 + 31) / 32];

  friend void  protobuf_AddDesc_msgs_2eproto();
  friend void protobuf_AssignDesc_msgs_2eproto();
  friend void protobuf_ShutdownFile_msgs_2eproto();

  void InitAsDefaultInstance();
  static Response_m* default_instance_;
};
// ===================================================================


// ===================================================================

// CommandAdd_m

// required int32 a = 1;
inline bool CommandAdd_m::has_a() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void CommandAdd_m::set_has_a() {
  _has_bits_[0] |= 0x00000001u;
}
inline void CommandAdd_m::clear_has_a() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void CommandAdd_m::clear_a() {
  a_ = 0;
  clear_has_a();
}
inline ::google::protobuf::int32 CommandAdd_m::a() const {
  return a_;
}
inline void CommandAdd_m::set_a(::google::protobuf::int32 value) {
  set_has_a();
  a_ = value;
}

// required int32 b = 2;
inline bool CommandAdd_m::has_b() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void CommandAdd_m::set_has_b() {
  _has_bits_[0] |= 0x00000002u;
}
inline void CommandAdd_m::clear_has_b() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void CommandAdd_m::clear_b() {
  b_ = 0;
  clear_has_b();
}
inline ::google::protobuf::int32 CommandAdd_m::b() const {
  return b_;
}
inline void CommandAdd_m::set_b(::google::protobuf::int32 value) {
  set_has_b();
  b_ = value;
}

// -------------------------------------------------------------------

// Command_m

// required .PFK.TestMsgs.CommandType type = 1;
inline bool Command_m::has_type() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Command_m::set_has_type() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Command_m::clear_has_type() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Command_m::clear_type() {
  type_ = 1;
  clear_has_type();
}
inline ::PFK::TestMsgs::CommandType Command_m::type() const {
  return static_cast< ::PFK::TestMsgs::CommandType >(type_);
}
inline void Command_m::set_type(::PFK::TestMsgs::CommandType value) {
  assert(::PFK::TestMsgs::CommandType_IsValid(value));
  set_has_type();
  type_ = value;
}

// optional .PFK.TestMsgs.CommandAdd_m add = 2;
inline bool Command_m::has_add() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Command_m::set_has_add() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Command_m::clear_has_add() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Command_m::clear_add() {
  if (add_ != NULL) add_->::PFK::TestMsgs::CommandAdd_m::Clear();
  clear_has_add();
}
inline const ::PFK::TestMsgs::CommandAdd_m& Command_m::add() const {
  return add_ != NULL ? *add_ : *default_instance_->add_;
}
inline ::PFK::TestMsgs::CommandAdd_m* Command_m::mutable_add() {
  set_has_add();
  if (add_ == NULL) add_ = new ::PFK::TestMsgs::CommandAdd_m;
  return add_;
}
inline ::PFK::TestMsgs::CommandAdd_m* Command_m::release_add() {
  clear_has_add();
  ::PFK::TestMsgs::CommandAdd_m* temp = add_;
  add_ = NULL;
  return temp;
}
inline void Command_m::set_allocated_add(::PFK::TestMsgs::CommandAdd_m* add) {
  delete add_;
  add_ = add;
  if (add) {
    set_has_add();
  } else {
    clear_has_add();
  }
}

// -------------------------------------------------------------------

// ResponseAdd_m

// required int32 sum = 1;
inline bool ResponseAdd_m::has_sum() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ResponseAdd_m::set_has_sum() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ResponseAdd_m::clear_has_sum() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ResponseAdd_m::clear_sum() {
  sum_ = 0;
  clear_has_sum();
}
inline ::google::protobuf::int32 ResponseAdd_m::sum() const {
  return sum_;
}
inline void ResponseAdd_m::set_sum(::google::protobuf::int32 value) {
  set_has_sum();
  sum_ = value;
}

// -------------------------------------------------------------------

// Response_m

// required .PFK.TestMsgs.ResponseType type = 1;
inline bool Response_m::has_type() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Response_m::set_has_type() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Response_m::clear_has_type() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Response_m::clear_type() {
  type_ = 1;
  clear_has_type();
}
inline ::PFK::TestMsgs::ResponseType Response_m::type() const {
  return static_cast< ::PFK::TestMsgs::ResponseType >(type_);
}
inline void Response_m::set_type(::PFK::TestMsgs::ResponseType value) {
  assert(::PFK::TestMsgs::ResponseType_IsValid(value));
  set_has_type();
  type_ = value;
}

// optional .PFK.TestMsgs.ResponseAdd_m add = 2;
inline bool Response_m::has_add() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Response_m::set_has_add() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Response_m::clear_has_add() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Response_m::clear_add() {
  if (add_ != NULL) add_->::PFK::TestMsgs::ResponseAdd_m::Clear();
  clear_has_add();
}
inline const ::PFK::TestMsgs::ResponseAdd_m& Response_m::add() const {
  return add_ != NULL ? *add_ : *default_instance_->add_;
}
inline ::PFK::TestMsgs::ResponseAdd_m* Response_m::mutable_add() {
  set_has_add();
  if (add_ == NULL) add_ = new ::PFK::TestMsgs::ResponseAdd_m;
  return add_;
}
inline ::PFK::TestMsgs::ResponseAdd_m* Response_m::release_add() {
  clear_has_add();
  ::PFK::TestMsgs::ResponseAdd_m* temp = add_;
  add_ = NULL;
  return temp;
}
inline void Response_m::set_allocated_add(::PFK::TestMsgs::ResponseAdd_m* add) {
  delete add_;
  add_ = add;
  if (add) {
    set_has_add();
  } else {
    clear_has_add();
  }
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace TestMsgs
}  // namespace PFK

#ifndef SWIG
namespace google {
namespace protobuf {

template <>
inline const EnumDescriptor* GetEnumDescriptor< ::PFK::TestMsgs::CommandType>() {
  return ::PFK::TestMsgs::CommandType_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::PFK::TestMsgs::ResponseType>() {
  return ::PFK::TestMsgs::ResponseType_descriptor();
}

}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_msgs_2eproto__INCLUDED
