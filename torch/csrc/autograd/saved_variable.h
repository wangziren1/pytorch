#pragma once

#include <torch/csrc/WindowsTorchApiMacro.h>
#include <torch/csrc/autograd/forward_grad.h>

#include <ATen/ATen.h>

#include <cstdint>
#include <memory>

namespace torch { namespace autograd {

using Variable = at::Tensor;
struct Node;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
TORCH_API extern const char* ERR_BACKWARD_TWICE;

/// A snapshot of a variable at a certain version. A `SavedVariable` stores
/// enough information to reconstruct a variable from a certain point in time.
class TORCH_API SavedVariable {
 public:
  SavedVariable() = default;
  SavedVariable(const Variable& variable, bool is_output, bool is_inplace_view=false);
  SavedVariable(const c10::optional<Variable>& variable, bool is_output, bool is_inplace_view=false);
  SavedVariable(SavedVariable&&) = default;
  SavedVariable& operator=(SavedVariable&&) = default;
  ~SavedVariable() {
    if (fw_grad_) {
      // See note [ Using ForwardGrad ]
      fw_grad_->clear();
    }
  }

  /// Reconstructs the saved variable. Pass `saved_for` as the gradient
  /// function if constructing the `SavedVariable` with it would have caused a
  /// circular reference.
  Variable unpack(std::shared_ptr<Node> saved_for = nullptr) const;

  void reset_data() {
    return data_.reset();
  }

  void reset_grad_function() {
    // No op
  }

 private:
  // The variable to save.
  // If this would create a circular reference, store its tensor_data and
  // and its metadata separately instead. In that case, the grad_fn must be
  // passed in to the unpack function when reconstructing the Variable.
  at::Tensor data_;

  // This field is used to store the forward AD gradients associated with
  // the saved Tensor. Note that this shared_ptr must never be shared with
  // either the saved Tensor or the unpacked Tensor. See note [ Using ForwardGrad ]
  std::shared_ptr<ForwardGrad> fw_grad_;

  // Weak version of grad_fn_ that prevents leaks in rebase_history() for
  // inplace views.
  std::weak_ptr<Node> weak_grad_fn_;
  c10::VariableVersion version_counter_;

  uint32_t saved_version_ = 0;
  uint32_t output_nr_ = 0;
  bool was_default_constructed_ = true;
  bool is_inplace_view_ = false;
  bool saved_original = false;
};
}} // namespace torch::autograd
