/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#ifndef TENSORFLOW_COMPILER_TF2TENSORRT_CONVERT_TRT_TENSOR_PROXY_H
#define TENSORFLOW_COMPILER_TF2TENSORRT_CONVERT_TRT_TENSOR_PROXY_H

#include <vector>
#include <string>
#include <cassert>
#include <memory>

#if GOOGLE_CUDA && GOOGLE_TENSORRT
#include "third_party/tensorrt/NvInfer.h"
namespace tensorflow {

namespace tensorrt {

namespace {
  nvinfer1::Dims GetTestDims(const std::vector<int>& d) {
    nvinfer1::Dims dims;
    dims.nbDims = d.size();
    for (int i = 0; i < d.size(); ++i) {
      dims.d[i] = d[i];
    }
  }
}
// A fake ITensor implementation used to check whether the TF-TRT converter can
// handle specific node. We only need shape and type information, and the
// converter won't (and shouldn't) use this to build the TRT network.
class SimpleITensor {
 public:
  SimpleITensor(nvinfer1::DataType trt_dtype, const nvinfer1::Dims& trt_dims)
      : trt_dtype_(trt_dtype), trt_dims_(trt_dims) {}

  void setName(const char* name) {}

  const char* getName() const { return ""; }

  void setDimensions(nvinfer1::Dims dimensions) {
    trt_dims_ = dimensions;
  }

  nvinfer1::Dims getDimensions() const { return trt_dims_; }

  void setType(nvinfer1::DataType trt_dtype) {
    trt_dtype_ = trt_dtype;
  }

  nvinfer1::DataType getType() const { return trt_dtype_; }

  bool isNetworkInput() const { return false; }

  bool isNetworkOutput() const { return false; }

  void setBroadcastAcrossBatch(bool broadcastAcrossBatch) {}

  bool getBroadcastAcrossBatch() const { return false; }

  nvinfer1::TensorLocation getLocation() const { return location_; }

  void setLocation(nvinfer1::TensorLocation location) {
    location_ = location;
  }
  bool setDynamicRange(float min, float max) {
    dynamic_range_ = std::max(std::abs(min), std::abs(max));
    return true;
  }
  
  float getDynamicRange() const { return dynamic_range_; }
  bool dynamicRangeIsSet() const { return true; }

  void resetDynamicRange() {}

  float getDynamicRangeMin() const { return 0.f; }

  float getDynamicRangeMax() const { return 0.f; }

  void setAllowedFormats(nvinfer1::TensorFormats formats) {}

  nvinfer1::TensorFormats getAllowedFormats() const { return 1; }

  bool isShapeTensor() const { return false; }
  bool isExecutionTensor() const { return true; }

 private:
  nvinfer1::DataType trt_dtype_;
  nvinfer1::Dims trt_dims_;
  std::string name_;
  nvinfer1::TensorLocation location_;
  float dynamic_range_;
};

// Fake ITensor implementation for testing purposes.
class FakeITensor {
 public:
  FakeITensor() : dynamic_range_(0.0f) {}

  FakeITensor(const nvinfer1::Dims& dims) : dims_(dims), dynamic_range_(0.0f) {}

  FakeITensor(const std::vector<int>& dims)
      : dims_(GetTestDims(dims)), dynamic_range_(0.0f) {}

  void setName(const char* name) { name_ = name; }

  const char* getName() const { return name_.c_str(); }

  void setDimensions(nvinfer1::Dims dimensions) { dims_ = dimensions; }

  nvinfer1::Dims getDimensions() const { return dims_; }

  void setType(nvinfer1::DataType type) { type_ = type; }

  nvinfer1::DataType getType() const { return type_; }

  bool isNetworkInput() const { return false; }

  bool isNetworkOutput() const { return false; }

  void setBroadcastAcrossBatch(bool broadcastAcrossBatch) {}

  bool getBroadcastAcrossBatch() const { return false; }

  nvinfer1::TensorLocation getLocation() const { return location_; }

  void setLocation(nvinfer1::TensorLocation location) {
    location_ = location;
  }

  bool setDynamicRange(float min, float max) {
    dynamic_range_ = std::max(std::abs(min), std::abs(max));
    return true;
  }

  bool dynamicRangeIsSet() const { return true; }

  void resetDynamicRange() {}

  float getDynamicRangeMin() const { return 0.f; }

  float getDynamicRangeMax() const { return 0.f; }

  void setAllowedFormats(nvinfer1::TensorFormats formats) {}

  nvinfer1::TensorFormats getAllowedFormats() const { return 1; }

  bool isShapeTensor() const { return false; }
  bool isExecutionTensor() const { return true; }

 private:
  std::string name_;
  nvinfer1::Dims dims_;
  nvinfer1::DataType type_;
  nvinfer1::TensorLocation location_;
  float dynamic_range_;
};

enum class TensorType : int
{
  kNONE,
  kTRT,
  kSIMPLE,
  kFAKE
};

class ITensorProxy
{
public:
  //! ITensor not owned
  ITensorProxy(nvinfer1::ITensor* trt_tensor)
    : trt_tensor_(trt_tensor),
      ttype_(TensorType::kTRT) {}

  //! SimpleITensor owned
  ITensorProxy(SimpleITensor* simple_itensor)
    : simple_tensor_(simple_itensor),
      ttype_(TensorType::kSIMPLE) {}

  //! FakeITensor owned
  ITensorProxy(FakeITensor* fake_tensor)
    : fake_tensor_(fake_tensor),
      ttype_(TensorType::kFAKE) {}
    
  //! SimpleITensor owned
  explicit ITensorProxy(nvinfer1::DataType trt_dtype, const nvinfer1::Dims& trt_dims)
    : simple_tensor_(std::unique_ptr<SimpleITensor>(new SimpleITensor(trt_dtype, trt_dims))),
      ttype_(TensorType::kSIMPLE) {}

  //! FakeITensor variants for testing purposes
  ITensorProxy()
    : fake_tensor_(std::unique_ptr<FakeITensor>(new FakeITensor())),
      ttype_(TensorType::kFAKE) {}

  explicit ITensorProxy(const nvinfer1::Dims& dims)
    : fake_tensor_(std::unique_ptr<FakeITensor>(new FakeITensor(dims))),
      ttype_(TensorType::kFAKE) {}

  explicit ITensorProxy(const std::vector<int>& dims)
    : fake_tensor_(std::unique_ptr<FakeITensor>(new FakeITensor(dims))),
      ttype_(TensorType::kFAKE) {}

  bool is_trt_tensor() const
  {
    assert(validate());
    assert(ttype_ == TensorType::kTRT);
    return trt_tensor_ != nullptr;
  }

  bool is_simple_tensor() const
  {
    assert(validate());
    assert(ttype_ == TensorType::kSIMPLE);
    return simple_tensor_ != nullptr;
  }

  bool is_fake_tensor() const
  {
    assert(validate());
    assert(ttype_ == TensorType::kFAKE);
    return fake_tensor_ != nullptr;
  }

  TensorType ttype() const
  {
    return ttype_;
  }

  nvinfer1::ITensor* trt_tensor() const
  {
    assert(trt_tensor_ != nullptr);
    assert(ttype_ == TensorType::kTRT);
    return trt_tensor_;
  }

  SimpleITensor* simple_tensor() const
  {
    assert(simple_tensor_ != nullptr);
    assert(ttype_ == TensorType::kTRT);
    return simple_tensor_.get();
  }

  FakeITensor* fake_tensor() const
  {
    assert(fake_tensor_ != nullptr);
    assert(ttype_ == TensorType::kTRT);
    return fake_tensor_.get();
  }
  
  void setName(const char* name)
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setName(name);
      case TensorType::kSIMPLE: return simple_tensor_->setName(name);
      case TensorType::kFAKE: return fake_tensor_->setName(name);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  const char* getName() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getName();
      case TensorType::kSIMPLE: return simple_tensor_->getName();
      case TensorType::kFAKE: return fake_tensor_->getName();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  void setDimensions(nvinfer1::Dims dimensions)
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setDimensions(dimensions);
      case TensorType::kSIMPLE: return simple_tensor_->setDimensions(dimensions);
      case TensorType::kFAKE: return fake_tensor_->setDimensions(dimensions);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  nvinfer1::Dims getDimensions() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getDimensions();
      case TensorType::kSIMPLE: return simple_tensor_->getDimensions();
      case TensorType::kFAKE: return fake_tensor_->getDimensions();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  void setType(nvinfer1::DataType type)
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setType(type);
      case TensorType::kSIMPLE: return simple_tensor_->setType(type);
      case TensorType::kFAKE: return fake_tensor_->setType(type);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  nvinfer1::DataType getType() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getType();
      case TensorType::kSIMPLE: return simple_tensor_->getType();
      case TensorType::kFAKE: return fake_tensor_->getType();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  bool isNetworkInput() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->isNetworkInput();
      case TensorType::kSIMPLE: return simple_tensor_->isNetworkInput();
      case TensorType::kFAKE: return fake_tensor_->isNetworkInput();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  bool isNetworkOutput() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->isNetworkOutput();
      case TensorType::kSIMPLE: return simple_tensor_->isNetworkOutput();
      case TensorType::kFAKE: return fake_tensor_->isNetworkOutput();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  void setBroadcastAcrossBatch(bool broadcastAcrossBatch)
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setBroadcastAcrossBatch(broadcastAcrossBatch);
      case TensorType::kSIMPLE: return simple_tensor_->setBroadcastAcrossBatch(broadcastAcrossBatch);
      case TensorType::kFAKE: return fake_tensor_->setBroadcastAcrossBatch(broadcastAcrossBatch);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  bool getBroadcastAcrossBatch() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getBroadcastAcrossBatch();
      case TensorType::kSIMPLE: return simple_tensor_->getBroadcastAcrossBatch();
      case TensorType::kFAKE: return fake_tensor_->getBroadcastAcrossBatch();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  nvinfer1::TensorLocation getLocation() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getLocation();
      case TensorType::kSIMPLE: return simple_tensor_->getLocation();
      case TensorType::kFAKE: return fake_tensor_->getLocation();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  void setLocation(nvinfer1::TensorLocation location) {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setLocation(location);
      case TensorType::kSIMPLE: return simple_tensor_->setLocation(location);
      case TensorType::kFAKE: return fake_tensor_->setLocation(location);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  bool setDynamicRange(float min, float max) {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setDynamicRange(min, max);
      case TensorType::kSIMPLE: return simple_tensor_->setDynamicRange(min, max);
      case TensorType::kFAKE: return fake_tensor_->setDynamicRange(min, max);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  bool dynamicRangeIsSet() const { 
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->dynamicRangeIsSet();
      case TensorType::kSIMPLE: return simple_tensor_->dynamicRangeIsSet();
      case TensorType::kFAKE: return fake_tensor_->dynamicRangeIsSet();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");
  }

  void resetDynamicRange() {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->resetDynamicRange();
      case TensorType::kSIMPLE: return simple_tensor_->resetDynamicRange();
      case TensorType::kFAKE: return fake_tensor_->resetDynamicRange();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");    
  }

  float getDynamicRangeMin() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getDynamicRangeMin();
      case TensorType::kSIMPLE: return simple_tensor_->getDynamicRangeMin();
      case TensorType::kFAKE: return fake_tensor_->getDynamicRangeMin();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

  float getDynamicRangeMax() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getDynamicRangeMax();
      case TensorType::kSIMPLE: return simple_tensor_->getDynamicRangeMax();
      case TensorType::kFAKE: return fake_tensor_->getDynamicRangeMax();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

  void setAllowedFormats(nvinfer1::TensorFormats formats)
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->setAllowedFormats(formats);
      case TensorType::kSIMPLE: return simple_tensor_->setAllowedFormats(formats);
      case TensorType::kFAKE: return fake_tensor_->setAllowedFormats(formats);
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

  nvinfer1::TensorFormats getAllowedFormats() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->getAllowedFormats();
      case TensorType::kSIMPLE: return simple_tensor_->getAllowedFormats();
      case TensorType::kFAKE: return fake_tensor_->getAllowedFormats();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

  bool isShapeTensor() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->isShapeTensor();
      case TensorType::kSIMPLE: return simple_tensor_->isShapeTensor();
      case TensorType::kFAKE: return fake_tensor_->isShapeTensor();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

  bool isExecutionTensor() const
  {
    switch(ttype_)
    {
      case TensorType::kTRT: return trt_tensor_->isExecutionTensor();
      case TensorType::kSIMPLE: return simple_tensor_->isExecutionTensor();
      case TensorType::kFAKE: return fake_tensor_->isExecutionTensor();
      case TensorType::kNONE: assert(0 && "Unsupported itensor_ type");
    }
    assert(0 && "Should not come here.");   
  }

private:
  bool validate() const
  {
    return trt_tensor_ && !simple_tensor_ && !fake_tensor_
      || !trt_tensor_ && simple_tensor_ && !fake_tensor_
      || !trt_tensor_ && !simple_tensor_ && fake_tensor_;
  }

  TensorType ttype_{TensorType::kNONE};

  // When ITensorProxy represents an ITensor, the ITensor can be either passed
  // by the caller via the constructor that takes an ITensor* as parameter, or
  // be created as a SimpleITensor.
  //
  // In the first case, the ITensor pointer is stored in 'tensor_' below, and
  // the ITensor itself is not owned by this class. This method is used by
  // Converter (e.g. AddInputTensor) and op converters during TRT network
  // construction, where the TRT network owns the ITensor.
  //
  nvinfer1::ITensor* trt_tensor_ = nullptr; // Not owned.
  // In the second case, the created SimpleITensor is stored in
  // 'simple_itensor_' below and is owned by this class. SimpleITensor is a fake
  // implementation of ITensor and is used only by TrtNodeValidator to validate
  // the graph nodes.
  std::shared_ptr<SimpleITensor> simple_tensor_ = nullptr;
  std::unique_ptr<FakeITensor> fake_tensor_ = nullptr;
};

class ITensorProxyPtr
{
public:
  ITensorProxyPtr(nullptr_t) : p_(nullptr) {}
  ITensorProxyPtr(ITensorProxy* p) : p_(p) {}
  ITensorProxyPtr(nvinfer1::ITensor* p) : p_(new ITensorProxy(p)) {}
  ITensorProxyPtr(SimpleITensor* p) : p_(new ITensorProxy(p)) {}
  
  // FakeITensor
  ITensorProxyPtr(FakeITensor* p) : p_(new ITensorProxy(p)) {}
  ITensorProxyPtr() : p_(new ITensorProxy()) {}
  ITensorProxyPtr(const nvinfer1::Dims& dims) : p_(new ITensorProxy(dims)) {}
  ITensorProxyPtr(const std::vector<int>& dims) : p_(new ITensorProxy(dims)) {}

  ITensorProxy* p_{nullptr};
  ITensorProxy* operator->()
  {
    return p_;
  }
  ITensorProxy* operator->() const
  {
    return p_;
  }
  ITensorProxy* operator*()
  {
    return p_;
  }
  ITensorProxy* operator*() const
  {
    return p_;
  }
};

inline bool operator==(const ITensorProxyPtr& p1, const ITensorProxyPtr& p2)
{
  if (p1.p_ == nullptr)
  {
    return p2.p_ == nullptr;
  }
  if (p2.p_ == nullptr)
  {
    return p1.p_ == nullptr;
  }
  return (p1->ttype() == p2->ttype())
    && (
      (p1->ttype() == TensorType::kTRT && p1->trt_tensor() == p2->trt_tensor())
      || (p1->ttype() == TensorType::kTRT && p1->fake_tensor() == p2->fake_tensor())
      || (p1->ttype() == TensorType::kTRT && p1->simple_tensor() == p2->simple_tensor())
    );
}

struct ITensorProxyHash
{
  size_t operator() (const ITensorProxyPtr& tensor) const
  {
    return reinterpret_cast<std::uintptr_t>(tensor.p_);
  }
};

} // namespace tensorrt
} // namespace tensorflow
#endif  // GOOGLE_CUDA && GOOGLE_TENSORRT

#endif // TENSORFLOW_COMPILER_TF2TENSORRT_CONVERT_TRT_TENSOR_PROXY_H
