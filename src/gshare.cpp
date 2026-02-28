//
// Copyright 2026 Blaise Tine
//
// Licensed under the Apache License;
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <iostream>
#include <assert.h>
#include <util.h>
#include "types.h"
#include "core.h"
#include "debug.h"

using namespace tinyrv;

////////////////////////////////////////////////////////////////////////////////

GShare::GShare(uint32_t BTB_size, uint32_t BHR_size)
  : BTB_(BTB_size, BTB_entry_t{false, 0x0, 0x0})
  , PHT_((1 << BHR_size), 0x0)
  , BHR_(0x0)
  , BTB_shift_(log2ceil(BTB_size))
  , BTB_mask_(BTB_size-1)
  , BHR_mask_((1 << BHR_size)-1) {
  //--
}

GShare::~GShare() {
  //--
}

uint32_t GShare::predict(uint32_t PC) {
  uint32_t next_PC = PC + 4;
  bool predict_taken = false;

  // TODO:
  uint32_t pht_index = ((PC >> 2) ^ BHR_) & BHR_mask_; // index into PHT
  predict_taken = (PHT_[pht_index] >= 2); // >= 2 means taken
  if (predict_taken) {
    // index into BTB
    uint32_t btb_index = (PC >> 2) & BTB_mask_;
    auto& btb_entry = BTB_[btb_index];
    // only trust BTB if valid and tag matches
    if (btb_entry.valid && btb_entry.tag == PC) {
      next_PC = btb_entry.target;
    }
    else {
      predict_taken = false;
      next_PC = PC + 4;
    }
  }
  DT(3, "*** GShare: predict PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GShare::update(uint32_t PC, uint32_t next_PC, bool taken) {
  DT(3, "*** GShare: update PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", taken=" << taken);

  // TODO:
  uint32_t pht_index = ((PC >> 2) ^ BHR_) & BHR_mask_; // update saturating counter
  if (taken) {
    if (PHT_[pht_index] < 3) {
      PHT_[pht_index]++;
    }
  }
  else {
    if (PHT_[pht_index] > 0) {
      PHT_[pht_index]--;
    }
  }
  if (taken) { // store actual target whenever branch is taken
    uint32_t btb_index = (PC >>2) & BTB_mask_;
    BTB_[btb_index].valid = true;
    BTB_[btb_index].tag = PC;
    BTB_[btb_index].target = next_PC;
  }
  BHR_ = ((BHR_ << 1) | (taken ? 1 : 0)) & BHR_mask_; // shift in actual outcome
}

///////////////////////////////////////////////////////////////////////////////

GSharePlus::GSharePlus(uint32_t BTB_size, uint32_t BHR_size)
  : BTB_(BTB_size, BTB_entry_t{false, 0x0, 0x0}) // init BTB_ w/ BTB_size entries
  , GPHT_((1 << BHR_size), 0x0) // init GPHT_ w/ 2^BHR_size zeroes
  , LPHT_(BTB_size, 0x0) // LPHT_ w/ BTB_size zeroes
  , META_((1 << BHR_size), 0x0) // same
  , BHR_(0x0) // starts at 0
  , BTB_mask_(BTB_size-1)
  , BHR_mask_((1 << BHR_size)-1) {
  //--
}

GSharePlus::~GSharePlus() {
  //--
}

uint32_t GSharePlus::predict(uint32_t PC) {
  uint32_t next_PC = PC + 4;

  uint32_t gpht_index = ((PC >> 2) ^ BHR_) & BHR_mask_;
  bool gshare_taken = (GPHT_[gpht_index] >= 2); // gshare prediction
  
  uint32_t local_index = (PC >> 2) & BTB_mask_;
  bool local_taken = (LPHT_[local_index] >= 2);

  uint32_t meta_index = (PC >> 2) & BTB_mask_;
  bool use_gshare = (META_[meta_index] >=2);
  bool predict_taken = use_gshare ? gshare_taken : local_taken;

  // TODO: extra credit component
  if (predict_taken) {
    uint32_t btb_index = (PC >> 2) & BTB_mask_;
    auto& entry = BTB_[btb_index];
    if (entry.valid && entry.tag == PC) {
      next_PC = entry.target;
    }
    else {
      predict_taken = false;
      next_PC = PC + 4;
    }
  }

  DT(3, "*** GShare+: predict PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GSharePlus::update(uint32_t PC, uint32_t next_PC, bool taken) {
  DT(3, "*** GShare+: update PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", taken=" << taken);

  // TODO: extra credit component
}


