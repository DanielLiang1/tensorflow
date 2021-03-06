/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <string>
#include <vector>

#include "tensorflow/lite/delegates/external/external_delegate.h"
#include "tensorflow/lite/tools/delegates/delegate_provider.h"

namespace tflite {
namespace tools {

// Split a given string to a vector of string using a delimiter character
std::vector<std::string> SplitString(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream ss(str);
  while (std::getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}


// External delegate provider used to dynamically load delegate libraries
// Note: Assumes the lifetime of the provider exceeds the usage scope of
// the generated delegates.
class ExternalDelegateProvider : public DelegateProvider {
 public:
  ExternalDelegateProvider() {
    default_params_.AddParam("external_delegate_path",
                             ToolParam::Create<std::string>(""));
    default_params_.AddParam("external_delegate_options",
                             ToolParam::Create<std::string>(""));
  }

  std::vector<Flag> CreateFlags(ToolParams* params) const final;

  void LogParams(const ToolParams& params) const final;

  TfLiteDelegatePtr CreateTfLiteDelegate(const ToolParams& params) const final;

  std::string GetName() const final { return "EXTERNAL"; }
};
REGISTER_DELEGATE_PROVIDER(ExternalDelegateProvider);

std::vector<Flag> ExternalDelegateProvider::CreateFlags(
    ToolParams* params) const {
  std::vector<Flag> flags = {
      CreateFlag<std::string>("external_delegate_path", params,
                              "The library path for the underlying external."),
      CreateFlag<std::string>(
          "external_delegate_options", params,
          "Comma-separated options to be passed to the external delegate")};
  return flags;
}

void ExternalDelegateProvider::LogParams(const ToolParams& params) const {
  TFLITE_LOG(INFO) << "External delegate path : ["
                   << params.Get<std::string>("external_delegate_path") << "]";
  TFLITE_LOG(INFO) << "External delegate options : ["
                   << params.Get<std::string>("external_delegate_options")
                   << "]";
}

TfLiteDelegatePtr ExternalDelegateProvider::CreateTfLiteDelegate(
    const ToolParams& params) const {
  TfLiteDelegatePtr delegate(nullptr, [](TfLiteDelegate*) {});
  std::string lib_path = params.Get<std::string>("external_delegate_path");
  if (!lib_path.empty()) {
    auto delegate_options =
        TfLiteExternalDelegateOptionsDefault(lib_path.c_str());

    // Parse delegate options
    const std::vector<std::string> options =
        SplitString(params.Get<std::string>("external_delegate_options"), ';');
    std::vector<std::string> keys, values;
    for (const auto& option : options) {
      auto key_value = SplitString(option, ':');
      if (key_value.size() == 2) {
        delegate_options.insert(&delegate_options, key_value[0].c_str(),
                                key_value[1].c_str());
      }
    }

    auto external_delegate = TfLiteExternalDelegateCreate(&delegate_options);
    return TfLiteDelegatePtr(external_delegate, [](TfLiteDelegate* delegate) {
      TfLiteExternalDelegateDelete(delegate);
    });
  }
  return delegate;
}

}  // namespace tools
}  // namespace tflite
