// Copyright 2004-present Facebook. All Rights Reserved.

#include "JSCHelpers.h"

#include <JavaScriptCore/JSStringRef.h>
#include <glog/logging.h>

#include "Value.h"

#if WITH_FBJSCEXTENSIONS
#include <jsc_preparsing_cache.h>
#endif

namespace facebook {
namespace react {

void installGlobalFunction(
    JSGlobalContextRef ctx,
    const char* name,
    JSObjectCallAsFunctionCallback callback) {
  JSStringRef jsName = JSStringCreateWithUTF8CString(name);
  JSObjectRef functionObj = JSObjectMakeFunctionWithCallback(
      ctx, jsName, callback);
  JSObjectRef globalObject = JSContextGetGlobalObject(ctx);
  JSObjectSetProperty(ctx, globalObject, jsName, functionObj, 0, NULL);
  JSStringRelease(jsName);
}

JSValueRef makeJSCException(
    JSContextRef ctx,
    const char* exception_text) {
  JSStringRef message = JSStringCreateWithUTF8CString(exception_text);
  JSValueRef exceptionString = JSValueMakeString(ctx, message);
  JSStringRelease(message);
  return JSValueToObject(ctx, exceptionString, NULL);
}

JSValueRef evaluateScript(JSContextRef context, JSStringRef script, JSStringRef source, const char *cachePath) {
  JSValueRef exn, result;
#if WITH_FBJSCEXTENSIONS
  // Only evaluate the script using pre-parsing cache if the script comes from
  //  a bundle file and a cache path is given.
  if (source && cachePath){
    // If evaluating an application script, send it through `JSEvaluateScriptWithCache()`
    //  to add cache support.
    result = JSEvaluateScriptWithCache(context, script, NULL, source, 0, &exn, cachePath);
  } else {
    result = JSEvaluateScript(context, script, NULL, source, 0, &exn);
  }
#else
  result = JSEvaluateScript(context, script, NULL, source, 0, &exn);
#endif
  if (result == nullptr) {
    Value exception = Value(context, exn);
    std::string exceptionText = exception.toString().str();
    LOG(ERROR) << "Got JS Exception: " << exceptionText;
    auto line = exception.asObject().getProperty("line");

    std::ostringstream locationInfo;
    std::string file = source != nullptr ? String::ref(source).str() : "";
    locationInfo << "(" << (file.length() ? file : "<unknown file>");
    if (line != nullptr && line.isNumber()) {
      locationInfo << ":" << line.asInteger();
    }
    locationInfo << ")";
    throwJSExecutionException("%s %s", exceptionText.c_str(), locationInfo.str().c_str());
  }
  return result;
}

} }
