#ifndef MATERIALX_LIGHTSAMPLERNODEGLSL_H
#define MATERIALX_LIGHTSAMPLERNODEGLSL_H

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

namespace MaterialX
{

/// Utility node for sampling lights for GLSL.
class LightSamplerNodeGlsl : public GlslImplementation
{
public:
    static ShaderNodeImplPtr create();

    void emitFunctionDefinition(ShaderStage& stage, GenContext& context, const ShaderGenerator& shadergen, const ShaderNode& node) const override;
};


} // namespace MaterialX

#endif
