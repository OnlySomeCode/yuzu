// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <unordered_map>
#include <vector>
#include "video_core/engines/maxwell_3d.h"
#include "video_core/macro/macro_hle.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra {

// HLE'd functions
static void HLE_771BB18C62444DA0(Engines::Maxwell3D& maxwell3d,
                                 const std::vector<u32>& parameters) {
    const u32 instance_count = parameters[2] & maxwell3d.GetRegisterValue(0xD1B);

    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0] &
                                                                        ~(0x3ffffff << 26)));
    maxwell3d.regs.vb_base_instance = parameters[5];
    maxwell3d.mme_draw.instance_count = instance_count;
    maxwell3d.regs.vb_element_base = parameters[3];
    maxwell3d.regs.index_array.count = parameters[1];
    maxwell3d.regs.index_array.first = parameters[4];

    if (maxwell3d.ShouldExecute()) {
        maxwell3d.GetRasterizer().Draw(true, true);
    }
    maxwell3d.regs.index_array.count = 0;
    maxwell3d.mme_draw.instance_count = 0;
}

static void HLE_0D61FC9FAAC9FCAD(Engines::Maxwell3D& maxwell3d,
                                 const std::vector<u32>& parameters) {
    const u32 count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);

    maxwell3d.regs.vertex_buffer.first = parameters[3];
    maxwell3d.regs.vertex_buffer.count = parameters[1];
    maxwell3d.regs.vb_base_instance = parameters[4];
    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0]));
    maxwell3d.mme_draw.instance_count = count;

    if (maxwell3d.ShouldExecute()) {
        maxwell3d.GetRasterizer().Draw(false, true);
    }
    maxwell3d.regs.vertex_buffer.count = 0;
    maxwell3d.mme_draw.instance_count = 0;
}

static void HLE_0217920100488FF7(Engines::Maxwell3D& maxwell3d,
                                 const std::vector<u32>& parameters) {
    const u32 instance_count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);
    const u32 element_base = parameters[4];
    const u32 base_instance = parameters[5];
    maxwell3d.regs.index_array.first = parameters[3];
    maxwell3d.regs.reg_array[0x446] = element_base; // vertex id base?
    maxwell3d.regs.index_array.count = parameters[1];
    maxwell3d.regs.vb_element_base = element_base;
    maxwell3d.regs.vb_base_instance = base_instance;
    maxwell3d.regs.const_buffer.cb_pos = 0x640;
    maxwell3d.mme_draw.instance_count = instance_count;
    maxwell3d.regs.const_buffer.cb_data[0] = element_base;
    maxwell3d.regs.const_buffer.cb_data[1] = base_instance;
    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0]));
    if (maxwell3d.ShouldExecute()) {
        maxwell3d.GetRasterizer().Draw(true, true);
    }
    maxwell3d.regs.reg_array[0x446] = 0x0; // vertex id base?
    maxwell3d.regs.index_array.count = 0;
    maxwell3d.regs.vb_element_base = 0x0;
    maxwell3d.regs.vb_base_instance = 0x0;
    maxwell3d.regs.const_buffer.cb_pos = 0x640;
    maxwell3d.regs.const_buffer.cb_data[0] = 0;
    maxwell3d.regs.const_buffer.cb_data[1] = 0;
    maxwell3d.mme_draw.instance_count = 0;
}

static const std::unordered_map<u64, HLEFunction> hle_funcs{
    {0x771BB18C62444DA0, &HLE_771BB18C62444DA0},
    {0x0D61FC9FAAC9FCAD, &HLE_0D61FC9FAAC9FCAD},
    {0x0217920100488FF7, &HLE_0217920100488FF7},
};

HLEMacro::HLEMacro(Engines::Maxwell3D& maxwell3d) : maxwell3d(maxwell3d) {}
HLEMacro::~HLEMacro() = default;

std::optional<std::unique_ptr<CachedMacro>> HLEMacro::GetHLEProgram(u64 hash) const {
    auto it = hle_funcs.find(hash);
    if (it != hle_funcs.end()) {
        return std::make_unique<HLEMacroImpl>(maxwell3d, it->second);
    } else {
        return {};
    }
}

HLEMacroImpl::~HLEMacroImpl() = default;

HLEMacroImpl::HLEMacroImpl(Engines::Maxwell3D& maxwell3d, HLEFunction func)
    : maxwell3d(maxwell3d), func(func) {}

void HLEMacroImpl::Execute(const std::vector<u32>& parameters, u32 method) {
    func(maxwell3d, parameters);
}

} // namespace Tegra
