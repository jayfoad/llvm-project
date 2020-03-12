//===--- AMDGPUMacroFusion.cpp - AMDGPU Macro Fusion ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file This file contains the AMDGPU implementation of the DAG scheduling
///  mutation to pair instructions back to back.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUMacroFusion.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"

#include "llvm/CodeGen/MacroFusion.h"

using namespace llvm;

namespace {

/// Check if the instr pair, FirstMI and SecondMI, should be fused
/// together. Given SecondMI, when FirstMI is unspecified, then check if
/// SecondMI may be part of a fused pair at all.
static bool shouldScheduleAdjacent(const TargetInstrInfo &TII_,
                                   const TargetSubtargetInfo &TSI,
                                   const MachineInstr *FirstMI,
                                   const MachineInstr &SecondMI,
                                   int Latency) {
  const SIInstrInfo &TII = static_cast<const SIInstrInfo&>(TII_);

  switch (SecondMI.getOpcode()) {
  case AMDGPU::V_ADDC_U32_e64:
  case AMDGPU::V_SUBB_U32_e64:
  case AMDGPU::V_SUBBREV_U32_e64:
  case AMDGPU::V_CNDMASK_B32_e64: {
    // Try to cluster defs of condition registers to their uses. This improves
    // the chance VCC will be available which will allow shrinking to VOP2
    // encodings.
    if (!FirstMI)
      return true;

    const MachineBasicBlock &MBB = *FirstMI->getParent();
    const MachineRegisterInfo &MRI = MBB.getParent()->getRegInfo();
    const TargetRegisterInfo *TRI = MRI.getTargetRegisterInfo();
    const MachineOperand *Src2 = TII.getNamedOperand(SecondMI,
                                                     AMDGPU::OpName::src2);
    if (!FirstMI->definesRegister(Src2->getReg(), TRI))
      return false;

    // If fusing the instructions won't cause a stall, go ahead.
    if (Latency <= 1)
      return true;

    // The hardware can issue these instructions back-to-back with no stall if
    // the first one is v_cmp or v_add/sub with carry-out, and the second one is
    // v_cndmask or v_add/sub with carry-in.
    //
    // This crude check accepts any VALU instruction (v_cmp/add/sub are the only
    // ones that commonly occur) but rejects other reasonably common inputs like
    // s_and/or/xor.
    return TII.isVALU(*FirstMI);
  }
  }

  return false;
}

} // end namespace


namespace llvm {

std::unique_ptr<ScheduleDAGMutation> createAMDGPUMacroFusionDAGMutation () {
  return createMacroFusionDAGMutation(shouldScheduleAdjacent);
}

} // end namespace llvm
