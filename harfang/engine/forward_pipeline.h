// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/color.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/rect.h"
#include "foundation/unit.h"
#include "foundation/vector4.h"

#include <bgfx/bgfx.h>

#include <array>
#include <map>
#include <string>
#include <vector>

namespace hg {

static const int forward_light_count = 8;

enum ForwardPipelineLightType { FPLT_None, FPLT_Point, FPLT_Spot, FPLT_Linear };
enum ForwardPipelineShadowType { FPST_None, FPST_Map };

struct ForwardPipelineLight { // 112B
	ForwardPipelineLightType type;
	ForwardPipelineShadowType shadow_type;

	Mat4 world; // 48B

	Color diffuse, specular;

	float radius;
	float inner_angle, outer_angle;
	Vec4 pssm_split;
	float priority;
	float shadow_bias;
};

ForwardPipelineLight MakeForwardPipelinePointLight(const hg::Mat4 &world, const hg::Color &diffuse, const hg::Color &specular, float radius = 0.f,
	float priority = 0.f, ForwardPipelineShadowType shadow_type = FPST_None, float shadow_bias = default_shadow_bias);
ForwardPipelineLight MakeForwardPipelineSpotLight(const hg::Mat4 &world, const hg::Color &diffuse, const hg::Color &specular, float radius = 0.f,
	float inner_angle = hg::Deg(40.f), float outer_angle = hg::Deg(45.f), float priority = 0.f, ForwardPipelineShadowType shadow_type = FPST_None,
	float shadow_bias = default_shadow_bias);
ForwardPipelineLight MakeForwardPipelineLinearLight(const hg::Mat4 &world, const hg::Color &diffuse, const hg::Color &specular,
	const hg::Vec4 &pssm_split = {10.f, 50.f, 100.f, 500.f}, float priority = 0.f, ForwardPipelineShadowType shadow_type = FPST_None,
	float shadow_bias = default_shadow_bias);

//
struct ForwardPipelineLights {
	std::array<Vec4, forward_light_count> pos, dir, diff, spec; // shader uniforms
	std::array<ForwardPipelineLight, forward_light_count> lights; // lights that were used to fill uniform values
};

ForwardPipelineLights PrepareForwardPipelineLights(const std::vector<ForwardPipelineLight> &lights);

struct ForwardPipelineShadowData {
	Mat44 linear_shadow_mtx[4]; // slot 0: 4 split PSSM linear light
	Vec4 linear_shadow_slice; // slot 0: PSSM slice distances linear light
	Mat44 spot_shadow_mtx; // slot 1: spot light
};

//
struct ForwardPipelineFog {
	float near{}, far{};
	Color color{};
};

//
struct ForwardPipeline : Pipeline {
	int shadow_map_resolution{1024};
};

ForwardPipeline CreateForwardPipeline(int shadow_map_resolution = 1024, bool spot_16bit_shadow_map = true);
inline void DestroyForwardPipeline(ForwardPipeline &pipeline) { DestroyPipeline(pipeline); }

//
void UpdateForwardPipeline(ForwardPipeline &pipeline, const ForwardPipelineShadowData &shadow_data, const Color &ambient, const ForwardPipelineLights &lights,
	const ForwardPipelineFog &fog);
void UpdateForwardPipelineNoise(ForwardPipeline &pipeline, Texture noise);
void UpdateForwardPipelinePBRProbe(ForwardPipeline &pipeline, Texture irradiance, Texture radiance, Texture brdf);
void UpdateForwardPipelineAO(ForwardPipeline &pipeline, Texture ao);
void UpdateForwardPipelineAAA(ForwardPipeline &pipeline, const iRect &rect, const Mat4 &view, const Mat44 &proj, const Mat4 &prv_view, const Mat44 &prv_proj,
	const Vec2 &jitter, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio, float temporal_aa_weight, float motion_blur_strength,
	float exposure, float gamma, int sample_count, float max_distance);

const PipelineInfo &GetForwardPipelineInfo();

//
enum ForwardPipelineStage { FPS_AttributeBuffers, FPS_Basic, FPS_Advanced };

void SubmitModelToForwardPipeline(bgfx::ViewId view_id, const Model &mdl, const ForwardPipeline &pipeline, const PipelineProgram &prg, uint32_t prg_variant,
	uint8_t pipeline_config_idx, const Color &ambient, const ForwardPipelineLights &lights, const ForwardPipelineFog &fog, const Mat4 &mtx);

//
enum ForwardPipelineShadowPass { FPSP_Slot0LinearSplit0, FPSP_Slot0LinearSplit1, FPSP_Slot0LinearSplit2, FPSP_Slot0LinearSplit3, FPSP_Slot1Spot, FPSP_Count };

using ForwardPipelineShadowPassViewId = std::array<bgfx::ViewId, FPSP_Count>;

void GenerateLinearShadowMapForForwardPipeline(bgfx::ViewId &view_id, const ViewState &view_state, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineShadowPassViewId &views, ForwardPipelineShadowData &shadow_data,
	const char *debug_name = nullptr);

void GenerateSpotShadowMapForForwardPipeline(bgfx::ViewId &view_id, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineShadowPassViewId &view, ForwardPipelineShadowData &shadow_data,
	const char *debug_name = nullptr);

} // namespace hg