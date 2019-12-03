#include "oneflow/core/operator/naive_model_update_op.h"
#include "oneflow/core/job/sbp_signature_builder.h"

namespace oneflow {

void NormalModelUpdtOp::InitFromOpConf() {
  const PbMessage& conf = this->GetCustomizedConf();
  EnrollInputBn("model_diff", false);
  const std::string total_instance_num_diff =
      GetValFromPbMessage<std::string>(conf, "total_instance_num_diff");
  if (!total_instance_num_diff.empty()) { EnrollInputBn("total_instance_num_diff", false); }
  EnrollInputBn("model", false)->set_is_mutable(true);
  EnrollInputBn("learning_rate", false);
  EnrollInputBn("train_step", false);
  const auto& user_conf = *GetMsgPtrFromPbMessage<NormalModelUpdateOpUserConf>(conf, "user_conf");
  if (user_conf.has_clip_conf() && user_conf.clip_conf().has_clip_by_global_norm()) {
    EnrollTmpBn("data_tmp");
  }
  MdUpdtVirtualInitFromOpConf();
}

Maybe<void> NormalModelUpdtOp::InferBlobDescs(
    std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx) const {
  const PbMessage& conf = this->GetCustomizedConf();
  const auto& user_conf = *GetMsgPtrFromPbMessage<NormalModelUpdateOpUserConf>(conf, "user_conf");
  if (user_conf.has_clip_conf() && user_conf.clip_conf().has_clip_by_global_norm()) {
    *GetBlobDesc4BnInOp("data_tmp") = *GetBlobDesc4BnInOp("model_diff");
    GetBlobDesc4BnInOp("data_tmp")->mut_shape() = Shape({1});
  }
  return MdUpdtVirtualInferBlobDescs(GetBlobDesc4BnInOp, parallel_ctx);
}

const PbMessage& NormalModelUpdtOp::GetCustomizedConf() const {
  return op_conf().normal_mdupdt_conf();
}

LogicalBlobId NormalModelUpdtOp::obn2lbi(const std::string& output_bn) const {
  const google::protobuf::Descriptor* desc = GetCustomizedConf().GetDescriptor();
  const google::protobuf::FieldDescriptor* fd = desc->FindFieldByName(output_bn);
  CHECK(fd);
  return GenLogicalBlobId(GetValFromCustomizedConf<std::string>(output_bn));
}

Maybe<void> NormalModelUpdtOp::InferBatchAxis(
    std::function<OptInt64*(const std::string&)> BatchAxis4BnInOp) const {
  return Maybe<void>::Ok();
}

Maybe<void> NormalModelUpdtOp::GetSbpSignatures(
    const std::function<Maybe<const BlobDesc*>(const std::string&)>& LogicalBlobDesc4Ibn,
    SbpSignatureList* sbp_sig_list) const {
  const auto& bns = AlwaysBroadcastParallelBns();
  PbRpf<std::string> broadcast_bns = {bns.begin(), bns.end()};
  const PbMessage& conf = this->GetCustomizedConf();
  const std::string total_instance_num_diff =
      GetValFromPbMessage<std::string>(conf, "total_instance_num_diff");
  if (!total_instance_num_diff.empty()) { *broadcast_bns.Add() = "total_instance_num_diff"; }
  *broadcast_bns.Add() = "learning_rate";
  *broadcast_bns.Add() = "train_step";
  FOR_RANGE(int64_t, i, 0, JUST(LogicalBlobDesc4Ibn("model"))->shape().NumAxes()) {
    SbpSignatureBuilder()
        .Split(input_bns(), i)
        .Broadcast(broadcast_bns)
        .Build(sbp_sig_list->mutable_sbp_signature()->Add());
  }
  return Maybe<void>::Ok();
}

REGISTER_OP_CREATOR(OperatorConf::kNormalMdupdtConf, [](const OperatorConf& op_conf) -> Operator* {
  return NewObj<NormalModelUpdtOp>(op_conf.normal_mdupdt_conf().user_conf().normal_mdupdt_case());
});

}  // namespace oneflow
