#include "he_engine.h"

#include "he_images.h"
#include "he_pipelines.h"

void HeapEngine::draw_background(VkCommandBuffer cmd)
{
  ComputeEffect &effect = backgroundEffects[currentBackgroundEffect];

  // bind the background compute pipeline
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

  vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
  vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void HeapEngine::init_descriptors()
{
  // Create descriptors used by the compute background shader.
  std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
  globalDescriptorAllocator.init_pool(_device, 10, sizes);

  DescriptorLayoutBuilder builder;
  builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

  _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = _drawImage.imageView;

  VkWriteDescriptorSet drawImageWrite = {};
  drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  drawImageWrite.dstBinding = 0;
  drawImageWrite.dstSet = _drawImageDescriptors;
  drawImageWrite.descriptorCount = 1;
  drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  drawImageWrite.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

  _mainDeletionQueue.push_function([&]()
                                   {
    globalDescriptorAllocator.destroy_pool(_device);
    vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr); });
}

void HeapEngine::init_pipelines()
{
  VkPipelineLayoutCreateInfo computeLayout{};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pNext = nullptr;
  computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
  computeLayout.setLayoutCount = 1;

  VkPushConstantRange pushConstant{};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputePushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  computeLayout.pPushConstantRanges = &pushConstant;
  computeLayout.pushConstantRangeCount = 1;

  VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

  VkShaderModule gradientShader;
  if (!vkutil::load_shader_module("gradient_color.spv", _device, &gradientShader))
  {
    fmt::print("Error when building the compute shader \n");
  }

  VkShaderModule skyShader;
  if (!vkutil::load_shader_module("sky.spv", _device, &skyShader))
  {
    fmt::print("Error when building the compute shader \n");
  }

  VkPipelineShaderStageCreateInfo stageinfo{};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.pNext = nullptr;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = gradientShader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.pNext = nullptr;
  computePipelineCreateInfo.layout = _gradientPipelineLayout;
  computePipelineCreateInfo.stage = stageinfo;

  ComputeEffect gradient;
  gradient.layout = _gradientPipelineLayout;
  gradient.name = "gradient";
  gradient.data = {};

  // default colors
  gradient.data.data1 = glm::vec4(1, 0, 0, 1);
  gradient.data.data2 = glm::vec4(0, 0, 1, 1);

  VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

  // change the shader module only to create the sky shader
  computePipelineCreateInfo.stage.module = skyShader;

  ComputeEffect sky;
  sky.layout = _gradientPipelineLayout;
  sky.name = "sky";
  sky.data = {};
  // default sky parameters
  sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

  VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

  // add the 2 background effects into the array
  backgroundEffects.push_back(gradient);
  backgroundEffects.push_back(sky);

  // destroy structures properly
  vkDestroyShaderModule(_device, gradientShader, nullptr);
  vkDestroyShaderModule(_device, skyShader, nullptr);
  _mainDeletionQueue.push_function([=]()
                                   {
	vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
	vkDestroyPipeline(_device, sky.pipeline, nullptr);
	vkDestroyPipeline(_device, gradient.pipeline, nullptr); });
}