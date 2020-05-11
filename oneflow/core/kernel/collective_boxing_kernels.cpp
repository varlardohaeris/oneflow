#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/job/collective_boxing_executor.h"
#include "oneflow/core/common/blocking_counter.h"
#include "oneflow/core/graph/boxing/collective_boxing_util.h"

namespace oneflow {

using namespace boxing::collective;

template<DeviceType device_type>
class CollectiveBoxingGenericKernel final : public KernelIf<device_type> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(CollectiveBoxingGenericKernel);
  CollectiveBoxingGenericKernel() = default;
  ~CollectiveBoxingGenericKernel() override = default;

 private:
  bool IsKernelLaunchSynchronized() const override { return false; }
  void ForwardDataContent(const KernelCtx& ctx,
                          std::function<Blob*(const std::string&)> BnInOp2Blob) const override;
};

template<DeviceType device_type>
void CollectiveBoxingGenericKernel<device_type>::ForwardDataContent(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  RuntimeRequestInfo request;
  const RankDesc& rank_desc = this->op_conf().collective_boxing_generic_conf().rank_desc();
  const DataType data_type = rank_desc.op_desc().data_type();
  if (GenericOpHasInput(rank_desc)) {
    const Blob* in = BnInOp2Blob("in");
    CHECK_EQ(in->data_type(), data_type);
    CHECK(in->shape() == ShapeView(GenericOpGetInputShape(rank_desc)));
    request.send_buff = in->dptr();
  } else {
    request.send_buff = nullptr;
  }
  if (GenericOpHasOutput(rank_desc)) {
    Blob* out = BnInOp2Blob("out");
    CHECK_EQ(out->data_type(), data_type);
    CHECK(out->shape() == ShapeView(GenericOpGetOutputShape(rank_desc)));
    request.recv_buff = out->mut_dptr();
  } else {
    request.recv_buff = nullptr;
  }
  std::shared_ptr<BlockingCounter> bc(new BlockingCounter(1));
  request.callback = [bc](const Maybe<void>& status) {
    CHECK(status.IsOk());
    bc->Decrease();
  };
  Global<CollectiveBoxingExecutor>::Get()->Enqueue(rank_desc, request);
  ctx.device_ctx->AddCallBack([bc]() { bc->WaitUntilCntEqualZero(); });
}

ADD_DEVICE_TYPE_KERNEL_CREATOR(OperatorConf::kCollectiveBoxingGenericConf,
                               CollectiveBoxingGenericKernel);

}  // namespace oneflow