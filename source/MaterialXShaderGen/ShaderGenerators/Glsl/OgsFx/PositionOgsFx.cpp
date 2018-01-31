#include <MaterialXShaderGen/ShaderGenerators/Glsl/OgsFx/PositionOgsFx.h>
#include <MaterialXShaderGen/ShaderGenerator.h>
#include <MaterialXShaderGen/SgNode.h>
#include <MaterialXShaderGen/HwShader.h>

namespace MaterialX
{

SgImplementationPtr PositionOgsFx::creator()
{
    return std::make_shared<PositionOgsFx>();
}

void PositionOgsFx::registerVariables(const SgNode& node, ShaderGenerator& /*shadergen*/, Shader& shader)
{
    shader.registerInput(Shader::Variable("vec3", "inPosition", "POSITION"), HwShader::VERTEX_STAGE);
    shader.registerUniform(Shader::Variable("mat4", "gWorldXf", "World"), HwShader::VERTEX_STAGE);

    const SgInput* spaceInput = node.getInput(SPACE);
    string space = spaceInput ? spaceInput->value->getValueString() : "";
    if (space == WORLD)
    {
        shader.registerOutput(Shader::Variable("vec3", "WorldPosition", "POSITION"), HwShader::VERTEX_STAGE);
    }
    else if (space == MODEL)
    {
        shader.registerOutput(Shader::Variable("vec3", "ModelPosition", "POSITION"), HwShader::VERTEX_STAGE);
    }
    else
    {
        shader.registerOutput(Shader::Variable("vec3", "ObjectPosition", "POSITION"), HwShader::VERTEX_STAGE);
    }
}

void PositionOgsFx::emitFunctionCall(const SgNode& node, ShaderGenerator& shadergen, Shader& shader_)
{
    HwShader& shader = static_cast<HwShader&>(shader_);

    BEGIN_SHADER_STAGE(shader, HwShader::VERTEX_STAGE)
        const SgInput* spaceInput = node.getInput(SPACE);
        string space = spaceInput ? spaceInput->value->getValueString() : "";
        if (space == WORLD)
        {
            if (!shader.isCalculated("WorldPosition"))
            {
                shader.setCalculated("WorldPosition");
                shader.addLine("VS_OUT.WorldPosition = Pw.xyz");
            }
        }
        else if (space == MODEL)
        {
            if (!shader.isCalculated("ModelPosition"))
            {
                shader.setCalculated("ModelPosition");
                shader.addLine("VS_OUT.ModelPosition = Po.xyz");
            }
        }
        else
        {
            if (!shader.isCalculated("ObjectPosition"))
            {
                shader.setCalculated("ObjectPosition");
                shader.addLine("VS_OUT.ObjectPosition = Po.xyz");
            }
        }
    END_SHADER_STAGE(shader, HwShader::VERTEX_STAGE)

    BEGIN_SHADER_STAGE(shader, HwShader::PIXEL_STAGE)
        shader.beginLine();
        shadergen.emitOutput(node.getOutput(), true, shader);

        const SgInput* spaceInput = node.getInput(SPACE);
        string space = spaceInput ? spaceInput->value->getValueString() : "";
        if (space == WORLD)
        {
            shader.addStr(" = PS_IN.WorldPosition");
        }
        else if (space == MODEL)
        {
            shader.addStr(" = PS_IN.ModelPosition");
        }
        else
        {
            shader.addStr(" = PS_IN.ObjectPosition");
        }

        shader.endLine();
    END_SHADER_STAGE(shader, HwShader::PIXEL_STAGE)
}

} // namespace MaterialX
