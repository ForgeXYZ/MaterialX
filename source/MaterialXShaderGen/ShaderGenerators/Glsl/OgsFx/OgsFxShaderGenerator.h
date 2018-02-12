#ifndef MATERIALX_OGSFXSHADERGENERATOR_H
#define MATERIALX_OGSFXSHADERGENERATOR_H

#include <MaterialXShaderGen/ShaderGenerators/Glsl/GlslShaderGenerator.h>
#include <MaterialXShaderGen/HwShader.h>

namespace MaterialX
{

using OgsFxShaderPtr = shared_ptr<class OgsFxShader>;

/// Shader class targeting the OgsFX file format.
/// Extending HwShader with a new stage holding the final 
/// composited OsgFx shader.
class OgsFxShader : public HwShader
{
public:
    /// Identifier for final effects stage
    static const size_t FINAL_FX_STAGE = HwShader::NUM_STAGES;
    static const size_t NUM_STAGES = HwShader::NUM_STAGES + 1;

public:
    OgsFxShader(const string& name) : HwShader(name) {}

    size_t numStages() const override { return NUM_STAGES; }

    void createUniform(const string& block, const string& type, const string& name, const string& sementic = EMPTY_STRING, ValuePtr value = nullptr) override;
    void createAppData(const string& type, const string& name, const string& sementic = EMPTY_STRING) override;
    void createVertexData(const string& type, const string& name, const string& sementic = EMPTY_STRING) override;
};


/// A GLSL shader generator targeting the OgsFX file format
class OgsFxShaderGenerator : public GlslShaderGenerator
{
public:
    OgsFxShaderGenerator();

    static ShaderGeneratorPtr creator() { return std::make_shared<OgsFxShaderGenerator>(); }

    /// Return a unique identifyer for the target this generator is for
    const string& getTarget() const override { return TARGET; }

    /// Return the v-direction used by the target system
    Shader::VDirection getTargetVDirection() const override { return Shader::VDirection::DOWN; }

    /// Generate a shader starting from the given element, translating 
    /// the element and all dependencies upstream into shader code.
    ShaderPtr generate(const string& shaderName, ElementPtr element) override;

    /// Emit a shader uniform input variable
    void emitUniform(const Shader::Variable& uniform, Shader& shader) override;

    /// Unique identifyer for this generator target
    static const string TARGET;
};

}

#endif
