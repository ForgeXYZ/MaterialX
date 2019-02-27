#include <MaterialXGenGlsl/Nodes/GeomAttrValueNodeGlsl.h>

namespace MaterialX
{

ShaderNodeImplPtr GeomAttrValueNodeGlsl::create()
{
    return std::make_shared<GeomAttrValueNodeGlsl>();
}

void GeomAttrValueNodeGlsl::createVariables(Shader& shader, GenContext&, const ShaderGenerator&, const ShaderNode& node) const
{
    const ShaderInput* attrNameInput = node.getInput(ATTRNAME);
    if (!attrNameInput || !attrNameInput->getValue())
    {
        throw ExceptionShaderGenError("No 'attrname' parameter found on geomattrvalue node '" + node.getName() + "', don't know what attribute to bind");
    }
    const string attrName = attrNameInput->getValue()->getValueString();
    ShaderStage& ps = shader.getStage(HW::PIXEL_STAGE);
    ShaderPort* uniform = addStageUniform(ps, HW::PRIVATE_UNIFORMS, node.getOutput()->getType(), "u_geomattr_" + attrName);
    uniform->setPath(attrNameInput->getPath());
}

void GeomAttrValueNodeGlsl::emitFunctionCall(ShaderStage& stage, GenContext& context, const ShaderGenerator& shadergen, const ShaderNode& node) const
{
BEGIN_SHADER_STAGE(stage, HW::PIXEL_STAGE)
    const ShaderInput* attrNameInput = node.getInput(ATTRNAME);
    if (!attrNameInput)
    {
        throw ExceptionShaderGenError("No 'attrname' parameter found on geomattrvalue node '" + node.getName() + "', don't know what attribute to bind");
    }
    const string attrName = attrNameInput->getValue()->getValueString();
    shadergen.emitLineBegin(stage);
    shadergen.emitOutput(stage, context, node.getOutput(), true, false);
    shadergen.emitString(stage, " = u_geomattr_" + attrName);
    shadergen.emitLineEnd(stage);
END_SHADER_STAGE(shader, HW::PIXEL_STAGE)
}

} // namespace MaterialX
