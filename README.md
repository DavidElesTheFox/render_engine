# render_engine
Experimental vulkan render engine
The goal is to create a flexible render engine that 
 - supports multiple windows
 - place for making experimental render code.

## Dependencies
Library uses 
 - volk for vulkan loading
 - imgui as debug UI.
 - glm as math library
 - ADR templates for documentation of design decisions
 - sdl for window handling
 - CT image source: https://isbweb.org/data/vsj/ (Thank you)
   - exported to image with: https://www.radiantviewer.com/

 # Design decisions

 There are some major design decision what was made during the library creation. Some of those are experimental decisions to figure out
 its consecvencies. 

 See 
  - [Error handling](render_engine/documentation/handling-errors.md)
  - [Object initialization](render_engine/documentation/object-initializations.md)
  - [Objects responsibility](render_engine/documentation/objects-responsibility.md)
  - [Singletons](render_engine/documentation/singletons.md)
  - [Resource State Machine](render_engine/documentation/resource-state-machine.md)
  - [Off-Screen Rendering](render_engine/documentation/offscreen-rendering.md)
  - [DescriptorSetLayout Setup](render_engine/documentation/descriptor-set-layout-setup.md)
  - [Volume Rendering](render_engine/documentation/volume-rendering.md)
  - [Distance Field Calculation and Compute](render_engine/documentation/distance-field-calculation.md)