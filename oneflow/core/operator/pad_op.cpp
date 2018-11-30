#include "oneflow/core/operator/pad_op.h"

namespace oneflow {

void PadOp::InitFromOpConf() {
  CHECK(op_conf().has_pad_conf());
  EnrollInputBn("in");
  EnrollOutputBn("out");

  EnrollDataTmpBn("inshape_at");
  EnrollDataTmpBn("outshape_at");
  EnrollDataTmpBn("inshape_count");
  EnrollDataTmpBn("outshape_count");
}

const PbMessage& PadOp::GetCustomizedConf() const { return op_conf().pad_conf(); }

void PadOp::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                            const ParallelContext* parallel_ctx) const {
  // in
  const BlobDesc* in_blob_desc = GetBlobDesc4BnInOp("in");
  const Shape& in_shape = in_blob_desc->shape();
  int64_t num_axes = in_blob_desc->shape().NumAxes();
  CHECK_GE(num_axes, 3);
  CHECK_LE(num_axes, 5);
  CHECK_EQ(in_blob_desc->data_type(), Global<JobDesc>::Get()->DefaultDataType());
  // out
  const std::string data_format = op_conf().pad_conf().data_format();
  int64_t dims = in_blob_desc->shape().NumAxes() - 2;
  std::vector<int64_t> in = {GetInDim(in_shape, data_format, 0, dims),
                             GetInDim(in_shape, data_format, 1, dims),
                             GetInDim(in_shape, data_format, 2, dims)};
  BlobDesc* out_blob_desc = GetBlobDesc4BnInOp("out");
  *out_blob_desc = *in_blob_desc;
  int64_t in_c = 0;
  if (data_format== "channels_first") {
    in_c = in_shape.At(1);
  } else if (data_format == "channels_last") {
    in_c = in_shape.At(in_shape.NumAxes() - 1);
  } else {
    UNIMPLEMENTED();
  }
  out_blob_desc->mut_shape() = GetOutShape(in_shape.At(0), in_c, dims, data_format, in);
  // tmp blobs
  BlobDesc* inshape_at_blob_desc = GetBlobDesc4BnInOp("inshape_at");
  inshape_at_blob_desc->mut_shape() = Shape({num_axes});
  inshape_at_blob_desc->set_data_type(DataType::kInt32);

  BlobDesc* inshape_count_blob_desc = GetBlobDesc4BnInOp("inshape_count");
  inshape_count_blob_desc->mut_shape() = Shape({num_axes});
  inshape_count_blob_desc->set_data_type(DataType::kInt32);

  BlobDesc* outshape_at_blob_desc = GetBlobDesc4BnInOp("outshape_at");
  outshape_at_blob_desc->mut_shape() = Shape({num_axes});
  outshape_at_blob_desc->set_data_type(DataType::kInt32);

  BlobDesc* outshape_count_blob_desc = GetBlobDesc4BnInOp("outshape_count");
  outshape_count_blob_desc->mut_shape() = Shape({num_axes});
  outshape_count_blob_desc->set_data_type(DataType::kInt32);

}

Shape PadOp::GetOutShape(int64_t in_n, int64_t in_c, int64_t dims, 
                         std::string data_format, const std::vector<int64_t>& in) const {
  std::vector<int64_t> out_shape;
  if (dims == 1) {
    out_shape = {in.at(2) + 1};
  } else if (dims == 2) {
    out_shape = {in.at(1) + 1, in.at(2) + 1};
  } else if (dims == 3) {
    out_shape = {in.at(0) + 1, in.at(1) + 1, in.at(2) + 1};
  } else {
    UNIMPLEMENTED();
  }

  if (data_format == "channels_first") {
    out_shape.insert(out_shape.begin(), in_c);
  } else if (data_format == "channels_last") {
    out_shape.insert(out_shape.end(), in_c);
  } else {
    UNIMPLEMENTED();
  }
  out_shape.insert(out_shape.begin(), in_n);
  return Shape(out_shape);
}

REGISTER_OP(OperatorConf::kPadConf, PadOp);

}  // namespace oneflow