#include "he_engine.h"

#include "he_images.h"
#include "he_pipelines.h"

void HeapEngine::draw_background(VkCommandBuffer cmd)
{
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

  vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void HeapEngine::init_descriptors()
{
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
  init_background_pipelines();
}

void HeapEngine::init_background_pipelines()
{
  VkPipelineLayoutCreateInfo computeLayout{};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
  computeLayout.setLayoutCount = 1;

  VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

  VkShaderModule computeDrawShader;
  if (!vkutil::load_shader_module("gradient.spv", _device, &computeDrawShader))
  {
    vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
    fmt::print("Error when building the compute shader\n");
    abort();
  }

  VkPipelineShaderStageCreateInfo stageinfo{};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = computeDrawShader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = _gradientPipelineLayout;
  computePipelineCreateInfo.stage = stageinfo;

  VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));
  vkDestroyShaderModule(_device, computeDrawShader, nullptr);

  _mainDeletionQueue.push_function([&]()
                                   {
    vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
    vkDestroyPipeline(_device, _gradientPipeline, nullptr); });
}
