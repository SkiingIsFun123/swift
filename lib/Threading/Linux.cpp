//==--- Linux.cpp - Threading abstraction implementation ------- -*-C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// Implements threading support for Linux
//
//===----------------------------------------------------------------------===//

#if SWIFT_THREADING_LINUX

#include "swift/Threading/Impl.h"
#include "swift/Threading/Errors.h"

namespace {

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"

class MainThreadRememberer {
private:
  pthread_t mainThread_;

public:
  MainThreadRememberer() { mainThread_ = pthread_self(); }

  pthread_t main_thread() const { return mainThread_; }
};

MainThreadRememberer rememberer;

#pragma clang diagnostic pop

} // namespace

using namespace swift;
using namespace threading_impl;

bool swift::threading_impl::thread_is_main() {
  return pthread_equal(pthread_self(), rememberer.main_thread());
}

void swift::threading_impl::once_slow(once_t &predicate, void (*fn)(void *),
                                      void *context) {
  linux::ulock_lock(&predicate.lock);
  if (predicate.flag.load(std::memory_order_acquire) == 0) {
    fn(context);
    predicate.flag.store(-1, std::memory_order_release);
  }
  linux::ulock_unlock(&predicate.lock);
}

llvm::Optional<swift::threading_impl::stack_bounds>
swift::threading_impl::thread_get_current_stack_bounds() {
  pthread_attr_t attr;
  size_t size = 0;
  void *begin = nullptr;

  if (!pthread_getattr_np(pthread_self(), &attr)) {
    if (!pthread_attr_getstack(&attr, &begin, &size)) {
      stack_bounds result = { begin, (char *)begin + size };

      pthread_attr_destroy(&attr);

      return result;
    }

    pthread_attr_destroy(&attr);
  }

  return {};
}

#endif // SWIFT_THREADING_LINUX
