#include <MaterialXGenGlsl/Nodes/SurfaceNodeGlsl.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

namespace MaterialX
{

namespace
{
    static const string LIGHT_LOOP_BEGIN =
        "vec3 N = normalize(vd.normalWorld);\n"
        "vec3 V = normalize(u_viewPosition - vd.positionWorld);\n"
        "int numLights = numActiveLightSources();\n"
        "lightshader lightShader;\n"
        "for (int activeLightIndex = 0; activeLightIndex < numLights; ++activeLightIndex)\n";

    static const string LIGHT_CONTRIBUTION =
        "sampleLightSource(u_lightData[activeLightIndex], vd.positionWorld, lightShader);\n"
        "vec3 L = lightShader.direction;\n";
}

SurfaceNodeGlsl::SurfaceNodeGlsl()
{
    // Create closure contexts for calling closure functions.
    //
    // Reflection context
    _callReflection = HwClosureContext::create(HwClosureContext::REFLECTION);
    _callReflection->setSuffix("_reflection");
    _callReflection->addArgument(Type::VECTOR3, HW::LIGHT_DIR);
    _callReflection->addArgument(Type::VECTOR3, HW::VIEW_DIR);
    // Transmission context
    _callTransmission = HwClosureContext::create(HwClosureContext::TRANSMISSION);
    _callTransmission->setSuffix("_transmission");
    _callTransmission->addArgument(Type::VECTOR3, HW::VIEW_DIR);
    // Indirect context
    _callIndirect = HwClosureContext::create(HwClosureContext::INDIRECT);
    _callIndirect->setSuffix("_indirect");
    _callIndirect->addArgument(Type::VECTOR3, HW::VIEW_DIR);
    // Emission context
    _callEmission = HwClosureContext::create(HwClosureContext::EMISSION);
    _callEmission->addArgument(Type::VECTOR3, HW::NORMAL_DIR);
    _callEmission->addArgument(Type::VECTOR3, HW::VIEW_DIR);
}

ShaderNodeImplPtr SurfaceNodeGlsl::create()
{
    return std::make_shared<SurfaceNodeGlsl>();
}

void SurfaceNodeGlsl::createVariables(Shader& shader, GenContext&, const ShaderGenerator&, const ShaderNode&) const
{
    // TODO: 
    // The surface shader needs position, normal, view position and light sources. We should solve this by adding some 
    // dependency mechanism so this implementation can be set to depend on the PositionNodeGlsl, NormalNodeGlsl
    // ViewDirectionNodeGlsl and LightNodeGlsl nodes instead? This is where the MaterialX attribute "internalgeomprops" 
    // is needed.
    //
    ShaderStage& vs = shader.getStage(HW::VERTEX_STAGE);
    ShaderStage& ps = shader.getStage(HW::PIXEL_STAGE);

    addStageInput(vs, HW::VERTEX_INPUTS, Type::VECTOR3, "i_position");
    addStageInput(vs, HW::VERTEX_INPUTS, Type::VECTOR3, "i_normal");
    addStageUniform(vs, HW::PRIVATE_UNIFORMS, Type::MATRIX44, "u_worldInverseTransposeMatrix");

    addStageConnector(vs, ps, HW::VERTEX_DATA, Type::VECTOR3, "positionWorld");
    addStageConnector(vs, ps, HW::VERTEX_DATA, Type::VECTOR3, "normalWorld");

    addStageUniform(ps, HW::PRIVATE_UNIFORMS, Type::VECTOR3, "u_viewPosition");
    addStageUniform(ps, HW::PRIVATE_UNIFORMS, Type::INTEGER, "u_numActiveLightSources", Value::createValue<int>(0));
}

void SurfaceNodeGlsl::emitFunctionCall(ShaderStage& stage, GenContext& context, const ShaderGenerator& shadergen_, const ShaderNode& node) const
{
    const GlslShaderGenerator& shadergen = static_cast<const GlslShaderGenerator&>(shadergen_);

    const ShaderGraph& graph = *node.getParent();

    BEGIN_SHADER_STAGE(stage, HW::VERTEX_STAGE)
        VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
        const string prefix = vertexData.getInstance() + ".";
        ShaderPort* position = vertexData["positionWorld"];
        if (!position->isEmitted())
        {
            position->setEmitted();
            shadergen.emitLine(stage, prefix + position->getVariable() + " = hPositionWorld.xyz");
        }
        ShaderPort* normal = vertexData["normalWorld"];
        if (!normal->isEmitted())
        {
            normal->setEmitted();
            shadergen.emitLine(stage, prefix + normal->getVariable() + " = normalize((u_worldInverseTransposeMatrix * vec4(i_normal, 0)).xyz)");
        }
    END_SHADER_STAGE(stage, HW::VERTEX_STAGE)

    BEGIN_SHADER_STAGE(stage, HW::PIXEL_STAGE)
        // Declare the output variable
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(stage, context, node.getOutput(), true, true);
        shadergen.emitLineEnd(stage);

        shadergen.emitScopeBegin(stage);

        if (context.getOptions().hwTransparency)
        {
            shadergen.emitLineBegin(stage);
            shadergen.emitString(stage, "float surfaceOpacity = ");
            shadergen.emitInput(stage, context, node.getInput("opacity"));
            shadergen.emitLineEnd(stage);
            // Early out for 100% cutout transparency
            shadergen.emitLine(stage, "if (surfaceOpacity < 0.001)", false);
            shadergen.emitScopeBegin(stage);
            shadergen.emitLine(stage, "discard");
            shadergen.emitScopeEnd(stage);
            shadergen.emitLineBreak(stage);
        }

        const ShaderOutput* output = node.getOutput();
        const string outColor = output->getVariable() + ".color";
        const string outTransparency = output->getVariable() + ".transparency";

        //
        // Handle direct lighting
        //

        shadergen.emitComment(stage, "Light loop");
        shadergen.emitBlock(stage, context, LIGHT_LOOP_BEGIN);
        shadergen.emitScopeBegin(stage);

        shadergen.emitBlock(stage, context, LIGHT_CONTRIBUTION);
        shadergen.emitLineBreak(stage);

        shadergen.emitComment(stage, "Calculate the BSDF response for this light source");
        string bsdf;
        shadergen.emitBsdfNodes(stage, context, graph, node, _callReflection, bsdf);
        shadergen.emitLineBreak(stage);

        shadergen.emitComment(stage, "Accumulate the light's contribution");
        shadergen.emitLine(stage, outColor + " += lightShader.intensity * " + bsdf);

        shadergen.emitScopeEnd(stage);
        shadergen.emitLineBreak(stage);

        //
        // Handle indirect lighting.
        //

        shadergen.emitComment(stage, "Add surface emission");
        shadergen.emitScopeBegin(stage);
        string emission;
        shadergen.emitEdfNodes(stage, context, graph, node, _callEmission, emission);
        shadergen.emitLine(stage, outColor + " += " + emission);
        shadergen.emitScopeEnd(stage);
        shadergen.emitLineBreak(stage);

        shadergen.emitComment(stage, "Add indirect contribution");
        shadergen.emitScopeBegin(stage);
        shadergen.emitBsdfNodes(stage, context, graph, node, _callIndirect, bsdf);
        shadergen.emitLineBreak(stage);
        shadergen.emitLine(stage, outColor + " += " + bsdf);
        shadergen.emitScopeEnd(stage);
        shadergen.emitLineBreak(stage);

        // Handle surface transparency
        //
        if (context.getOptions().hwTransparency)
        {
            shadergen.emitComment(stage, "Calculate the BSDF transmission for viewing direction");
            shadergen.emitScopeBegin(stage);
            shadergen.emitBsdfNodes(stage, context, graph, node, _callTransmission, bsdf);
            shadergen.emitLine(stage, outTransparency + " = " + bsdf);
            shadergen.emitScopeEnd(stage);
            shadergen.emitLineBreak(stage);

            shadergen.emitComment(stage, "Mix in opacity which affect the total result");
            shadergen.emitLine(stage, outColor + " *= surfaceOpacity");
            shadergen.emitLine(stage, outTransparency + " = mix(vec3(1.0), " + outTransparency + ", surfaceOpacity)");
        }
        else
        {
            shadergen.emitLine(stage, outTransparency + " = vec3(0.0)");
        }

        shadergen.emitScopeEnd(stage);
        shadergen.emitLineBreak(stage);

    END_SHADER_STAGE(stage, HW::PIXEL_STAGE)
}

} // namespace MaterialX
