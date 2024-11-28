#pragma once

#if __has_include(<onnxruntime/dml_provider_factory.h>)
#pragma message("MAA_WITH_DML")
#define MAA_WITH_DML
#include <onnxruntime/dml_provider_factory.h>
#endif

#if __has_include(<onnxruntime/coreml_provider_factory.h>)
#pragma message("MAA_WITH_COREML")
#define MAA_WITH_COREML
#include <onnxruntime/coreml_provider_factory.h>
#endif
