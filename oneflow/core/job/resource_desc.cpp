#include "oneflow/core/job/resource_desc.h"

namespace oneflow {

int64_t ResourceDesc::GetMachineId(const std::string& addr) const {
  int64_t machine_id = -1;
  int64_t machine_num = resource_.machine_size();
  FOR_RANGE(int64_t, i, 0, machine_num) {
    if (addr == resource_.machine(i).addr()) {
      machine_id = i;
      break;
    }
  }
  CHECK_GE(machine_id, 0);
  CHECK_LT(machine_id, machine_num);
  return machine_id;
}

}  // namespace oneflow
