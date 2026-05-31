// third_party/blink/renderer/modules/horizon_vm/horizon_vm.cc

#include "third_party/blink/renderer/modules/horizon_vm/horizon_vm.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_arraybuffer_arraybufferview.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/horizon_vm/horizon_vm_module.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/horizon_vm/cpp/include/horizon_vm/loader.h"

namespace blink {

namespace {

std::optional<horizon::Module> ParseBytecode(const uint8_t* data, size_t size,
                                               std::string& out_error) {
  return horizon::ParseModule(data, size, &out_error);
}

std::pair<const uint8_t*, size_t> ExtractBytes(const V8BufferSource* source) {
  if (source->IsArrayBuffer()) {
    auto* buf = source->GetAsArrayBuffer();
    return {static_cast<const uint8_t*>(buf->Data()), buf->ByteLength()};
  }
  auto view = source->GetAsArrayBufferView();
  return {static_cast<const uint8_t*>(view->BaseAddress()),
          view->byteLength()};
}

}  // namespace

// static
ScriptPromise<HorizonVMModule> HorizonVM::compile(
    ScriptState* script_state,
    const V8BufferSource* bytes,
    ExceptionState& exception_state) {
  auto [data, size] = ExtractBytes(bytes);
  std::string error;
  auto maybe_module = ParseBytecode(data, size, error);

  if (!maybe_module) {
    return ScriptPromise<HorizonVMModule>::RejectWithDOMException(
        script_state,
        MakeGarbageCollected<DOMException>(DOMExceptionCode::kDataError,
                                           String::FromUtf8(error)));
  }

  auto* module = HorizonVMModule::Create(std::move(*maybe_module));
  auto* resolver =
      MakeGarbageCollected<ScriptPromiseResolver<HorizonVMModule>>(script_state);
  resolver->Resolve(module);
  return resolver->Promise();
}

// static
ScriptPromise<HorizonVMModule> HorizonVM::compileStreaming(
    ScriptState* script_state,
    ScriptPromise<Response> /*source*/,
    ExceptionState& exception_state) {
  exception_state.ThrowDOMException(DOMExceptionCode::kNotSupportedError,
                                     "HorizonVM.compileStreaming() is not yet implemented");
  return ScriptPromise<HorizonVMModule>();
}

// static
ScriptPromise<IDLBoolean> HorizonVM::validate(
    ScriptState* script_state,
    const V8BufferSource* bytes,
    ExceptionState& /*exception_state*/) {
  auto [data, size] = ExtractBytes(bytes);
  std::string err;
  bool valid = ParseBytecode(data, size, err).has_value();
  v8::Local<v8::Value> v8_bool =
      v8::Boolean::New(script_state->GetIsolate(), valid);
  return ScriptPromise<IDLBoolean>::FromV8Value(script_state, v8_bool);
}

}  // namespace blink
