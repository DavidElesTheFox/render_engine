# Logical Device

## Status

accepted

## Context

To support multiple GPU the usage of volk need to be change. It needs to use the function table scenario during initialization.

The goal is to create a LogicalDevice class that is a wrapper around the VkLogicalDevice and the vulkan API functions.

## Decision

The goal is to handle every function call via the Volk device table. Thus, no global vulkan prototypes are loaded anymore.

The wnew object LogicalDevice basically the VulkanDevice that encapsulates the API calls. The goal is to make this less painful as it is possible.
To achive this the LogicalDevice behaves like the API interface (like the Volk Function Table) and it is basically a VkDevice. Thus it can be used like:
```cpp
logical_device->vkDeviceWaitIdle(*logical_device); 
```
- with `operator->` one can access the device functions.
- with `operator*` one can use the logical device as a VkDevice.

## Consequences

- Multiple Device Support
- More complex ImGui handling. ImGui uses global function prototypes. It can be loaded via the `ImGui_ImplVulkan_LoadFunctions` but still it bounded to one device.
To handle this situation we can use the assumption that only the window that is in focus renders the ImGui UI. Thus, whenever the global device is changed the vulkan functions are reloaded.
