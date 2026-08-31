#ifndef SNODEC_LOG_DETAIL_NATIVE_H
#define SNODEC_LOG_DETAIL_NATIVE_H

#include "Log.h"
#include "log/SemanticLogger.h"

namespace snode::log::detail {

    logger::LogScope nativeScope(const Scope& scope) noexcept;

} // namespace snode::log::detail

#endif // SNODEC_LOG_DETAIL_NATIVE_H
